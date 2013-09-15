/**
 * \addtogroup param_config
 */
/*@{*/

/***************************************************
 * File name: param_config_api.h
 *
 * Copyright (C) 2011, Broadcom Corporation
 * All Rights Reserved.
 * 
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 *
 * First written on 21/09/2006 by I. Barrera.
 *
 ***************************************************/
/** \file param_config_api.h
 *
 * \brief This is the exported API of param_config.
 *
 * $Id: param_config_api.h 300292 2011-12-02 10:57:27Z $
 **************************************************/


/* FILE-CSTYLED */

#ifndef PARAM_CONFIG_DATA_API_H_
#define PARAM_CONFIG_DATA_API_H_



/***************************************************
 *                 Public Constants Section
 ***************************************************/
 /** \brief NVM paramconfig version file offset */
#define PARAM_CONFIG_VERSION_OFFSET    (2)
 /** \brief NVM paramconfig version size */
#define PARAM_CONFIG_VERSION_SIZE      (2)
 
 /** \brief Value when error at retrieving. */
#define PARAM_CONFIG_NO_VALUE (0xffffffff)
#define PARAM_CONFIG_REVISION (26663)

/** \brief Number of 8-bit Configuration Parameters */
#define PARAM_CONFIG_N_DEFAULT_ARRAY_8 (1709)		
/** \brief Number of 16-bit Configuration Parameters */
#define PARAM_CONFIG_N_DEFAULT_ARRAY_16 (17)		
/** \brief Number of 32-bit Configuration Parameters */
#define PARAM_CONFIG_N_DEFAULT_ARRAY_32 (5)		
/***************************************************
 *                 Public Typedefs Section
 ***************************************************/

/** \brief These are the descriptors of the configuration parameters. */
typedef enum
{
  PARAM_CONFIG_BRIDGEFW_IGMP_SNOOPING = 0,		/**< IGMP snooping switch - 0-off, 1-on */
  PARAM_CONFIG_BRIDGEFW_LOOP_FILTER,		/**< Loop filter switch - 0-off, 1-on */
  PARAM_CONFIG_PLSCHED_PM_BACKOFF_PERIOD_PLC_MIN,		/**< Minimum plc traffic to prevent idle mode switch (period interval) */
  PARAM_CONFIG_PLSCHED_PM_BACKOFF_REGION_MII_MIN,		/**< Minimum mii traffic to switch backoff (region interval) */
  PARAM_CONFIG_MDIO_SLAVE_CONF,		/**< MDIO Slaves configuration */
  PARAM_CONFIG_WATCHDOG1_TIMER,		/**< Watchdog1 timer overrun period */
  PARAM_CONFIG_WATCHDOG2_TIMER,		/**< Watchdog2 timer overrun period */
  PARAM_CONFIG_WATCHDOG_CTRL,		/**< Watchdog control parameters */
  PARAM_CONFIG_WATCHDOG_LEVEL,		/**< Watchdog level */
  PARAM_CONFIG_WATCHDOG_REFRESH,		/**< Watchdog refresh */
  PARAM_CONFIG_PLCONFIG_MANUFACTURER_NID,		/**< Manufacturer Network ID */
  PARAM_CONFIG_PLCONFIG_MANUFACTURER_NMK,		/**< Manufacturer Network Membership Key(npw: HompePlugAV) */
  PARAM_CONFIG_PLCONFIG_MANUFACTURER_DAK_1,		/**< Manufacturer Device Access Key 1 */
  PARAM_CONFIG_PLCONFIG_MANUFACTURER_DAK_2,		/**< Manufacturer Device Access Key 2 */
  PARAM_CONFIG_PLCONFIG_MANUFACTURER_DAK_3,		/**< Manufacturer Device Access Key 3 */
  PARAM_CONFIG_PLCONFIG_MANUFACTURER_DAK_4,		/**< Manufacturer Device Access Key 4 */
  PARAM_CONFIG_MAC_ADDRESS_PCI,		/**< MAC address in case the configuration is taken from PCI */
  PARAM_CONFIG_DSP_LB_TDMULT_FACTOR,		/**< Multiplication linear factor for time domain signal */
  PARAM_CONFIG_DSP_HB_CP_LEN0,		/**< Configuration of the highband cyclic-prefix */
  PARAM_CONFIG_DSP_HB_FFT_FIRST_CARRIER,		/**< Index of the first FFT carrier */
  PARAM_CONFIG_TONEMAPBOSS_LB_AMAP,		/**< Amplitude map information for the low-band */
  PARAM_CONFIG_TONEMAPBOSS_HB_AMAP,		/**< Amplitude mask information for high-band */
  PARAM_CONFIG_LOW_TEMPERATURE_EDGE,		/**< Low temperature edge for threm iface */
  PARAM_CONFIG_HIGH_TEMPERATURE_EDGE,		/**< High temperature edge for threm iface */
  PARAM_CONFIG_MAC_ADDRESS_PCI_HOST,		/**< PCI host MAC address */
  PARAM_CONFIG_PLCONFIG_PREV_STATE,		/**< State of the STA before last power-down */
  PARAM_CONFIG_IGC_HP_THRESHOLD,		/**< Home Plug IGC threshold to detect short circuit */
  PARAM_CONFIG_IGC_GB_THRESHOLD,		/**< Gigabit IGC threshold to detect short circuit */
  PARAM_CONFIG_IGC_HP_AVRG_CNST,		/**< Home Plug IGC constant to calculate moving average */
  PARAM_CONFIG_IGC_GB_AVRG_CNST,		/**< Gigabit IGC constant to calculate moving average */
  PARAM_CONFIG_IGC_HP_DACGAIN_LUT,		/**< Home Plug IGC look up table to get DAC gain */
  PARAM_CONFIG_IGC_GB_DACGAIN_LUT,		/**< Gigabit IGC look up table to get DAC gain */
  PARAM_CONFIG_MMPKTS_SNIFFER_IF,		/**< Sniffer Interface */
  PARAM_CONFIG_LEDS_CONFIG_MODE,		/**< LEDS configuration mode */
  PARAM_CONFIG_TONEMAPFILTER_MAX_TIME_WITHOUT_BLIND,		/**< Maximum time in multiples of 40ns between the reception of Blind SNR measurements */
  PARAM_CONFIG_TONEMAPFILTER_DUMP_INFO,		/**< Measurement dump information configured for each band of operation */
  PARAM_CONFIG_TONEMAPFILTER_SOUND_COMPLETE_DELAY,		/**< Number of Sound SNR measurements pending when the SoundCompleteGet API is called */
  PARAM_CONFIG_TONEMAPFILTER_INIT_CHEST_SOUND_COUNT,		/**< Number of Sound SNR measurements during the Initial Channel Estimation process */
  PARAM_CONFIG_TONEMAPFILTER_FEC_RATE,		/**< FEC rate information used to fill the Tone Map Data for each band of operation */
  PARAM_CONFIG_TONEMAPFILTER_CP_CODE,		/**< Cyclic prefix information used to fill the Tone Map Data for each band of operation */
  PARAM_CONFIG_SNRFILTER_KF_ERROR_TH,		/**< Kalman Filter prediction error threshold */
  PARAM_CONFIG_SNRFILTER_KF_RESET_MSE,		/**< Kalman Filter MSE reset value */
  PARAM_CONFIG_SNRFILTER_KF_RESET_MVAR,		/**< Kalman Filter measurement model variance reset value */
  PARAM_CONFIG_SNRFILTER_KF_RESET_DVAR,		/**< Kalman Filter dynamical model variance reset value */
  PARAM_CONFIG_PPMTRACKING_HF_MOD_INDEX_TH,		/**< Maximum modulation allowed in the HF range to activate the PPM tracking enable */
  PARAM_CONFIG_PPMTRACKING_REAL_CARRIER_TH,		/**< Index of the first real carrier in the HF range */
  PARAM_CONFIG_LLRWEIGHTING_LB_SNR_TH,		/**< LLRWeighting SNR thresholds for LB */
  PARAM_CONFIG_LLRWEIGHTING_HB_SNR_TH,		/**< LLRWeighting SNR thresholds for HB */
  PARAM_CONFIG_LLRWEIGHTING_LB_WEIGHTS,		/**< LLRWeighting LLR weights for LB */
  PARAM_CONFIG_LLRWEIGHTING_HB_WEIGHTS,		/**< LLRWeighting LLR weights for HB */
  PARAM_CONFIG_CSMONITOR_BPS_CHANGE_TH,		/**< BPS change thresholds for each band of operation */
  PARAM_CONFIG_CSMONITOR_RESET_COUNTER_TH,		/**< Kalman Filter reset counter thresholds for each band of operation */
  PARAM_CONFIG_BITLOADING_LB_SNR_OFFSET_GROUP_INFO,		/**< SNR offset information to apply in the LB related to groups */
  PARAM_CONFIG_BITLOADING_HB_SNR_OFFSET_GROUP_INFO,		/**< SNR offset information to apply in the HB related to groups */
  PARAM_CONFIG_BITLOADING_LB_SNR_OFFSET_CARRIER_INFO,		/**< SNR offset information to apply in the LB related to carriers */
  PARAM_CONFIG_BITLOADING_HB_SNR_OFFSET_CARRIER_INFO,		/**< SNR offset information to apply in the HB related to carriers */
  PARAM_CONFIG_BITLOADING_LB_TABLE_INFO,		/**< Bitloading table for LB */
  PARAM_CONFIG_BITLOADING_HB_TABLE_INFO,		/**< Bitloading table for HB */
  PARAM_CONFIG_BITLOADING_LB_SNR_TH,		/**< Bitloading hystheresis SNR thresholds for LB bitloading table */
  PARAM_CONFIG_BITLOADING_HB_SNR_TH,		/**< Bitloading hystheresis SNR thresholds for HB bitloading table */
  PARAM_CONFIG_BITLOADING_LB_ROBO_BPS_TH_RATIO,		/**< ROBO BPS threshold for LB */
  PARAM_CONFIG_BITLOADING_HB_ROBO_BPS_TH_RATIO,		/**< ROBO BPS thresholds for HB */
  PARAM_CONFIG_TONEMAP_SECS_EXPIRE_TM,		/**< Seconds before sending sound refresh 0 to disable */
  PARAM_CONFIG_TONEMAP_QUALITY_RATIO_TO_RESET_TM,		/**< Ratio bad sent packets reset a TM 0 to disable */
  PARAM_CONFIG_TONEMAP_MAX_CONSECUTIVE_SOUNDS,		/**< Maximum consecutive sounds for a TM 0 to disable */
  PARAM_CONFIG_STATINFO_MANUFACTURER_STA_HFID,		/**< Manufacturer  HFID of the STA */
  PARAM_CONFIG_STATINFO_MANUFACTURER_AVLN_HFID,		/**< Manufacturer HFID of the Network */
  PARAM_CONFIG_PROD_TEST,		/**< Value != 0 indicates we are running the production test */
  PARAM_CONFIG_BUTTON_FACTORYRESET_INTERVAL,		/**< Amount of time button must be pushed to force factory reset ms */
  PARAM_CONFIG_BUTTON_NETCONFIG_INTERVAL,		/**< Switching time interval between button and led ms */
  PARAM_CONFIG_CONNECTIONMGR_CONNECTION_DATA,		/**< Classifier rules and its associated CSPEC */
  PARAM_CONFIG_SNRCORRECTION_HB_SOUND_CORRECTION_NGROUPS,		/**< Number of groups to define the HB Sound correction */
  PARAM_CONFIG_SNRCORRECTION_HB_SOUND_CORRECTION_INFO,		/**< Number of groups to define the HB Sound correction */
  PARAM_CONFIG_PLCONFIG_USER_NID,		/**< User Network ID */
  PARAM_CONFIG_PLCONFIG_USER_NMK,		/**< User Network Membership Key */
  PARAM_CONFIG_STATINFO_USER_STA_HFID,		/**< User HFID of the STA */
  PARAM_CONFIG_STATINFO_USER_AVLN_HFID,		/**< User HFID of the Network */
  PARAM_CONFIG_TONEMAPFILTER_BLER_THRESHOLD,		/**< BLER threshold for LB and HB in parts per 1000 */
  PARAM_CONFIG_TONEMAPFILTER_SEGMENTS_THRESHOLD,		/**< Number of segments to receive to compute valid BLER statistics */
  PARAM_CONFIG_BITLOADING_LB_SNR_TH_BOOST,		/**< Bitloading hystheresis SNR thresholds for LB bitloading table boost */
  PARAM_CONFIG_SNRFILTER_KF_ERROR_TH_ACQUISITION,		/**< Kalman Filter prediction error threshold in acquisition mode */
  PARAM_CONFIG_DAC_GB_GAIN,		/**< DAC Gain for Mediaxtream  */
  PARAM_CONFIG_REF_DESIGN,		/**< Reference design PHONE_LINE= 0, RF_1=1, RF_2=2, BBCOAX=3, PLC=4 */
  PARAM_CONFIG_HB_ENABLED,		/**< HB enabled flag */
  PARAM_CONFIG_DAC_HP_GAIN,		/**< bit 6:DAC_REXT_SEL, bits 5:0 dac_hp_gain  */
  PARAM_CONFIG_TONEMAPFILTER_INTERVAL_BLER_THRESHOLD,		/**< BLER threshold for LB and HB in parts per 1000 for intervals */
  PARAM_CONFIG_TONEMAPFILTER_INTERVAL_SEGMENTS_THRESHOLD,		/**< Number of segments to receive to compute valid BLER statistics for intervals */
  PARAM_CONFIG_HB_TX_GAIN,		/**< Digital Transmission Gain in MX band */
  PARAM_CONFIG_HB_WN_SHIFT_SAMPLES,		/**< Mx windowing configuration */
  PARAM_CONFIG_HB_MOD_PREAM_BOOST,		/**< Gain applied to preamble, relative to mean signal level in MX */
  PARAM_CONFIG_DSP_LB_TDMULT_FACTOR_BOOST,		/**< Boosted value of the multiplication linear factor for time domain signal */
  PARAM_CONFIG_LB_TX_PREEQ_EN,		/**< LB Tx preequalisation enable */
  PARAM_CONFIG_POWERMGR_STANDBY_TIMEOUT,		/**< Standby timeout */
  PARAM_CONFIG_IGMPSNOOP_MRT_DELTA,		/**< IGMP Maximum Response Time extra delay */
  PARAM_CONFIG_HARDWARE_CODE,		/**< Encodes HW DB */
  PARAM_CONFIG_BYPASS_START,		/**< Start Bypass Enabled */
  PARAM_CONFIG_CLASSIFIER_RULES_SRC_DST_MAC,		/**< Number of classifying rules for  src or dst MAC address */
  PARAM_CONFIG_CLASSIFIER_RULES_VLAN,		/**< Number of classifying rules for VLAN tag or prio */
  PARAM_CONFIG_CLASSIFIER_RULES_IP_TOS_PROT,		/**< Number of classifying rules for IP ToS prot or IPV6 class */
  PARAM_CONFIG_CLASSIFIER_RULES_IP_DST_SRC_ADDR,		/**< Number of classifying rules for IP dest or src address */
  PARAM_CONFIG_CLASSIFIER_RULES_UDP_TCP_PORT,		/**< Number of classifying rules for  UDP or TCP port */
  PARAM_CONFIG_IGMPSNOOP_DUMP_UNKNOWN,		/**< IGMPSoop dump unknown  */
  PARAM_CONFIG_IGMPSNOOP_ROUTER_ATTACHED_INTERFACES,		/**< Router-Attached Interfaces(MII1/MII0) */
  PARAM_CONFIG_BONDING_ENABLED,		/**< Enable Bonding in HPAV/MX channel  */
  PARAM_CONFIG_HB_TX_GAIN_REDUCED,		/**< power in the MX band when using reduced power */
  PARAM_CONFIG_CLASSIFIER_RULES_MATCHING_ORDER,		/**< Matching order of classifying rules */
  PARAM_CONFIG_MDU_NODE_TYPE,		/**< MDU node type slave or master */
  PARAM_CONFIG_POWERMGR_STANDBY_INPUT,		/**< Standby mode module input */
  PARAM_CONFIG_PLSCHED_PM_TXRX_STATS_INTERVAL,		/**< Interval between traffic stats adq (ms) */
  PARAM_CONFIG_PLSCHED_PM_HB_MII_MIN,		/**< Minimum hb mii traffic to prevent idle mode switch (stats interval) */
  PARAM_CONFIG_PLSCHED_PM_HB_PLC_MIN,		/**< Minimum hb plc traffic to prevent idle mode switch (stats interval) */
  PARAM_CONFIG_PLSCHED_PM_BACKOFF_PERIOD_MII_MIN,		/**< Minimum mii traffic to prevent idle mode switch (period nterval) */
  PARAM_CONFIG_UART1,		/**< UART 1 parameters */
  PARAM_CONFIG_UART2,		/**< UART 2 parameters */
  PARAM_CONFIG_PLSCHED_PM_BACKOFF_REGION_PLC_MIN,		/**< Minimum plc traffic to switch backoff (region interval) */
  PARAM_CONFIG_PLSCHED_PM_BACKOFF_MIN,		/**< Minimum backoff interval (us) */
  PARAM_CONFIG_BRIDGEFWD_WIFI_BONDING_THRESHOLD,		/**< WIFI link quality threshold  for WIFI/PLC bonding (in dbm / default value -65dbm) */
  PARAM_CONFIG_BRIDGEFWD_PLC_BONDING_THRESHOLD,		/**< PLC link quality threshold for WIFI/PLC bonding (in Mbps / default value 60Mbps) */
  PARAM_CONFIG_RALINK_OP_MODE,		/**< WIFI operation mode of the WIFI IC (Access Point = 0 / End Point = 1) */
  PARAM_CONFIG_WIFI_ROOT,		/**< root or slave wifi node */
  PARAM_CONFIG_DUMMY_FOR_ROM_32,		/**< dummy for ROM  */
  PARAM_CONFIG_DUMMY_FOR_ROM_33,		/**< dummy for ROM  */
  PARAM_CONFIG_DUMMY_FOR_ROM_34,		/**< dummy for ROM  */
  PARAM_CONFIG_DUMMY_FOR_ROM_35,		/**< dummy for ROM  */
  PARAM_CONFIG_DUMMY_FOR_ROM_36,		/**< dummy for ROM  */

  PARAM_CONFIG_N_PARAMETERS		/**< Number of Configuration Parameters */
}
t_param_config_param;

/** \brief Structure for the parameter information. */
typedef struct {
  t_uint8 attr;                    /**< Attributes of the parameter */
  t_uint16 length;                 /**< Length of array, =1 if scalar */
  t_uint16 nvm_offset;             /**< NVM address offset */
  t_gig_error (*callback)(t_param_config_param);  /**< Callback function to be called when parameter is externally modified */
} t_param_config_info;

/***************************************************
 *                 Public Function Prototypes Section
 ***************************************************/

  t_gig_error ParamConfigInit(void) INIT_THROW;
  t_uint32 ParamConfigIntGetValue(t_param_config_param param);
  t_uint32 ParamConfigGetNValue(t_param_config_param param, t_uint16 index);
  void ParamConfigSetNValue(t_param_config_param param, t_uint16 index, t_uint32 value);
  void ParamConfigFactoryReset(void);
  void ParamConfigFullReset(void);
  void ParamConfigSetArrayValue(t_param_config_param param, t_uint16 index,
    t_uint16 length, t_uint8 *value);
 
  /** Deprecated (ROM) **/
  t_gig_error ParamConfigInitDeprecated(void) INIT_THROW;
  t_uint32 ParamConfigIntGetValueDeprecated(t_param_config_param param);
  t_uint32 ParamConfigGetNValueDeprecated(t_param_config_param param, t_uint16 index);
  void ParamConfigSetValueDeprecated(t_param_config_param, t_uint32 value);
  void ParamConfigSetNValueDeprecated(t_param_config_param param, t_uint16 index, t_uint32 value);
  void ParamConfigResetDeprecated(t_param_config_param param);
  void ParamConfigFactoryResetDeprecated(void);
  void ParamConfigFullResetDeprecated(void);
  
#ifndef _UNIT_TEST_

/** \brief Define macro for ParamConfigGetValue. */
#define ParamConfigGetValue(param) \
  (gighook[GIGHOOK_PARAM_CONFIG_GET_VALUE])((t_uint32)(param), \
  (t_uint32)(0), (t_uint32)(0))

#else

/** \brief Define macro for ParamConfigGetValue. */
#define ParamConfigGetValue(param) ParamConfigIntGetValue(param)

#endif

#define ParamConfigSetValue(param,value) \
  ParamConfigSetNValue(param,0,value)

#else
#error "Header file __FILE__ has already been included!"
#endif

/*@}*/
