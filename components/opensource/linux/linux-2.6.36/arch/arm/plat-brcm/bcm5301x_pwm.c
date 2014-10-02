
#include <linux/proc_fs.h>
#include <asm/uaccess.h>
#include <typedefs.h>
#include <bcmutils.h>

/* Parameters for each PWM channel */
typedef struct pwm_ch_params {
	uint32 ch_num;			/* which channel */
	uint32 prescale;		/* prescale 0~0x3F */
	uint32 period_cnt;		/* period count 0~0xFFFF */
	uint32 duty_hi_cnt;		/* duty high count 0~0xFFFF */
	uint32 output_en;		/* waveform output enable for each channel */
} pwm_ch_params_t;

#define PWM_PARAMS_NUM ((sizeof(pwm_ch_params_t)) / (sizeof(uint32)))

#define PWM_CHANNEL_NUM						(4)
#define PWM_PRESCALE_MASK					(0x3F)
#define PWM_PERIOD_MASK						(0xFFFF)
#define PWM_DUTYHI_MASK						(0xFFFF)

/* GPIO PIN MUX registers */
#define DMU_CRU_GPIO_CTRL0					(0x1800C1C0)
#define CCB_GPIO_AUX_SEL					(0x18001028)

/* PWM base register */
#define PWM_BASE							(0x18002000)
/* PWM reg offset */
#define PWM_CTL								(0x00)
#define PWM_CH0_PERIOD_CNT					(0x04)
#define PWM_CH0_DUTYHI_CNT					(0x08)
#define PWM_CH1_PERIOD_CNT					(0x0C)
#define PWM_CH1_DUTYHI_CNT					(0x10)
#define PWM_CH2_PERIOD_CNT					(0x14)
#define PWM_CH2_DUTYHI_CNT					(0x18)
#define PWM_CH3_PERIOD_CNT					(0x1C)
#define PWM_CH3_DUTYHI_CNT					(0x20)
#define PWM_PRESCALE						(0x24)
/* PWM regsiters */
#define PWM_CTL_REG(base)					(base + PWM_CTL)
#define PWM_CH_PERIOD_CNT_REG(base, ch)		(base + PWM_CH0_PERIOD_CNT + (ch * 8))
#define PWM_CH_DUTYHI_CNT_REG(base, ch)		(base + PWM_CH0_DUTYHI_CNT + (ch * 8))
#define PWM_PRESCALE_REG(base)				(base + PWM_PRESCALE)

static uint32 pwm_base;

/* Validate the legal range of PWM channel parameters except output_en */
static bool
pwm_ch_params_is_valid(pwm_ch_params_t *ch_params)
{
	bool valid = FALSE;

	/* total 4 PWM channels */
	if (ch_params->ch_num > (PWM_CHANNEL_NUM - 1))
		goto out;

	/* prescale range is 0~0x3F */
	if (ch_params->prescale & ~PWM_PRESCALE_MASK)
		goto out;

	/* period count range is 0~0xFFFF */
	if (ch_params->period_cnt & ~PWM_PERIOD_MASK)
		goto out;

	/* duty high count range is 0~0xFFFF */
	if (ch_params->duty_hi_cnt & ~PWM_DUTYHI_MASK)
		goto out;

	valid = TRUE;
out:
	return valid;
}

/* Configure prescale, periodCnt, and dutyHiCnt for specific channel
 * frequency = pwm input clock 1M/((prescale + 1) x periodCnt)
 * dutyCycle = dutyHiCnt/periodCnt
 */
static void
pwm_ch_config(pwm_ch_params_t *ch_params)
{
	uint32 val32;

	/* Set prescale, bit[23:18] is ch0 prescale ... bit[5:0] is ch3 prescale */
	val32 = readl(PWM_PRESCALE_REG(pwm_base));
	val32 &= ~(PWM_PRESCALE_MASK << (18 - (ch_params->ch_num * 6)));
	val32 |= (ch_params->prescale << (18 - (ch_params->ch_num * 6)));
	writel(val32, PWM_PRESCALE_REG(pwm_base));

	/* Set period count */
	val32 = ch_params->period_cnt & PWM_PERIOD_MASK;
	writel(val32, PWM_CH_PERIOD_CNT_REG(pwm_base, ch_params->ch_num));

	/* Set duty high count */
	val32 = ch_params->duty_hi_cnt & PWM_DUTYHI_MASK;
	writel(val32, PWM_CH_DUTYHI_CNT_REG(pwm_base, ch_params->ch_num));
}

/* Enable/disable waveform outputing on a specific PWM channel */
static void
pwm_ch_output(uint32 ch_num, bool on)
{
	uint32 val32;

	val32 = readl(PWM_CTL_REG(pwm_base));

	if (on)
		val32 |= (1 << ch_num);
	else
		val32 &= ~(1 << ch_num);

	writel(val32, PWM_CTL_REG(pwm_base));
}

/* Get PWM channel status */
static void
pwm_ch_status(pwm_ch_params_t *ch_params, uint32 ch_num)
{
	uint32 val32;

	if (!pwm_base)
		return;

	/* channel number */
	ch_params->ch_num = ch_num;

	/* prescale */
	val32 = readl(PWM_PRESCALE_REG(pwm_base));
	ch_params->prescale = (val32 >> (18 - (ch_num * 6))) & PWM_PRESCALE_MASK;

	/* period count */
	val32 = readl(PWM_CH_PERIOD_CNT_REG(pwm_base, ch_num));
	ch_params->period_cnt = val32 & PWM_PERIOD_MASK;

	/* duty high count */
	val32 = readl(PWM_CH_DUTYHI_CNT_REG(pwm_base, ch_num));
	ch_params->duty_hi_cnt = val32 & PWM_DUTYHI_MASK;

	/* waveform output enable */
	val32 = readl(PWM_CTL_REG(pwm_base));
	ch_params->output_en = (val32 & (1 << ch_num)) ? TRUE : FALSE;
}

static char
*get_token(char **buffer)
{
	char *p = NULL;

	while ((p = strsep(buffer, " ")) != NULL) {
		if (!*p || p[0] == ' ' || p[0] == '\0') {
			continue;
		}
		break;
	}

	return p;
}

/* Usage:
 * Set GPIO MUX to turn on/off PWM function on channel ch_num 0~3 (gpio 8~11)
 * echo "[ch_num] [on]" > /proc/pwm/enable
 * ch_num: 0 | 1 | 2 | 3
 * on: 0 | 1
 */
static int
pwm_enable_write_proc(struct file *file, const char __user *buf,
	unsigned long count, void *data)
{
	char *buffer, *tmp, *p;
	uint32 tokens, ch_num;
	bool pwm_func_en;
	void __iomem *dmu_cru_gpio_control0, *ccb_gpio_aux_sel;
	uint32 val32;

	buffer = kmalloc(count + 1, GFP_KERNEL);
	if (!buffer) {
		printk(KERN_ERR "%s: kmalloc failed.\n", __FUNCTION__);
		goto out;
	}

	if (copy_from_user(buffer, buf, count)) {
		printk(KERN_ERR "%s: copy_from_user failed.\n", __FUNCTION__);
		goto out;
	}

	buffer[count] = '\0';
	tmp = buffer;
	tokens = 0;

	if ((p = get_token((char **)&tmp))) {
		ch_num = simple_strtoul(p, NULL, 0);
		tokens++;
	}

	/* Check channel range */
	if (ch_num > (PWM_CHANNEL_NUM - 1))
		goto out;

	if ((p = get_token((char **)&tmp))) {
		pwm_func_en = (simple_strtoul(p, NULL, 0)) ? TRUE : FALSE;
		tokens++;
	}

	if (tokens != 2)
		goto out;

	/* Enable PWM function on channel 0~3 by clearing bit 8~11 in dmu_cru_gpio_control0
	 * and setting bit 0~3 in ccb_gpio_aux_sel
	 */
	dmu_cru_gpio_control0 = REG_MAP(DMU_CRU_GPIO_CTRL0, sizeof(uint32));
	val32 = readl(dmu_cru_gpio_control0);
	if (pwm_func_en)
		val32 &= ~(1 << (8 + ch_num));
	else
		val32 |= (1 << (8 + ch_num));
	writel(val32, dmu_cru_gpio_control0);
	REG_UNMAP(dmu_cru_gpio_control0);

	ccb_gpio_aux_sel = REG_MAP(CCB_GPIO_AUX_SEL, sizeof(uint32));
	val32 = readl(ccb_gpio_aux_sel);
	if (pwm_func_en)
		val32 |= (1 << ch_num);
	else
		val32 &= ~(1 << ch_num);
	writel(val32, ccb_gpio_aux_sel);
	REG_UNMAP(ccb_gpio_aux_sel);

out:
	if (buffer)
		kfree(buffer);

	return count;
}

/* Usage:
 * config PWM parameters for specific channel ch_num.
 * echo "[ch_num] [prescale] [periodCnt] [dutyHiCnt]" > /proc/pwm/config
 * ch_num: 0 | 1 | 2 | 3
 */
static int
pwm_config_write_proc(struct file *file, const char __user *buf,
	unsigned long count, void *data)
{
	char *buffer, *tmp, *p;
	uint32 tokens;
	uint32 *params;
	pwm_ch_params_t ch_params;
	int i;

	buffer = kmalloc(count + 1, GFP_KERNEL);
	if (!buffer) {
		printk(KERN_ERR "%s: kmalloc failed.\n", __FUNCTION__);
		goto out;
	}

	if (copy_from_user(buffer, buf, count)) {
		printk(KERN_ERR "%s: copy_from_user failed.\n", __FUNCTION__);
		goto out;
	}

	buffer[count] = '\0';
	tmp = buffer;
	tokens = 0;
	params = (uint32 *)&ch_params;

	/* Get PWM params */
	for (i = 0; i < (PWM_PARAMS_NUM - 1); i++) {
		if ((p = get_token((char **)&tmp))) {
			*params = (uint32)simple_strtoul(p, NULL, 0);
			params++;
			tokens++;
		}
	}

	/* Validate PWM params */
	if (tokens != (PWM_PARAMS_NUM - 1) || !pwm_ch_params_is_valid(&ch_params))
		goto out;

	/* PWM config */
	pwm_ch_config(&ch_params);

out:
	if (buffer)
		kfree(buffer);

	return count;
}

/* Usage:
 * start or stop outputing on channel ch_num
 * Note that PWM parameters of the channel shall be ready before starting outputing.
 * echo "[ch_num] [start]" > /proc/pwm/output
 * ch_num: 0 | 1 | 2 | 3
 * start: 0 | 1
 */
static int
pwm_output_write_proc(struct file *file, const char __user *buf,
	unsigned long count, void *data)
{
	char *buffer, *tmp, *p;
	uint32 tokens, ch_num;
	bool output_en;

	buffer = kmalloc(count + 1, GFP_KERNEL);
	if (!buffer) {
		printk(KERN_ERR "%s: kmalloc failed.\n", __FUNCTION__);
		goto out;
	}

	if (copy_from_user(buffer, buf, count)) {
		printk(KERN_ERR "%s: copy_from_user failed.\n", __FUNCTION__);
		goto out;
	}

	buffer[count] = '\0';
	tmp = buffer;
	tokens = 0;

	if ((p = get_token((char **)&tmp))) {
		ch_num = simple_strtoul(p, NULL, 0);
		tokens++;
	}

	/* Check channel range */
	if (ch_num > (PWM_CHANNEL_NUM - 1))
		goto out;

	if ((p = get_token((char **)&tmp))) {
		output_en = (simple_strtoul(p, NULL, 0)) ? TRUE : FALSE;
		tokens++;
	}

	if (tokens != 2)
		goto out;

	pwm_ch_output(ch_num, output_en);

out:
	if (buffer)
		kfree(buffer);

	return count;
}

/* Usage:
 * dump the status of all PWM channels
 * cat /proc/pwm/status
 */
static int
pwm_status_read_proc(char *buffer, char **start,
	off_t offset, int length, int *eof, void *data)
{
	void __iomem *dmu_cru_gpio_control0, *ccb_gpio_aux_sel;
	uint32 dmu_cru_gpio_control0_val, ccb_gpio_aux_sel_val;
	pwm_ch_params_t ch_params;
	int len;
	off_t pos, begin;
	int i;

	len = 0;
	pos = begin = 0;

	dmu_cru_gpio_control0 = REG_MAP(DMU_CRU_GPIO_CTRL0, sizeof(uint32));
	dmu_cru_gpio_control0_val = readl(dmu_cru_gpio_control0);
	REG_UNMAP(dmu_cru_gpio_control0);

	ccb_gpio_aux_sel = REG_MAP(CCB_GPIO_AUX_SEL, sizeof(uint32));
	ccb_gpio_aux_sel_val = readl(ccb_gpio_aux_sel);
	REG_UNMAP(ccb_gpio_aux_sel);

	for (i = 0; i < PWM_CHANNEL_NUM; i++) {
		pwm_ch_status(&ch_params, i);

		len += sprintf(buffer + len, "GPIO PIN MUX:\n");
		len += sprintf(buffer + len, "dmu_cru_gpio_control0: \tPWM channel %d func %s\n",
			i, (dmu_cru_gpio_control0_val & (1 << (8 + i))) ? "off" : "on");
		len += sprintf(buffer + len, "ccb_gpio_aux_sel: \tPWM channel %d func %s\n",
			i, (ccb_gpio_aux_sel_val & (1 << i)) ? "on" : "off");
		len += sprintf(buffer + len, "\nPWM channel %d config:\n", ch_params.ch_num);
		len += sprintf(buffer + len, "prescale: \t%d\n", ch_params.prescale);
		len += sprintf(buffer + len, "period_cnt: \t%d\n", ch_params.period_cnt);
		len += sprintf(buffer + len, "duty_hi_cnt: \t%d\n", ch_params.duty_hi_cnt);
		len += sprintf(buffer + len, "output_en: \t%s\n\n",
			ch_params.output_en ? "Enabled" : "Disabled");
	}

	pos = begin + len;

	if (pos < offset) {
		len = 0;
		begin = pos;
	}

	*eof = 1;

	*start = buffer + (offset - begin);
	len -= (offset - begin);

	if (len > length)
		len = length;

	return len;
}

static void __init
pwm_proc_init(void)
{
	struct proc_dir_entry *pwm_proc_dir, *pwm_enable, *pwm_config, *pwm_status, *pwm_output;

	pwm_proc_dir = pwm_enable = pwm_config = pwm_status = pwm_output = NULL;
	pwm_base = 0;

	pwm_base = (uint32)REG_MAP(PWM_BASE, 4096);
	if (!pwm_base)
		return;

	pwm_proc_dir = proc_mkdir("pwm", NULL);
	if (!pwm_proc_dir) {
		printk(KERN_ERR "%s: Create pwm_proc_dir failed.\n", __FUNCTION__);
		goto err;
	}

	pwm_enable = create_proc_entry("pwm/enable", 0, NULL);
	if (!pwm_enable) {
		printk(KERN_ERR "%s: Create pwm_enable failed.\n", __FUNCTION__);
		goto err;
	}
	pwm_enable->write_proc = pwm_enable_write_proc;

	pwm_config = create_proc_entry("pwm/config", 0, NULL);
	if (!pwm_config) {
		printk(KERN_ERR "%s: Create pwm_config failed.\n", __FUNCTION__);
		goto err;
	}
	pwm_config->write_proc = pwm_config_write_proc;

	pwm_output = create_proc_entry("pwm/output", 0, NULL);
	if (!pwm_output) {
		printk(KERN_ERR "%s: Create pwm_output failed.\n", __FUNCTION__);
		goto err;
	}
	pwm_output->write_proc = pwm_output_write_proc;

	pwm_status = create_proc_entry("pwm/status", 0, NULL);
	if (!pwm_status) {
		printk(KERN_ERR "%s: Create pwm_status failed.\n", __FUNCTION__);
		goto err;
	}
	pwm_status->read_proc = pwm_status_read_proc;

	return;

err:
	if (pwm_enable)
		remove_proc_entry("pwm/enable", NULL);
	if (pwm_config)
		remove_proc_entry("pwm/config", NULL);
	if (pwm_output)
		remove_proc_entry("pwm/output", NULL);
	if (pwm_proc_dir)
		remove_proc_entry("pwm", NULL);
	if (pwm_base)
		REG_UNMAP((void *)pwm_base);
}

static void __exit
pwm_proc_exit(void)
{
	if (!pwm_base)
		return;

	remove_proc_entry("pwm/enable", NULL);
	remove_proc_entry("pwm/config", NULL);
	remove_proc_entry("pwm/output", NULL);
	remove_proc_entry("pwm/status", NULL);
	remove_proc_entry("pwm", NULL);
	REG_UNMAP((void *)pwm_base);
}

module_init(pwm_proc_init);
module_exit(pwm_proc_exit);
