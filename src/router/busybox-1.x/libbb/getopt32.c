/* vi: set sw=4 ts=4: */
/*
 * universal getopt32 implementation for busybox
 *
 * Copyright (C) 2003-2005  Vladimir Oleynik  <dzo@simtreas.ru>
 *
 * Licensed under GPLv2 or later, see file LICENSE in this tarball for details.
 */

#include <getopt.h>
#include "libbb.h"


/* Code here assumes that 'unsigned' is at least 32 bits wide */

const char *opt_complementary;

typedef struct {
	int opt;
	int list_flg;
	unsigned switch_on;
	unsigned switch_off;
	unsigned incongruously;
	unsigned requires;
	void **optarg;               /* char **optarg or llist_t **optarg */
	int *counter;
} t_complementary;

/* You can set applet_long_options for parse called long options */
#if ENABLE_GETOPT_LONG
static const struct option bb_null_long_options[1] = {
	{ 0, 0, 0, 0 }
};
const char *applet_long_options;
#endif

uint32_t option_mask32;

uint32_t
getopt32(char **argv, const char *applet_opts, ...)
{
	int argc;
	unsigned flags = 0;
	unsigned requires = 0;
	t_complementary complementary[33];
	int c;
	const unsigned char *s;
	t_complementary *on_off;
	va_list p;
#if ENABLE_GETOPT_LONG
	const struct option *l_o;
	struct option *long_options = (struct option *) &bb_null_long_options;
#endif
	unsigned trigger;
	char **pargv = NULL;
	int min_arg = 0;
	int max_arg = -1;

#define SHOW_USAGE_IF_ERROR     1
#define ALL_ARGV_IS_OPTS        2
#define FIRST_ARGV_IS_OPT       4
#define FREE_FIRST_ARGV_IS_OPT  8
	int spec_flgs = 0;

	argc = 0;
	while (argv[argc])
		argc++;

	va_start(p, applet_opts);

	c = 0;
	on_off = complementary;
	memset(on_off, 0, sizeof(complementary));

	/* skip GNU extension */
	s = (const unsigned char *)applet_opts;
	if (*s == '+' || *s == '-')
		s++;
	while (*s) {
		if (c >= 32) break;
		on_off->opt = *s;
		on_off->switch_on = (1 << c);
		if (*++s == ':') {
			on_off->optarg = va_arg(p, void **);
			while (*++s == ':') /* skip */;
		}
		on_off++;
		c++;
	}

#if ENABLE_GETOPT_LONG
	if (applet_long_options) {
		const char *optstr;
		unsigned i, count;

		count = 1;
		optstr = applet_long_options;
		while (optstr[0]) {
			optstr += strlen(optstr) + 3; /* skip NUL, has_arg, val */
			count++;
		}
		/* count == no. of longopts + 1 */
		long_options = alloca(count * sizeof(*long_options));
		memset(long_options, 0, count * sizeof(*long_options));
		i = 0;
		optstr = applet_long_options;
		while (--count) {
			long_options[i].name = optstr;
			optstr += strlen(optstr) + 1;
			long_options[i].has_arg = (unsigned char)(*optstr++);
			/* long_options[i].flag = NULL; */
			long_options[i].val = (unsigned char)(*optstr++);
			i++;
		}
		for (l_o = long_options; l_o->name; l_o++) {
			if (l_o->flag)
				continue;
			for (on_off = complementary; on_off->opt != 0; on_off++)
				if (on_off->opt == l_o->val)
					goto next_long;
			if (c >= 32) break;
			on_off->opt = l_o->val;
			on_off->switch_on = (1 << c);
			if (l_o->has_arg != no_argument)
				on_off->optarg = va_arg(p, void **);
			c++;
 next_long: ;
		}
	}
#endif /* ENABLE_GETOPT_LONG */
	for (s = (const unsigned char *)opt_complementary; s && *s; s++) {
		t_complementary *pair;
		unsigned *pair_switch;

		if (*s == ':')
			continue;
		c = s[1];
		if (*s == '?') {
			if (c < '0' || c > '9') {
				spec_flgs |= SHOW_USAGE_IF_ERROR;
			} else {
				max_arg = c - '0';
				s++;
			}
			continue;
		}
		if (*s == '-') {
			if (c < '0' || c > '9') {
				if (c == '-') {
					spec_flgs |= FIRST_ARGV_IS_OPT;
					s++;
				} else
					spec_flgs |= ALL_ARGV_IS_OPTS;
			} else {
				min_arg = c - '0';
				s++;
			}
			continue;
		}
		if (*s == '=') {
			min_arg = max_arg = c - '0';
			s++;
			continue;
		}
		for (on_off = complementary; on_off->opt; on_off++)
			if (on_off->opt == *s)
				break;
		if (c == ':' && s[2] == ':') {
			on_off->list_flg++;
			continue;
		}
		if (c == ':' || c == '\0') {
			requires |= on_off->switch_on;
			continue;
		}
		if (c == '-' && (s[2] == ':' || s[2] == '\0')) {
			flags |= on_off->switch_on;
			on_off->incongruously |= on_off->switch_on;
			s++;
			continue;
		}
		if (c == *s) {
			on_off->counter = va_arg(p, int *);
			s++;
		}
		pair = on_off;
		pair_switch = &(pair->switch_on);
		for (s++; *s && *s != ':'; s++) {
			if (*s == '?') {
				pair_switch = &(pair->requires);
			} else if (*s == '-') {
				if (pair_switch == &(pair->switch_off))
					pair_switch = &(pair->incongruously);
				else
					pair_switch = &(pair->switch_off);
			} else {
				for (on_off = complementary; on_off->opt; on_off++)
					if (on_off->opt == *s) {
						*pair_switch |= on_off->switch_on;
						break;
					}
			}
		}
		s--;
	}
	va_end(p);

	if (spec_flgs & FIRST_ARGV_IS_OPT) {
		if (argv[1] && argv[1][0] != '-' && argv[1][0] != '\0') {
			argv[1] = xasprintf("-%s", argv[1]);
			if (ENABLE_FEATURE_CLEAN_UP)
				spec_flgs |= FREE_FIRST_ARGV_IS_OPT;
		}
	}

	/* In case getopt32 was already called, reinit some state */
	optind = 1;
	/* optarg = NULL; opterr = 0; optopt = 0; ?? */

	/* Note: just "getopt() <= 0" will not work good for
	 * "fake" short options, like this one:
	 * wget $'-\203' "Test: test" http://kernel.org/
	 * (supposed to act as --header, but doesn't) */
#if ENABLE_GETOPT_LONG
	while ((c = getopt_long(argc, argv, applet_opts,
			long_options, NULL)) != -1) {
#else
	while ((c = getopt(argc, argv, applet_opts)) != -1) {
#endif
		c &= 0xff; /* fight libc's sign extends */
 loop_arg_is_opt:
		for (on_off = complementary; on_off->opt != c; on_off++) {
			/* c==0 if long opt have non NULL flag */
			if (on_off->opt == 0 && c != 0)
				bb_show_usage();
		}
		if (flags & on_off->incongruously)
			bb_show_usage();
		trigger = on_off->switch_on & on_off->switch_off;
		flags &= ~(on_off->switch_off ^ trigger);
		flags |= on_off->switch_on ^ trigger;
		flags ^= trigger;
		if (on_off->counter)
			(*(on_off->counter))++;
		if (on_off->list_flg) {
			llist_add_to_end((llist_t **)(on_off->optarg), optarg);
		} else if (on_off->optarg) {
			*(char **)(on_off->optarg) = optarg;
		}
		if (pargv != NULL)
			break;
	}

	if (spec_flgs & ALL_ARGV_IS_OPTS) {
		/* process argv is option, for example "ps" applet */
		if (pargv == NULL)
			pargv = argv + optind;
		while (*pargv) {
			c = **pargv;
			if (c == '\0') {
				pargv++;
			} else {
				(*pargv)++;
				goto loop_arg_is_opt;
			}
		}
	}

#if (ENABLE_AR || ENABLE_TAR) && ENABLE_FEATURE_CLEAN_UP
	if (spec_flgs & FREE_FIRST_ARGV_IS_OPT)
		free(argv[1]);
#endif
	/* check depending requires for given options */
	for (on_off = complementary; on_off->opt; on_off++) {
		if (on_off->requires && (flags & on_off->switch_on) &&
					(flags & on_off->requires) == 0)
			bb_show_usage();
	}
	if (requires && (flags & requires) == 0)
		bb_show_usage();
	argc -= optind;
	if (argc < min_arg || (max_arg >= 0 && argc > max_arg))
		bb_show_usage();

	option_mask32 = flags;
	return flags;
}
