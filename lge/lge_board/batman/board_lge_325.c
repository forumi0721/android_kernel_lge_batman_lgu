/* Copyright (c) 2010-2012, Code Aurora Forum. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <linux/kernel.h>
#include <linux/platform_device.h>
#include <linux/gpio.h>
#include <linux/irq.h>
#include <linux/io.h>
#include <linux/msm_ssbi.h>
#include <linux/mfd/pmic8058.h>

#include <linux/leds.h>
#include <linux/pmic8058-othc.h>
#include <linux/mfd/pmic8901.h>
#include <linux/regulator/gpio-regulator.h>
#include <linux/regulator/pmic8901-regulator.h>
#include <linux/bootmem.h>
#include <linux/msm_adc.h>
#include <linux/m_adcproc.h>
#include <linux/mfd/marimba.h>
#include <linux/msm-charger.h>
#include <linux/i2c.h>
#include <linux/i2c/sx150x.h>
#include <linux/spi/spi.h>
#if defined (CONFIG_LGE_TOUCH_SYNAPTICS_325) || defined (CONFIG_LGE_TOUCHSCREEN_MELFAS)
#include <linux/input/lge_touch_core_325.h>
#include "melfas_ts.h"
#else
#include <linux/input/tdisc_shinetsu.h>
#include <linux/input/cy8c_ts.h>
#include <linux/cyttsp.h>
#endif
#include <linux/i2c/isa1200.h>
#include <linux/dma-mapping.h>
#include <linux/i2c/bq27520.h>

#ifdef CONFIG_ANDROID_PMEM
#include <linux/android_pmem.h>
#endif

#include <linux/clk.h>
#ifdef CONFIG_SND_SOC_WM8903
#include <sound/wm8903.h>
#endif
#include <asm/mach-types.h>
#include <asm/mach/arch.h>
#include <asm/setup.h>
#include <asm/hardware/gic.h>

#include <mach/dma.h>
#include <mach/board.h>
#include <mach/irqs.h>
#include <mach/msm_spi.h>
#include <mach/msm_serial_hs.h>
#include <mach/msm_serial_hs_lite.h>
#include <mach/msm_iomap.h>
#include <mach/msm_memtypes.h>
#include <asm/mach/mmc.h>
#include <mach/msm_battery.h>
#include <mach/msm_hsusb.h>
#include <mach/gpiomux.h>
#ifdef CONFIG_MSM_DSPS
#include <mach/msm_dsps.h>
#endif
#include <mach/msm_xo.h>
#include <mach/msm_bus_board.h>
#include <mach/socinfo.h>
#include <linux/i2c/isl9519.h>
#ifdef CONFIG_USB_G_ANDROID
#include <linux/usb/android.h>
#include <mach/usbdiag.h>
#endif
#include <linux/regulator/consumer.h>
#include <linux/regulator/machine.h>
#include <mach/sdio_al.h>
#include <mach/rpm.h>
#include <mach/rpm-regulator.h>
#include <mach/restart.h>
#include <mach/board-msm8660.h>

#include "devices.h"
#include "devices_lge_325.h"
#include <mach/cpuidle.h>
#include "pm.h"
#include "mpm.h"
#include "spm.h"
#include "rpm_log.h"
#include "timer.h"
#include "gpiomux_lge_325.h"
#include "rpm_stats.h"
#include "peripheral-loader.h"
#include <linux/platform_data/qcom_crypto_device.h>
#include "rpm_resources.h"
#include "acpuclock.h"
#include "pm-boot.h"

#include <linux/ion.h>
#include <mach/ion.h>

#include "board_lge.h"
#include "board_lge_325.h"

#define MSM_SHARED_RAM_PHYS 0x40000000

#ifdef CONFIG_LGE_FUEL_GAUGE
#include <linux/max17047_battery.h>
#endif

#ifdef CONFIG_LGE_WIRELESS_CHARGER_MAX8971
#include <linux/max8971-charger.h>
#endif

#ifdef CONFIG_BATTERY_325
#include <linux/bq24160-charger.h>
#endif

#ifdef CONFIG_LGE_WIRELESS_CHARGER_BQ24160
#include <linux/bq24160-charger.h>
#endif

#if 0 /* moved following macros into devices_lge_325_dcm.h */
/* Macros assume PMIC GPIOs start at 0 */
#define PM8058_GPIO_BASE			NR_MSM_GPIOS
#define PM8058_GPIO_PM_TO_SYS(pm_gpio)		(pm_gpio + PM8058_GPIO_BASE)
#define PM8058_GPIO_SYS_TO_PM(sys_gpio)		(sys_gpio - PM8058_GPIO_BASE)
#define PM8058_MPP_BASE			(PM8058_GPIO_BASE + PM8058_GPIOS)
#define PM8058_MPP_PM_TO_SYS(pm_gpio)		(pm_gpio + PM8058_MPP_BASE)
#define PM8058_MPP_SYS_TO_PM(sys_gpio)		(sys_gpio - PM8058_MPP_BASE)
#define PM8058_IRQ_BASE				(NR_MSM_IRQS + NR_GPIO_IRQS)

#define PM8901_GPIO_BASE			(PM8058_GPIO_BASE + \
						PM8058_GPIOS + PM8058_MPPS)
#define PM8901_GPIO_PM_TO_SYS(pm_gpio)		(pm_gpio + PM8901_GPIO_BASE)
#define PM8901_GPIO_SYS_TO_PM(sys_gpio)		(sys_gpio - PM8901_GPIO_BASE)
#define PM8901_IRQ_BASE				(PM8058_IRQ_BASE + \
						NR_PMIC8058_IRQS)
#endif

#define MDM2AP_SYNC 129

#define GPIO_ETHERNET_RESET_N_DRAGON	30
#define DSPS_PIL_GENERIC_NAME		"dsps"
#define DSPS_PIL_FLUID_NAME		"dsps_fluid"

#ifdef CONFIG_ION_MSM
static struct platform_device ion_dev;
#endif

enum {
	GPIO_EXPANDER_IRQ_BASE  = PM8901_IRQ_BASE + NR_PMIC8901_IRQS,
	GPIO_EXPANDER_GPIO_BASE = PM8901_MPP_BASE + PM8901_MPPS,
	/* CORE expander */
	GPIO_CORE_EXPANDER_BASE = GPIO_EXPANDER_GPIO_BASE,
	GPIO_CLASS_D1_EN        = GPIO_CORE_EXPANDER_BASE,
	GPIO_WLAN_DEEP_SLEEP_N,
	GPIO_LVDS_SHUTDOWN_N,
	GPIO_DISP_RESX_N        = GPIO_LVDS_SHUTDOWN_N,
	GPIO_MS_SYS_RESET_N,
	GPIO_CAP_TS_RESOUT_N,
	GPIO_CAP_GAUGE_BI_TOUT,
	GPIO_ETHERNET_PME,
	GPIO_EXT_GPS_LNA_EN,
	GPIO_MSM_WAKES_BT,
	GPIO_ETHERNET_RESET_N,
	GPIO_HEADSET_DET_N,
	GPIO_USB_UICC_EN,
	GPIO_BACKLIGHT_EN,
	GPIO_EXT_CAMIF_PWR_EN,
	GPIO_BATT_GAUGE_INT_N,
	GPIO_BATT_GAUGE_EN,
	/* DOCKING expander */
	GPIO_DOCKING_EXPANDER_BASE = GPIO_EXPANDER_GPIO_BASE + 16,
	GPIO_MIPI_DSI_RST_N        = GPIO_DOCKING_EXPANDER_BASE,
	GPIO_AUX_JTAG_DET_N,
	GPIO_DONGLE_DET_N,
	GPIO_SVIDEO_LOAD_DET,
	GPIO_SVID_AMP_SHUTDOWN1_N,
	GPIO_SVID_AMP_SHUTDOWN0_N,
	GPIO_SDC_WP,
	GPIO_IRDA_PWDN,
	GPIO_IRDA_RESET_N,
	GPIO_DONGLE_GPIO0,
	GPIO_DONGLE_GPIO1,
	GPIO_DONGLE_GPIO2,
	GPIO_DONGLE_GPIO3,
	GPIO_DONGLE_PWR_EN,
	GPIO_EMMC_RESET_N,
	GPIO_TP_EXP2_IO15,
	/* SURF expander */
	GPIO_SURF_EXPANDER_BASE = GPIO_EXPANDER_GPIO_BASE + (16 * 2),
	GPIO_SD_CARD_DET_1      = GPIO_SURF_EXPANDER_BASE,
	GPIO_SD_CARD_DET_2,
	GPIO_SD_CARD_DET_4,
	GPIO_SD_CARD_DET_5,
	GPIO_UIM3_RST,
	GPIO_SURF_EXPANDER_IO5,
	GPIO_SURF_EXPANDER_IO6,
	GPIO_ADC_I2C_EN,
	GPIO_SURF_EXPANDER_IO8,
	GPIO_SURF_EXPANDER_IO9,
	GPIO_SURF_EXPANDER_IO10,
	GPIO_SURF_EXPANDER_IO11,
	GPIO_SURF_EXPANDER_IO12,
	GPIO_SURF_EXPANDER_IO13,
	GPIO_SURF_EXPANDER_IO14,
	GPIO_SURF_EXPANDER_IO15,
	/* LEFT KB IO expander */
	GPIO_LEFT_KB_EXPANDER_BASE = GPIO_EXPANDER_GPIO_BASE + (16 * 3),
	GPIO_LEFT_LED_1            = GPIO_LEFT_KB_EXPANDER_BASE,
	GPIO_LEFT_LED_2,
	GPIO_LEFT_LED_3,
	GPIO_LEFT_LED_WLAN,
	GPIO_JOYSTICK_EN,
	GPIO_CAP_TS_SLEEP,
	GPIO_LEFT_KB_IO6,
	GPIO_LEFT_LED_5,
	/* RIGHT KB IO expander */
	GPIO_RIGHT_KB_EXPANDER_BASE = GPIO_EXPANDER_GPIO_BASE + (16 * 3) + 8,
	GPIO_RIGHT_LED_1            = GPIO_RIGHT_KB_EXPANDER_BASE,
	GPIO_RIGHT_LED_2,
	GPIO_RIGHT_LED_3,
	GPIO_RIGHT_LED_BT,
	GPIO_WEB_CAMIF_STANDBY,
	GPIO_COMPASS_RST_N,
	GPIO_WEB_CAMIF_RESET_N,
	GPIO_RIGHT_LED_5,
	GPIO_R_ALTIMETER_RESET_N,
	/* FLUID S IO expander */
	GPIO_SOUTH_EXPANDER_BASE,
	GPIO_MIC2_ANCR_SEL = GPIO_SOUTH_EXPANDER_BASE,
	GPIO_MIC1_ANCL_SEL,
	GPIO_HS_MIC4_SEL,
	GPIO_FML_MIC3_SEL,
	GPIO_FMR_MIC5_SEL,
	GPIO_TS_SLEEP,
	GPIO_HAP_SHIFT_LVL_OE,
	GPIO_HS_SW_DIR,
	/* FLUID N IO expander */
	GPIO_NORTH_EXPANDER_BASE,
	GPIO_EPM_3_3V_EN = GPIO_NORTH_EXPANDER_BASE,
	GPIO_EPM_5V_BOOST_EN,
	GPIO_AUX_CAM_2P7_EN,
	GPIO_LED_FLASH_EN,
	GPIO_LED1_GREEN_N,
	GPIO_LED2_RED_N,
	GPIO_FRONT_CAM_RESET_N,
	GPIO_EPM_LVLSFT_EN,
	GPIO_N_ALTIMETER_RESET_N,
	/* EPM expander */
	GPIO_EPM_EXPANDER_BASE,
	GPIO_PWR_MON_START = GPIO_EPM_EXPANDER_BASE,
	GPIO_PWR_MON_RESET_N,
	GPIO_ADC1_PWDN_N,
	GPIO_ADC2_PWDN_N,
	GPIO_EPM_EXPANDER_IO4,
	GPIO_ADC1_MUX_SPI_INT_N_3_3V,
	GPIO_ADC2_MUX_SPI_INT_N,
	GPIO_EPM_EXPANDER_IO7,
	GPIO_PWR_MON_ENABLE,
	GPIO_EPM_SPI_ADC1_CS_N,
	GPIO_EPM_SPI_ADC2_CS_N,
	GPIO_EPM_EXPANDER_IO11,
	GPIO_EPM_EXPANDER_IO12,
	GPIO_EPM_EXPANDER_IO13,
	GPIO_EPM_EXPANDER_IO14,
	GPIO_EPM_EXPANDER_IO15,
};

struct pm8xxx_mpp_init_info {
	unsigned			mpp;
	struct pm8xxx_mpp_config_data	config;
};

#define PM8058_MPP_INIT(_mpp, _type, _level, _control) \
{ \
	.mpp	= PM8058_MPP_PM_TO_SYS(_mpp), \
	.config	= { \
		.type		= PM8XXX_MPP_TYPE_##_type, \
		.level		= _level, \
		.control	= PM8XXX_MPP_##_control, \
	} \
}

#define PM8901_MPP_INIT(_mpp, _type, _level, _control) \
{ \
	.mpp	= PM8901_MPP_PM_TO_SYS(_mpp), \
	.config	= { \
		.type		= PM8XXX_MPP_TYPE_##_type, \
		.level		= _level, \
		.control	= PM8XXX_MPP_##_control, \
	} \
}

/*
 * The UI_INTx_N lines are pmic gpio lines which connect i2c
 * gpio expanders to the pm8058.
 */
#define UI_INT1_N 25
#define UI_INT2_N 34
#define UI_INT3_N 14
/*
FM GPIO is GPIO 18 on PMIC 8058.
As the index starts from 0 in the PMIC driver, and hence 17
corresponds to GPIO 18 on PMIC 8058.
*/
#define FM_GPIO 17
//[START]kcal for 325
int g_kcal[6];
static int __init lcd_kcal_setup(char *kcal)
{
    int i;

    for (i=0; i<6; i++) {
        if (kcal[i*2] >= '0' && kcal[i*2] <= '9') {
            g_kcal[i] = (kcal[i*2] - '0') * 16;
        }
        else if (kcal[i*2] >= 'a' && kcal[i*2] <= 'f') {
            g_kcal[i] = ((kcal[i*2] - 'a') + 10) * 16;
        }

        if (kcal[i*2+1] >= '0' && kcal[i*2+1] <= '9') {
            g_kcal[i] += kcal[i*2+1] - '0';
        }
        else if (kcal[i*2+1] >= 'a' && kcal[i*2+1] <= 'f') {
            g_kcal[i] += kcal[i*2+1] - 'a' + 10;
        }
	//g_kcal[i] = 0;
    }
    printk(KERN_INFO "####kcal=%s, r=%x, g=%x, b=%x  sign=%x,%x,%x\n", kcal, g_kcal[0], g_kcal[1], g_kcal[2], g_kcal[3], g_kcal[4], g_kcal[5]);
	return 1;
}
__setup("kcal=", lcd_kcal_setup);
//[END]kcal for 325

#if defined(CONFIG_LGE_SHOW_FB_BOOTLOGO)
unsigned int lge_reset_flag = 0;
unsigned int lge_boot_flag = 0;

static int __init lge_reset_cause_setup(char *rst_flag) {
	if (!strcmp(rst_flag, "rst_none") || !strcmp(rst_flag, "off_but_rst") || !strcmp(rst_flag, "normal"))
		lge_boot_flag = 1;

	return 1;
}
__setup("lge.reset=", lge_reset_cause_setup);

static int __init lge_boot_cause_setup(char *boot_cause) {
	if (!strcmp(boot_cause, "usb_chg") || !strcmp(boot_cause, "wall_chg"))
		lge_reset_flag = 1;

	return 1;
}
__setup("lge.pwron=", lge_boot_cause_setup);
#endif

#ifdef CONFIG_MMC_MSM_SDC2_SUPPORT
static void (*sdc2_status_notify_cb)(int card_present, void *dev_id);
static void *sdc2_status_notify_cb_devid;
#endif

#ifdef CONFIG_MMC_MSM_SDC5_SUPPORT
static void (*sdc5_status_notify_cb)(int card_present, void *dev_id);
static void *sdc5_status_notify_cb_devid;
#endif

static struct msm_spm_platform_data msm_spm_data_v1[] __initdata = {
	[0] = {
		.reg_base_addr = MSM_SAW0_BASE,

#ifdef CONFIG_MSM_AVS_HW
		.reg_init_values[MSM_SPM_REG_SAW_AVS_CTL] = 0x586020FF,
#endif
		.reg_init_values[MSM_SPM_REG_SAW_CFG] = 0x0F,
		.reg_init_values[MSM_SPM_REG_SAW_SPM_CTL] = 0x68,
		.reg_init_values[MSM_SPM_REG_SAW_SPM_SLP_TMR_DLY] = 0xFFFFFFFF,
		.reg_init_values[MSM_SPM_REG_SAW_SPM_WAKE_TMR_DLY] = 0xFFFFFFFF,

		.reg_init_values[MSM_SPM_REG_SAW_SLP_CLK_EN] = 0x01,
		.reg_init_values[MSM_SPM_REG_SAW_SLP_HSFS_PRECLMP_EN] = 0x07,
		.reg_init_values[MSM_SPM_REG_SAW_SLP_HSFS_POSTCLMP_EN] = 0x00,

		.reg_init_values[MSM_SPM_REG_SAW_SLP_CLMP_EN] = 0x01,
		.reg_init_values[MSM_SPM_REG_SAW_SLP_RST_EN] = 0x00,
		.reg_init_values[MSM_SPM_REG_SAW_SPM_MPM_CFG] = 0x00,

		.awake_vlevel = 0x94,
		.retention_vlevel = 0x81,
		.collapse_vlevel = 0x20,
		.retention_mid_vlevel = 0x94,
		.collapse_mid_vlevel = 0x8C,

		.vctl_timeout_us = 50,
	},

	[1] = {
		.reg_base_addr = MSM_SAW1_BASE,

#ifdef CONFIG_MSM_AVS_HW
		.reg_init_values[MSM_SPM_REG_SAW_AVS_CTL] = 0x586020FF,
#endif
		.reg_init_values[MSM_SPM_REG_SAW_CFG] = 0x0F,
		.reg_init_values[MSM_SPM_REG_SAW_SPM_CTL] = 0x68,
		.reg_init_values[MSM_SPM_REG_SAW_SPM_SLP_TMR_DLY] = 0xFFFFFFFF,
		.reg_init_values[MSM_SPM_REG_SAW_SPM_WAKE_TMR_DLY] = 0xFFFFFFFF,

		.reg_init_values[MSM_SPM_REG_SAW_SLP_CLK_EN] = 0x13,
		.reg_init_values[MSM_SPM_REG_SAW_SLP_HSFS_PRECLMP_EN] = 0x07,
		.reg_init_values[MSM_SPM_REG_SAW_SLP_HSFS_POSTCLMP_EN] = 0x00,

		.reg_init_values[MSM_SPM_REG_SAW_SLP_CLMP_EN] = 0x01,
		.reg_init_values[MSM_SPM_REG_SAW_SLP_RST_EN] = 0x00,
		.reg_init_values[MSM_SPM_REG_SAW_SPM_MPM_CFG] = 0x00,

		.awake_vlevel = 0x94,
		.retention_vlevel = 0x81,
		.collapse_vlevel = 0x20,
		.retention_mid_vlevel = 0x94,
		.collapse_mid_vlevel = 0x8C,

		.vctl_timeout_us = 50,
	},
};

static struct msm_spm_platform_data msm_spm_data[] __initdata = {
	[0] = {
		.reg_base_addr = MSM_SAW0_BASE,

#ifdef CONFIG_MSM_AVS_HW
		.reg_init_values[MSM_SPM_REG_SAW_AVS_CTL] = 0x586020FF,
#endif
		.reg_init_values[MSM_SPM_REG_SAW_CFG] = 0x1C,
		.reg_init_values[MSM_SPM_REG_SAW_SPM_CTL] = 0x68,
		.reg_init_values[MSM_SPM_REG_SAW_SPM_SLP_TMR_DLY] = 0x0C0CFFFF,
		.reg_init_values[MSM_SPM_REG_SAW_SPM_WAKE_TMR_DLY] = 0x78780FFF,

		.reg_init_values[MSM_SPM_REG_SAW_SLP_CLK_EN] = 0x01,
		.reg_init_values[MSM_SPM_REG_SAW_SLP_HSFS_PRECLMP_EN] = 0x07,
		.reg_init_values[MSM_SPM_REG_SAW_SLP_HSFS_POSTCLMP_EN] = 0x00,

		.reg_init_values[MSM_SPM_REG_SAW_SLP_CLMP_EN] = 0x01,
		.reg_init_values[MSM_SPM_REG_SAW_SLP_RST_EN] = 0x00,
		.reg_init_values[MSM_SPM_REG_SAW_SPM_MPM_CFG] = 0x00,

		.awake_vlevel = 0xA0,
		.retention_vlevel = 0x89,
		.collapse_vlevel = 0x20,
		.retention_mid_vlevel = 0x89,
		.collapse_mid_vlevel = 0x89,

		.vctl_timeout_us = 50,
	},

	[1] = {
		.reg_base_addr = MSM_SAW1_BASE,

#ifdef CONFIG_MSM_AVS_HW
		.reg_init_values[MSM_SPM_REG_SAW_AVS_CTL] = 0x586020FF,
#endif
		.reg_init_values[MSM_SPM_REG_SAW_CFG] = 0x1C,
		.reg_init_values[MSM_SPM_REG_SAW_SPM_CTL] = 0x68,
		.reg_init_values[MSM_SPM_REG_SAW_SPM_SLP_TMR_DLY] = 0x0C0CFFFF,
		.reg_init_values[MSM_SPM_REG_SAW_SPM_WAKE_TMR_DLY] = 0x78780FFF,

		.reg_init_values[MSM_SPM_REG_SAW_SLP_CLK_EN] = 0x13,
		.reg_init_values[MSM_SPM_REG_SAW_SLP_HSFS_PRECLMP_EN] = 0x07,
		.reg_init_values[MSM_SPM_REG_SAW_SLP_HSFS_POSTCLMP_EN] = 0x00,

		.reg_init_values[MSM_SPM_REG_SAW_SLP_CLMP_EN] = 0x01,
		.reg_init_values[MSM_SPM_REG_SAW_SLP_RST_EN] = 0x00,
		.reg_init_values[MSM_SPM_REG_SAW_SPM_MPM_CFG] = 0x00,

		.awake_vlevel = 0xA0,
		.retention_vlevel = 0x89,
		.collapse_vlevel = 0x20,
		.retention_mid_vlevel = 0x89,
		.collapse_mid_vlevel = 0x89,

		.vctl_timeout_us = 50,
	},
};

/*
 * Consumer specific regulator names:
 *			 regulator name		consumer dev_name
 */
static struct regulator_consumer_supply vreg_consumers_8901_S0[] = {
	REGULATOR_SUPPLY("8901_s0",		NULL),
};
static struct regulator_consumer_supply vreg_consumers_8901_S1[] = {
	REGULATOR_SUPPLY("8901_s1",		NULL),
};

static struct regulator_init_data saw_s0_init_data = {
		.constraints = {
			.name = "8901_s0",
			.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE,
			.min_uV = 800000,
			.max_uV = 1325000,
		},
		.consumer_supplies = vreg_consumers_8901_S0,
		.num_consumer_supplies = ARRAY_SIZE(vreg_consumers_8901_S0),
};

static struct regulator_init_data saw_s1_init_data = {
		.constraints = {
			.name = "8901_s1",
			.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE,
			.min_uV = 800000,
			.max_uV = 1325000,
		},
		.consumer_supplies = vreg_consumers_8901_S1,
		.num_consumer_supplies = ARRAY_SIZE(vreg_consumers_8901_S1),
};

static struct platform_device msm_device_saw_s0 = {
	.name          = "saw-regulator",
	.id            = 0,
	.dev           = {
		.platform_data = &saw_s0_init_data,
	},
};

static struct platform_device msm_device_saw_s1 = {
	.name          = "saw-regulator",
	.id            = 1,
	.dev           = {
		.platform_data = &saw_s1_init_data,
	},
};

#if defined(CONFIG_CRYPTO_DEV_QCRYPTO) || \
		defined(CONFIG_CRYPTO_DEV_QCRYPTO_MODULE) || \
		defined(CONFIG_CRYPTO_DEV_QCEDEV) || \
		defined(CONFIG_CRYPTO_DEV_QCEDEV_MODULE)

#define QCE_SIZE		0x10000
#define QCE_0_BASE		0x18500000

#define QCE_HW_KEY_SUPPORT	0
#define QCE_SHA_HMAC_SUPPORT	0
#define QCE_SHARE_CE_RESOURCE	2
#define QCE_CE_SHARED		1

static struct resource qcrypto_resources[] = {
	[0] = {
		.start = QCE_0_BASE,
		.end = QCE_0_BASE + QCE_SIZE - 1,
		.flags = IORESOURCE_MEM,
	},
	[1] = {
		.name = "crypto_channels",
		.start = DMOV_CE_IN_CHAN,
		.end = DMOV_CE_OUT_CHAN,
		.flags = IORESOURCE_DMA,
	},
	[2] = {
		.name = "crypto_crci_in",
		.start = DMOV_CE_IN_CRCI,
		.end = DMOV_CE_IN_CRCI,
		.flags = IORESOURCE_DMA,
	},
	[3] = {
		.name = "crypto_crci_out",
		.start = DMOV_CE_OUT_CRCI,
		.end = DMOV_CE_OUT_CRCI,
		.flags = IORESOURCE_DMA,
	},
	[4] = {
		.name = "crypto_crci_hash",
		.start = DMOV_CE_HASH_CRCI,
		.end = DMOV_CE_HASH_CRCI,
		.flags = IORESOURCE_DMA,
	},
};

static struct resource qcedev_resources[] = {
	[0] = {
		.start = QCE_0_BASE,
		.end = QCE_0_BASE + QCE_SIZE - 1,
		.flags = IORESOURCE_MEM,
	},
	[1] = {
		.name = "crypto_channels",
		.start = DMOV_CE_IN_CHAN,
		.end = DMOV_CE_OUT_CHAN,
		.flags = IORESOURCE_DMA,
	},
	[2] = {
		.name = "crypto_crci_in",
		.start = DMOV_CE_IN_CRCI,
		.end = DMOV_CE_IN_CRCI,
		.flags = IORESOURCE_DMA,
	},
	[3] = {
		.name = "crypto_crci_out",
		.start = DMOV_CE_OUT_CRCI,
		.end = DMOV_CE_OUT_CRCI,
		.flags = IORESOURCE_DMA,
	},
	[4] = {
		.name = "crypto_crci_hash",
		.start = DMOV_CE_HASH_CRCI,
		.end = DMOV_CE_HASH_CRCI,
		.flags = IORESOURCE_DMA,
	},
};

#endif

#if defined(CONFIG_CRYPTO_DEV_QCRYPTO) || \
		defined(CONFIG_CRYPTO_DEV_QCRYPTO_MODULE)

static struct msm_ce_hw_support qcrypto_ce_hw_suppport = {
	.ce_shared = QCE_CE_SHARED,
	.shared_ce_resource = QCE_SHARE_CE_RESOURCE,
	.hw_key_support = QCE_HW_KEY_SUPPORT,
	.sha_hmac = QCE_SHA_HMAC_SUPPORT,
	.bus_scale_table = NULL,
};

static struct platform_device qcrypto_device = {
	.name		= "qcrypto",
	.id		= 0,
	.num_resources	= ARRAY_SIZE(qcrypto_resources),
	.resource	= qcrypto_resources,
	.dev		= {
		.coherent_dma_mask = DMA_BIT_MASK(32),
		.platform_data = &qcrypto_ce_hw_suppport,
	},
};
#endif

#if defined(CONFIG_CRYPTO_DEV_QCEDEV) || \
		defined(CONFIG_CRYPTO_DEV_QCEDEV_MODULE)

static struct msm_ce_hw_support qcedev_ce_hw_suppport = {
	.ce_shared = QCE_CE_SHARED,
	.shared_ce_resource = QCE_SHARE_CE_RESOURCE,
	.hw_key_support = QCE_HW_KEY_SUPPORT,
	.sha_hmac = QCE_SHA_HMAC_SUPPORT,
	.bus_scale_table = NULL,
};

static struct platform_device qcedev_device = {
	.name		= "qce",
	.id		= 0,
	.num_resources	= ARRAY_SIZE(qcedev_resources),
	.resource	= qcedev_resources,
	.dev		= {
		.coherent_dma_mask = DMA_BIT_MASK(32),
		.platform_data = &qcedev_ce_hw_suppport,
	},
};
#endif

static struct msm_pm_platform_data msm_pm_data[MSM_PM_SLEEP_MODE_NR * 2] = {
	[MSM_PM_MODE(0, MSM_PM_SLEEP_MODE_POWER_COLLAPSE)] = {
		.idle_supported = 1,
		.suspend_supported = 1,
		.idle_enabled = 0,
		.suspend_enabled = 0,
	},

	[MSM_PM_MODE(0, MSM_PM_SLEEP_MODE_POWER_COLLAPSE_STANDALONE)] = {
		.idle_supported = 1,
		.suspend_supported = 1,
		.idle_enabled = 0,
		.suspend_enabled = 0,
	},

	[MSM_PM_MODE(0, MSM_PM_SLEEP_MODE_WAIT_FOR_INTERRUPT)] = {
		.idle_supported = 1,
		.suspend_supported = 1,
		.idle_enabled = 1,
		.suspend_enabled = 1,
	},

	[MSM_PM_MODE(1, MSM_PM_SLEEP_MODE_POWER_COLLAPSE)] = {
		.idle_supported = 1,
		.suspend_supported = 1,
		.idle_enabled = 0,
		.suspend_enabled = 0,
	},

	[MSM_PM_MODE(1, MSM_PM_SLEEP_MODE_POWER_COLLAPSE_STANDALONE)] = {
		.idle_supported = 1,
		.suspend_supported = 1,
		.idle_enabled = 0,
		.suspend_enabled = 0,
	},

	[MSM_PM_MODE(1, MSM_PM_SLEEP_MODE_WAIT_FOR_INTERRUPT)] = {
		.idle_supported = 1,
		.suspend_supported = 1,
		.idle_enabled = 1,
		.suspend_enabled = 1,
	},
};

static struct msm_cpuidle_state msm_cstates[] __initdata = {
	{0, 0, "C0", "WFI",
		MSM_PM_SLEEP_MODE_WAIT_FOR_INTERRUPT},

	{0, 1, "C1", "STANDALONE_POWER_COLLAPSE",
		MSM_PM_SLEEP_MODE_POWER_COLLAPSE_STANDALONE},

	{0, 2, "C2", "POWER_COLLAPSE",
		MSM_PM_SLEEP_MODE_POWER_COLLAPSE},

	{1, 0, "C0", "WFI",
		MSM_PM_SLEEP_MODE_WAIT_FOR_INTERRUPT},

	{1, 1, "C1", "STANDALONE_POWER_COLLAPSE",
		MSM_PM_SLEEP_MODE_POWER_COLLAPSE_STANDALONE},
};

static struct msm_rpmrs_level msm_rpmrs_levels[] __initdata = {
	{
		MSM_PM_SLEEP_MODE_WAIT_FOR_INTERRUPT,
		MSM_RPMRS_LIMITS(ON, ACTIVE, MAX, ACTIVE),
		true,
		1, 8000, 100000, 1,
	},

	{
		MSM_PM_SLEEP_MODE_POWER_COLLAPSE_STANDALONE,
		MSM_RPMRS_LIMITS(ON, ACTIVE, MAX, ACTIVE),
		true,
		1500, 5000, 60100000, 3000,
	},

	{
		MSM_PM_SLEEP_MODE_POWER_COLLAPSE,
		MSM_RPMRS_LIMITS(ON, ACTIVE, MAX, ACTIVE),
		false,
		1800, 5000, 60350000, 3500,
	},
	{
		MSM_PM_SLEEP_MODE_POWER_COLLAPSE,
		MSM_RPMRS_LIMITS(OFF, ACTIVE, MAX, ACTIVE),
		false,
		3800, 4500, 65350000, 5500,
	},

	{
		MSM_PM_SLEEP_MODE_POWER_COLLAPSE,
		MSM_RPMRS_LIMITS(ON, HSFS_OPEN, MAX, ACTIVE),
		false,
		2800, 2500, 66850000, 4800,
	},

	{
		MSM_PM_SLEEP_MODE_POWER_COLLAPSE,
		MSM_RPMRS_LIMITS(OFF, HSFS_OPEN, MAX, ACTIVE),
		false,
		4800, 2000, 71850000, 6800,
	},

	{
		MSM_PM_SLEEP_MODE_POWER_COLLAPSE,
		MSM_RPMRS_LIMITS(OFF, HSFS_OPEN, ACTIVE, RET_HIGH),
		false,
		6800, 500, 75850000, 8800,
	},

	{
		MSM_PM_SLEEP_MODE_POWER_COLLAPSE,
		MSM_RPMRS_LIMITS(OFF, HSFS_OPEN, RET_HIGH, RET_LOW),
		false,
		7800, 0, 76350000, 9800,
	},
};

static struct msm_pm_boot_platform_data msm_pm_boot_pdata __initdata = {
	.mode = MSM_PM_BOOT_CONFIG_TZ,
};
#if defined(CONFIG_USB_GADGET_MSM_72K) || defined(CONFIG_USB_EHCI_MSM_72K)
static struct msm_otg_platform_data msm_otg_pdata;
static struct regulator *ldo6_3p3;
static struct regulator *ldo7_1p8;
static struct regulator *vdd_cx;
#define PMICID_INT		PM8058_GPIO_IRQ(PM8058_IRQ_BASE, 36)
#define PMIC_ID_GPIO		36
notify_vbus_state notify_vbus_state_func_ptr;

#ifdef CONFIG_LGE_PM_CURRENT_CONSUMPTION_FIX
static int usb_phy_susp_dig_vol = 500000;
#else
static int usb_phy_susp_dig_vol = 750000;
#endif

static int pmic_id_notif_supported;

#ifdef CONFIG_USB_EHCI_MSM_72K
#define USB_PMIC_ID_DET_DELAY	msecs_to_jiffies(100)
struct delayed_work pmic_id_det;

static int __init usb_id_pin_rework_setup(char *support)
{
	if (strncmp(support, "true", 4) == 0)
		pmic_id_notif_supported = 1;

	return 1;
}
__setup("usb_id_pin_rework=", usb_id_pin_rework_setup);

#if ! defined(CONFIG_LGE_PMIC8058_GPIO)
static void pmic_id_detect(struct work_struct *w)
{
	int val = gpio_get_value_cansleep(PM8058_GPIO_PM_TO_SYS(36));
	pr_debug("%s(): gpio_read_value = %d\n", __func__, val);

	if (notify_vbus_state_func_ptr)
		(*notify_vbus_state_func_ptr) (val);
}

static irqreturn_t pmic_id_on_irq(int irq, void *data)
{
	/*
	 * Spurious interrupts are observed on pmic gpio line
	 * even though there is no state change on USB ID. Schedule the
	 * work to to allow debounce on gpio
	 */
	schedule_delayed_work(&pmic_id_det, USB_PMIC_ID_DET_DELAY);

	return IRQ_HANDLED;
}
#endif

#if 0
static int msm_hsusb_phy_id_setup_init(int init)
{
	unsigned ret;

	struct pm8xxx_mpp_config_data hsusb_phy_mpp = {
		.type	= PM8XXX_MPP_TYPE_D_OUTPUT,
		.level	= PM8901_MPP_DIG_LEVEL_L5,
	};

	if (init) {
		hsusb_phy_mpp.control = PM8XXX_MPP_DOUT_CTRL_HIGH;
		ret = pm8xxx_mpp_config(PM8901_MPP_PM_TO_SYS(1),
							&hsusb_phy_mpp);
		if (ret < 0)
			pr_err("%s:MPP2 configuration failed\n", __func__);
	} else {
		hsusb_phy_mpp.control = PM8XXX_MPP_DOUT_CTRL_LOW;
		ret = pm8xxx_mpp_config(PM8901_MPP_PM_TO_SYS(1),
							&hsusb_phy_mpp);
		if (ret < 0)
			pr_err("%s:MPP2 un config failed\n", __func__);
	}
	return ret;
}
#endif

static int msm_hsusb_pmic_id_notif_init(void (*callback)(int online), int init)
{
#if defined(CONFIG_LGE_PMIC8058_GPIO)
	return -ENOTSUPP;
#else
	unsigned ret = -ENODEV;

	struct pm_gpio pmic_id_cfg = {
		.direction	= PM_GPIO_DIR_IN,
		.pull		= PM_GPIO_PULL_UP_1P5,
		.function	= PM_GPIO_FUNC_NORMAL,
		.vin_sel	= 2,
		.inv_int_pol	= 0,
	};
	struct pm_gpio pmic_id_uncfg = {
		.direction	= PM_GPIO_DIR_IN,
		.pull		= PM_GPIO_PULL_NO,
		.function	= PM_GPIO_FUNC_NORMAL,
		.vin_sel	= 2,
		.inv_int_pol	= 0,
	};
	if (!callback)
		return -EINVAL;

	if (machine_is_msm8x60_fluid())
		return -ENOTSUPP;

	if (SOCINFO_VERSION_MAJOR(socinfo_get_version()) != 2) {
		pr_debug("%s: USB_ID pin is not routed to PMIC"
					"on V1 surf/ffa\n", __func__);
		return -ENOTSUPP;
	}

	if ((machine_is_msm8x60_fusion() || machine_is_msm8x60_fusn_ffa() || machine_is_lge_i_board() || machine_is_lge_325_board()) &&
			!pmic_id_notif_supported) {
		pr_debug("%s: USB_ID is not routed to PMIC"
			"on V2 ffa\n", __func__);
		return -ENOTSUPP;
	}

	usb_phy_susp_dig_vol = 500000;

	if (init) {
		notify_vbus_state_func_ptr = callback;
		INIT_DELAYED_WORK(&pmic_id_det, pmic_id_detect);
		ret = pm8xxx_gpio_config(PM8058_GPIO_PM_TO_SYS(PMIC_ID_GPIO),
							&pmic_id_cfg);
		if (ret) {
			pr_err("%s:return val of pm8xxx_gpio_config: %d\n",
						__func__,  ret);
			return ret;
		}
		ret = request_threaded_irq(PMICID_INT, NULL, pmic_id_on_irq,
			(IRQF_TRIGGER_RISING|IRQF_TRIGGER_FALLING),
						"msm_otg_id", NULL);
		if (ret) {
			pr_err("%s:pmic_usb_id interrupt registration failed",
					__func__);
			return ret;
		}
		msm_otg_pdata.pmic_id_irq = PMICID_INT;
	} else {
		usb_phy_susp_dig_vol = 750000;
		free_irq(PMICID_INT, 0);
		ret = pm8xxx_gpio_config(PM8058_GPIO_PM_TO_SYS(PMIC_ID_GPIO),
							&pmic_id_uncfg);
		if (ret) {
			pr_err("%s: return val of pm8xxx_gpio_config: %d\n",
						__func__,  ret);
			return ret;
		}
		msm_otg_pdata.pmic_id_irq = 0;
		cancel_delayed_work_sync(&pmic_id_det);
		notify_vbus_state_func_ptr = NULL;
	}
	return 0;
#endif //CONFIG_LGE_PMIC8058_GPIO
}
#endif

#define USB_PHY_OPERATIONAL_MIN_VDD_DIG_VOL	1000000
#define USB_PHY_MAX_VDD_DIG_VOL			1320000
static int msm_hsusb_init_vddcx(int init)
{
	int ret = 0;

	if (init) {
		vdd_cx = regulator_get(NULL, "8058_s1");
		if (IS_ERR(vdd_cx)) {
			return PTR_ERR(vdd_cx);
		}

		ret = regulator_set_voltage(vdd_cx,
				USB_PHY_OPERATIONAL_MIN_VDD_DIG_VOL,
				USB_PHY_MAX_VDD_DIG_VOL);
		if (ret) {
			pr_err("%s: unable to set the voltage for regulator"
				"vdd_cx\n", __func__);
			regulator_put(vdd_cx);
			return ret;
		}

		ret = regulator_enable(vdd_cx);
		if (ret) {
			pr_err("%s: unable to enable regulator"
				"vdd_cx\n", __func__);
			regulator_put(vdd_cx);
		}
	} else {
		ret = regulator_disable(vdd_cx);
		if (ret) {
			pr_err("%s: Unable to disable the regulator:"
				"vdd_cx\n", __func__);
			return ret;
		}

		regulator_put(vdd_cx);
	}

	return ret;
}

static int msm_hsusb_config_vddcx(int high)
{
	int max_vol = USB_PHY_MAX_VDD_DIG_VOL;
	int min_vol;
	int ret;

	if (high)
		min_vol = USB_PHY_OPERATIONAL_MIN_VDD_DIG_VOL;
	else
		min_vol = usb_phy_susp_dig_vol;

	ret = regulator_set_voltage(vdd_cx, min_vol, max_vol);
	if (ret) {
		pr_err("%s: unable to set the voltage for regulator"
			"vdd_cx\n", __func__);
		return ret;
	}

	pr_debug("%s: min_vol:%d max_vol:%d\n", __func__, min_vol, max_vol);

	return ret;
}

#define USB_PHY_3P3_VOL_MIN	3050000 /* uV */
#define USB_PHY_3P3_VOL_MAX	3050000 /* uV */
#define USB_PHY_3P3_HPM_LOAD	50000	/* uA */
#define USB_PHY_3P3_LPM_LOAD	4000	/* uA */

#define USB_PHY_1P8_VOL_MIN	1800000 /* uV */
#define USB_PHY_1P8_VOL_MAX	1800000 /* uV */
#define USB_PHY_1P8_HPM_LOAD	50000	/* uA */
#define USB_PHY_1P8_LPM_LOAD	4000	/* uA */

#ifdef CONFIG_USB_G_LGE_ANDROID_FACTORY
#include "../../../lge/include/lg_power_common.h"
static int usb_ldo_vltg = USB_PHY_3P3_VOL_MIN;
int msm_hsusb_ldo_set_3p5(acc_cable_type usb_cable_type)
{
	int rc = 0;

	if (ldo6_3p3)
	{
		if (usb_cable_type == LT_CABLE_56K && usb_ldo_vltg != 3500000)
		{
            rc = regulator_disable(ldo6_3p3);
            rc |= regulator_set_voltage(ldo6_3p3, 3500000, 3500000);
            rc |= regulator_enable(ldo6_3p3);
            usb_ldo_vltg = 3500000;
		}
        else if (usb_cable_type != LT_CABLE_56K && usb_ldo_vltg == 3500000)
        {
            rc = regulator_disable(ldo6_3p3);
            rc |= regulator_set_voltage(ldo6_3p3, USB_PHY_3P3_VOL_MIN, USB_PHY_3P3_VOL_MAX);
            rc |= regulator_enable(ldo6_3p3);
            usb_ldo_vltg = USB_PHY_3P3_VOL_MIN;
        }
        else
        {
            pr_info("%s: hsusb_ldo6_3p3 is already set (%d)\n", __func__, usb_ldo_vltg);
            return 0;
        }

		pr_info("%s: regulator_set_voltage(ldo6_3p3, %d)\n", __func__, usb_ldo_vltg);
		return rc;
	}
	return -1;
}
#endif

static int msm_hsusb_ldo_init(int init)
{
	int rc = 0;

	if (init) {
		ldo6_3p3 = regulator_get(NULL, "8058_l6");
		if (IS_ERR(ldo6_3p3))
			return PTR_ERR(ldo6_3p3);

		ldo7_1p8 = regulator_get(NULL, "8058_l7");
		if (IS_ERR(ldo7_1p8)) {
			rc = PTR_ERR(ldo7_1p8);
			goto put_3p3;
		}

		rc = regulator_set_voltage(ldo6_3p3, USB_PHY_3P3_VOL_MIN,
				USB_PHY_3P3_VOL_MAX);
		if (rc) {
			pr_err("%s: Unable to set voltage level for"
				"ldo6_3p3 regulator\n", __func__);
			goto put_1p8;
		}
		rc = regulator_enable(ldo6_3p3);
		if (rc) {
			pr_err("%s: Unable to enable the regulator:"
				"ldo6_3p3\n", __func__);
			goto put_1p8;
		}
		rc = regulator_set_voltage(ldo7_1p8, USB_PHY_1P8_VOL_MIN,
				USB_PHY_1P8_VOL_MAX);
		if (rc) {
			pr_err("%s: Unable to set voltage level for"
				"ldo7_1p8 regulator\n", __func__);
			goto disable_3p3;
		}
		rc = regulator_enable(ldo7_1p8);
		if (rc) {
			pr_err("%s: Unable to enable the regulator:"
				"ldo7_1p8\n", __func__);
			goto disable_3p3;
		}

		return 0;
	}

	regulator_disable(ldo7_1p8);
disable_3p3:
	regulator_disable(ldo6_3p3);
put_1p8:
	regulator_put(ldo7_1p8);
put_3p3:
	regulator_put(ldo6_3p3);
	return rc;
}

static int msm_hsusb_ldo_enable(int on)
{
	int ret = 0;

	if (!ldo7_1p8 || IS_ERR(ldo7_1p8)) {
		pr_err("%s: ldo7_1p8 is not initialized\n", __func__);
		return -ENODEV;
	}

	if (!ldo6_3p3 || IS_ERR(ldo6_3p3)) {
		pr_err("%s: ldo6_3p3 is not initialized\n", __func__);
		return -ENODEV;
	}

	if (on) {
		ret = regulator_set_optimum_mode(ldo7_1p8,
				USB_PHY_1P8_HPM_LOAD);
		if (ret < 0) {
			pr_err("%s: Unable to set HPM of the regulator:"
				"ldo7_1p8\n", __func__);
			return ret;
		}
		ret = regulator_set_optimum_mode(ldo6_3p3,
				USB_PHY_3P3_HPM_LOAD);
		if (ret < 0) {
			pr_err("%s: Unable to set HPM of the regulator:"
				"ldo6_3p3\n", __func__);
			regulator_set_optimum_mode(ldo7_1p8,
				USB_PHY_1P8_LPM_LOAD);
			return ret;
		}
	} else {
		ret = regulator_set_optimum_mode(ldo7_1p8,
				USB_PHY_1P8_LPM_LOAD);
		if (ret < 0)
			pr_err("%s: Unable to set LPM of the regulator:"
				"ldo7_1p8\n", __func__);
		ret = regulator_set_optimum_mode(ldo6_3p3,
				USB_PHY_3P3_LPM_LOAD);
		if (ret < 0)
			pr_err("%s: Unable to set LPM of the regulator:"
				"ldo6_3p3\n", __func__);
	}

	pr_debug("reg (%s)\n", on ? "HPM" : "LPM");
	return ret < 0 ? ret : 0;
 }
#endif
#ifdef CONFIG_USB_EHCI_MSM_72K
static void msm_hsusb_vbus_power(unsigned phy_info, int on)
{
#ifdef CONFIG_LGE_PMIC8901_REGULATOR
		return;
#else
	static struct regulator *votg_5v_switch;
	static struct regulator *ext_5v_reg;
	static int vbus_is_on;

	/* If VBUS is already on (or off), do nothing. */
	if (on == vbus_is_on)
		return;

	if (!votg_5v_switch) {
		votg_5v_switch = regulator_get(NULL, "8901_usb_otg");
		if (IS_ERR(votg_5v_switch)) {
			pr_err("%s: unable to get votg_5v_switch\n", __func__);
			return;
		}
	}
	if (!ext_5v_reg) {
		ext_5v_reg = regulator_get(NULL, "8901_mpp0");
		if (IS_ERR(ext_5v_reg)) {
			pr_err("%s: unable to get ext_5v_reg\n", __func__);
			return;
		}
	}
	if (on) {
		if (regulator_enable(ext_5v_reg)) {
			pr_err("%s: Unable to enable the regulator:"
					" ext_5v_reg\n", __func__);
			return;
		}
		if (regulator_enable(votg_5v_switch)) {
			pr_err("%s: Unable to enable the regulator:"
					" votg_5v_switch\n", __func__);
			return;
		}
	} else {
		if (regulator_disable(votg_5v_switch))
			pr_err("%s: Unable to enable the regulator:"
				" votg_5v_switch\n", __func__);
		if (regulator_disable(ext_5v_reg))
			pr_err("%s: Unable to enable the regulator:"
				" ext_5v_reg\n", __func__);
	}

	vbus_is_on = on;
#endif
}

static struct msm_usb_host_platform_data msm_usb_host_pdata = {
	.phy_info	= (USB_PHY_INTEGRATED | USB_PHY_MODEL_45NM),
	.power_budget	= 390,
};
#endif

#ifdef CONFIG_BATTERY_MSM8X60
static int msm_hsusb_pmic_vbus_notif_init(void (*callback)(int online),
								int init)
{
	int ret = -ENOTSUPP;

	/* ID and VBUS lines are connected to pmic on 8660.V2.SURF,
	 * hence, irrespective of either peripheral only mode or
	 * OTG (host and peripheral) modes, can depend on pmic for
	 * vbus notifications
	 */
	if ((SOCINFO_VERSION_MAJOR(socinfo_get_version()) == 2)
			&& (machine_is_msm8x60_surf() ||
				pmic_id_notif_supported)) {
		if (init)
			ret = msm_charger_register_vbus_sn(callback);
		else {
			msm_charger_unregister_vbus_sn(callback);
			ret = 0;
		}
	} else {
#if !defined(CONFIG_USB_EHCI_MSM_72K)
	if (init)
		ret = msm_charger_register_vbus_sn(callback);
	else {
		msm_charger_unregister_vbus_sn(callback);
		ret = 0;
	}
#endif
	}
	return ret;
}
#endif

#if defined(CONFIG_USB_GADGET_MSM_72K) || defined(CONFIG_USB_EHCI_MSM_72K)
static struct msm_otg_platform_data msm_otg_pdata = {
	/* if usb link is in sps there is no need for
	 * usb pclk as dayatona fabric clock will be
	 * used instead
	 */
	.pemp_level		 = PRE_EMPHASIS_WITH_20_PERCENT,
	.cdr_autoreset		 = CDR_AUTO_RESET_DISABLE,
	.se1_gating		 = SE1_GATING_DISABLE,
	.bam_disable		 = 1,
#ifdef CONFIG_USB_EHCI_MSM_72K
	.pmic_id_notif_init = msm_hsusb_pmic_id_notif_init,
#if 0
	.phy_id_setup_init = msm_hsusb_phy_id_setup_init,
#endif
#endif
#ifdef CONFIG_USB_EHCI_MSM_72K
	.vbus_power = msm_hsusb_vbus_power,
#endif
#ifdef CONFIG_BATTERY_MSM8X60
	.pmic_vbus_notif_init	= msm_hsusb_pmic_vbus_notif_init,
#endif
	.ldo_init		 = msm_hsusb_ldo_init,
	.ldo_enable		 = msm_hsusb_ldo_enable,
	.config_vddcx            = msm_hsusb_config_vddcx,
	.init_vddcx              = msm_hsusb_init_vddcx,
#ifdef CONFIG_BATTERY_MSM8X60
	.chg_vbus_draw = msm_charger_vbus_draw,
#endif
};
#endif

#ifdef CONFIG_USB_GADGET_MSM_72K
static struct msm_hsusb_gadget_platform_data msm_gadget_pdata = {
	.is_phy_status_timer_on = 1,
};
#endif

#ifdef CONFIG_USB_G_ANDROID

#define PID_MAGIC_ID		0x71432909
#define SERIAL_NUM_MAGIC_ID	0x61945374
#define SERIAL_NUMBER_LENGTH	127
#define DLOAD_USB_BASE_ADD	0x2A05F0C8

struct magic_num_struct {
	uint32_t pid;
	uint32_t serial_num;
};

struct dload_struct {
	uint32_t	reserved1;
	uint32_t	reserved2;
	uint32_t	reserved3;
	uint16_t	reserved4;
	uint16_t	pid;
	char		serial_number[SERIAL_NUMBER_LENGTH];
	uint16_t	reserved5;
	struct magic_num_struct
			magic_struct;
};

static int usb_diag_update_pid_and_serial_num(uint32_t pid, const char *snum)
{
	struct dload_struct __iomem *dload = 0;

	dload = ioremap(DLOAD_USB_BASE_ADD, sizeof(*dload));
	if (!dload) {
		pr_err("%s: cannot remap I/O memory region: %08x\n",
					__func__, DLOAD_USB_BASE_ADD);
		return -ENXIO;
	}

	pr_debug("%s: dload:%p pid:%x serial_num:%s\n",
				__func__, dload, pid, snum);
	/* update pid */
	dload->magic_struct.pid = PID_MAGIC_ID;
	dload->pid = pid;

	/* update serial number */
	dload->magic_struct.serial_num = 0;
	if (!snum)
		return 0;

	dload->magic_struct.serial_num = SERIAL_NUM_MAGIC_ID;
	strncpy(dload->serial_number, snum, SERIAL_NUMBER_LENGTH);
	dload->serial_number[SERIAL_NUMBER_LENGTH - 1] = '\0';

	iounmap(dload);

	return 0;
}

static struct android_usb_platform_data android_usb_pdata = {
	.update_pid_and_serial_num = usb_diag_update_pid_and_serial_num,
};

static struct platform_device android_usb_device = {
	.name	= "android_usb",
	.id	= -1,
	.dev	= {
		.platform_data = &android_usb_pdata,
	},
};
#endif

#ifdef CONFIG_MSM_VPE
#ifndef CONFIG_MSM_CAMERA_V4L2
static struct resource msm_vpe_resources[] = {
	{
		.start	= 0x05300000,
		.end	= 0x05300000 + SZ_1M - 1,
		.flags	= IORESOURCE_MEM,
	},
	{
		.start	= INT_VPE,
		.end	= INT_VPE,
		.flags	= IORESOURCE_IRQ,
	},
};

static struct platform_device msm_vpe_device = {
	.name = "msm_vpe",
	.id   = 0,
	.num_resources = ARRAY_SIZE(msm_vpe_resources),
	.resource = msm_vpe_resources,
};
#endif
#endif


#ifdef CONFIG_MSM_GEMINI
static struct resource msm_gemini_resources[] = {
	{
		.start  = 0x04600000,
		.end    = 0x04600000 + SZ_1M - 1,
		.flags  = IORESOURCE_MEM,
	},
	{
		.start  = INT_JPEG,
		.end    = INT_JPEG,
		.flags  = IORESOURCE_IRQ,
	},
};

static struct platform_device msm_gemini_device = {
	.name           = "msm_gemini",
	.resource       = msm_gemini_resources,
	.num_resources  = ARRAY_SIZE(msm_gemini_resources),
};
#endif
//2011-05-05 ella.hwang added for 1Seg Driver SPI porting - [Begins]
#ifdef CONFIG_LGE_BROADCAST_DCM
static struct spi_board_info __initdata lge_bcast_msm_spi_board_info[]  = {
	{
		.modalias	= "mb86a35s",
		.mode		= SPI_MODE_0,
		.irq		= MSM_GPIO_TO_INT(141),	//DTV_INT_GPIO_NUM  Rev.C : 62  -> Rev.D: 141 kichul.park
		.bus_num	= 2,
		.chip_select	= 0,
		//.max_speed_hz	= 15060000,	//set spi clock 24MHz from 10MHz(10000000) taew00k.kang 2011-08-30
		 .max_speed_hz	= 32 * 1000 * 1000,
		//.max_speed_hz	= 24 * 1000 * 1000,
		//.platform_data	= &bma_pdata,
	},
};
#endif
//2011-05-05 ella.hwang added for 1Seg Driver SPI porting - [Ends]

#ifdef CONFIG_LGE_BROADCAST_TDMB
static struct spi_board_info __initdata tdmb_spi_board_info[]  = {
	{
		.modalias	= "tdmb_fc8050",
		.mode		= SPI_MODE_0,
		.irq		= MSM_GPIO_TO_INT(107),	//TDMB_INT_GPIO_NUM
		.bus_num	= 2,
		.chip_select	= 0,
		.max_speed_hz	= 24*1000*1000,
		//.platform_data	= &bma_pdata,
	},
};
#endif	// CONFIG_LGE_BROADCAST_TDMB

#ifdef CONFIG_I2C_QUP
static void gsbi_qup_i2c_gpio_config(int adap_id, int config_type)
{
}

#if defined (CONFIG_LGE_TOUCHSCREEN_SYNAPTICS_RMI4_I2C) || defined (CONFIG_LGE_TOUCH_SYNAPTICS_325)|| defined (CONFIG_LGE_TOUCHSCREEN_MELFAS)
static struct msm_i2c_platform_data msm_gsbi1_qup_i2c_pdata = {
	.clk_freq = 400000,
	.src_clk_rate = 24000000,
	.msm_i2c_config_gpio = gsbi_qup_i2c_gpio_config,
};
#endif

static struct msm_i2c_platform_data msm_gsbi3_qup_i2c_pdata = {
//20110729, seunghyup.ryoo@lge.com,  [START]
#if defined(CONFIG_LGE_NFC_NXP_PN544PN65N)
	.clk_freq = 400000,
#else
	.clk_freq = 384000,
#endif
//20110729, seunghyup.ryoo@lge.com,  [END]
	.src_clk_rate = 24000000,
	.msm_i2c_config_gpio = gsbi_qup_i2c_gpio_config,
};

static struct msm_i2c_platform_data msm_gsbi4_qup_i2c_pdata = {
	.clk_freq = 400000,	// LGE_BSP_CAMERA::jungki.kim@lge.com  2012-04-12  Increase I2C Clock Speed like GB
	.src_clk_rate = 24000000,
	.msm_i2c_config_gpio = gsbi_qup_i2c_gpio_config,
};

#ifdef CONFIG_LGE_FUEL_GAUGE
static struct msm_i2c_platform_data msm_gsbi5_qup_i2c_pdata = {
	.clk_freq = 100000,
	.src_clk_rate = 24000000,
	.msm_i2c_config_gpio = gsbi_qup_i2c_gpio_config,
};
#endif

static struct msm_i2c_platform_data msm_gsbi7_qup_i2c_pdata = {
	.clk_freq = 100000,
	.src_clk_rate = 24000000,
	.msm_i2c_config_gpio = gsbi_qup_i2c_gpio_config,
};

static struct msm_i2c_platform_data msm_gsbi8_qup_i2c_pdata = {
	.clk_freq = 100000,
	.src_clk_rate = 24000000,
	.use_gsbi_shared_mode = 1, /*kyungbum.o@lge.com irda,hdmai gsbi8_DualMode 2012.0331 */
	.msm_i2c_config_gpio = gsbi_qup_i2c_gpio_config,
};

static struct msm_i2c_platform_data msm_gsbi9_qup_i2c_pdata = {
	.clk_freq = 100000,
	.src_clk_rate = 24000000,
	.msm_i2c_config_gpio = gsbi_qup_i2c_gpio_config,
};

static struct msm_i2c_platform_data msm_gsbi10_qup_i2c_pdata = {
	.clk_freq = 400000,
	.src_clk_rate = 24000000,
	.msm_i2c_config_gpio = gsbi_qup_i2c_gpio_config,
};

#if defined (CONFIG_LGE_WIRELESS_CHARGER_MAX8971) || defined (CONFIG_LGE_WIRELESS_CHARGER_BQ24160)
static struct msm_i2c_platform_data msm_gsbi11_qup_i2c_pdata = {
	.clk_freq = 100000,
	.src_clk_rate = 24000000,
	.msm_i2c_config_gpio = gsbi_qup_i2c_gpio_config,
};
#endif

static struct msm_i2c_platform_data msm_gsbi12_qup_i2c_pdata = {
	.clk_freq = 100000,
	.src_clk_rate = 24000000,
	.use_gsbi_shared_mode = 1,
	.msm_i2c_config_gpio = gsbi_qup_i2c_gpio_config,
};

#endif

#if defined(CONFIG_SPI_QUP) || defined(CONFIG_SPI_QUP_MODULE)
static struct msm_spi_platform_data msm_gsbi1_qup_spi_pdata = {
	.max_clock_speed = 24000000,
};

#ifndef CONFIG_LGE_FELICA
static struct msm_spi_platform_data msm_gsbi10_qup_spi_pdata = {
	.max_clock_speed = 24000000,
};
#endif
#endif

#if defined(CONFIG_LGE_BROADCAST_DCM) || defined(CONFIG_LGE_BROADCAST_TDMB)
static struct msm_spi_platform_data msm_gsbi11_qup_spi_pdata = {
#ifdef CONFIG_LGE_BROADCAST_DCM
	.max_clock_speed = 32 * 1000 * 1000,
#else
	.max_clock_speed = 24*1000*1000,
#endif
};
#endif

#ifdef CONFIG_I2C_SSBI
/* CODEC/TSSC SSBI */
static struct msm_i2c_ssbi_platform_data msm_ssbi3_pdata = {
	.controller_type = MSM_SBI_CTRL_SSBI,
};
#endif

#ifdef CONFIG_BATTERY_MSM
/* Use basic value for fake MSM battery */
static struct msm_psy_batt_pdata msm_psy_batt_data = {
	.avail_chg_sources = AC_CHG,
};

static struct platform_device msm_batt_device = {
	.name              = "msm-battery",
	.id                = -1,
	.dev.platform_data = &msm_psy_batt_data,
};
#endif


#define MSM_PMEM_SF_SIZE 0x4000000 /* 64 Mbytes */
#define MSM_HDMI_PRIM_PMEM_SF_SIZE 0x4000000 /* 64 Mbytes */

#ifdef CONFIG_FB_MSM_HDMI_AS_PRIMARY
unsigned char hdmi_is_primary = 1;
#else
unsigned char hdmi_is_primary;
#endif

#define MSM_PMEM_KERNEL_EBI1_SIZE  0x600000
#define MSM_PMEM_ADSP_SIZE         0x2000000
#define MSM_PMEM_AUDIO_SIZE        0x28B000

#define MSM_SMI_BASE          0x38000000
#define MSM_SMI_SIZE          0x4000000

#define KERNEL_SMI_BASE       (MSM_SMI_BASE)
#define KERNEL_SMI_SIZE       0x600000

#define USER_SMI_BASE         (KERNEL_SMI_BASE + KERNEL_SMI_SIZE)
#define USER_SMI_SIZE         (MSM_SMI_SIZE - KERNEL_SMI_SIZE)
#define MSM_PMEM_SMIPOOL_SIZE USER_SMI_SIZE

//#define MSM_ION_SF_SIZE		0x4000000 /* 64MB */
#define MSM_ION_SF_SIZE               0x5400000 /* 84MB */
#define MSM_ION_CAMERA_SIZE     MSM_PMEM_ADSP_SIZE
#define MSM_ION_MM_FW_SIZE	0x200000 /* (2MB) */
#define MSM_ION_MM_SIZE		0x3600000 /* (54MB) */
#define MSM_ION_MFC_SIZE	SZ_8K

//#define MSM_ION_WB_SIZE		0x610000 /* 6MB + 64KB*/
#define MSM_ION_WB_SIZE                0x2000000 /* 32MB */

#define MSM_ION_QSECOM_SIZE	0x600000 /* (6MB) */
#define MSM_ION_AUDIO_SIZE	MSM_PMEM_AUDIO_SIZE

#ifdef CONFIG_MSM_MULTIMEDIA_USE_ION
#if defined(CONFIG_MACH_LGE_325_BOARD_SKT) || defined(CONFIG_MACH_LGE_325_BOARD_LGU)
/* korean-model doesn't use CAMERA-ION for increasing 30MB memory */
#define MSM_ION_HEAP_NUM	8
#else
#define MSM_ION_HEAP_NUM	9
#endif
#define MSM_HDMI_PRIM_ION_SF_SIZE MSM_HDMI_PRIM_PMEM_SF_SIZE
static unsigned msm_ion_sf_size = MSM_ION_SF_SIZE;
#else
#define MSM_ION_HEAP_NUM	1
#endif

static unsigned fb_size;
static int __init fb_size_setup(char *p)
{
	fb_size = memparse(p, NULL);
	return 0;
}
early_param("fb_size", fb_size_setup);

static unsigned pmem_kernel_ebi1_size = MSM_PMEM_KERNEL_EBI1_SIZE;
static int __init pmem_kernel_ebi1_size_setup(char *p)
{
	pmem_kernel_ebi1_size = memparse(p, NULL);
	return 0;
}
early_param("pmem_kernel_ebi1_size", pmem_kernel_ebi1_size_setup);

#ifdef CONFIG_ANDROID_PMEM
static unsigned pmem_sf_size = MSM_PMEM_SF_SIZE;
static int __init pmem_sf_size_setup(char *p)
{
	pmem_sf_size = memparse(p, NULL);
	return 0;
}
early_param("pmem_sf_size", pmem_sf_size_setup);

static unsigned pmem_adsp_size = MSM_PMEM_ADSP_SIZE;

static int __init pmem_adsp_size_setup(char *p)
{
	pmem_adsp_size = memparse(p, NULL);
	return 0;
}
early_param("pmem_adsp_size", pmem_adsp_size_setup);

static unsigned pmem_audio_size = MSM_PMEM_AUDIO_SIZE;

static int __init pmem_audio_size_setup(char *p)
{
	pmem_audio_size = memparse(p, NULL);
	return 0;
}
early_param("pmem_audio_size", pmem_audio_size_setup);
#endif

#ifdef CONFIG_ANDROID_PMEM
#ifndef CONFIG_MSM_MULTIMEDIA_USE_ION
static struct android_pmem_platform_data android_pmem_pdata = {
	.name = "pmem",
	.allocator_type = PMEM_ALLOCATORTYPE_ALLORNOTHING,
	.cached = 1,
	.memory_type = MEMTYPE_EBI1,
};

static struct platform_device android_pmem_device = {
	.name = "android_pmem",
	.id = 0,
	.dev = {.platform_data = &android_pmem_pdata},
};

static struct android_pmem_platform_data android_pmem_adsp_pdata = {
	.name = "pmem_adsp",
	.allocator_type = PMEM_ALLOCATORTYPE_BITMAP,
	.cached = 0,
	.memory_type = MEMTYPE_EBI1,
};

static struct platform_device android_pmem_adsp_device = {
	.name = "android_pmem",
	.id = 2,
	.dev = { .platform_data = &android_pmem_adsp_pdata },
};
#endif
static struct android_pmem_platform_data android_pmem_audio_pdata = {
	.name = "pmem_audio",
	.allocator_type = PMEM_ALLOCATORTYPE_BITMAP,
	.cached = 0,
	.memory_type = MEMTYPE_EBI1,
};

static struct platform_device android_pmem_audio_device = {
	.name = "android_pmem",
	.id = 4,
	.dev = { .platform_data = &android_pmem_audio_pdata },
};

#define PMEM_BUS_WIDTH(_bw) \
	{ \
		.vectors = &(struct msm_bus_vectors){ \
			.src = MSM_BUS_MASTER_AMPSS_M0, \
			.dst = MSM_BUS_SLAVE_SMI, \
			.ib = (_bw), \
			.ab = 0, \
		}, \
	.num_paths = 1, \
	}

static struct msm_bus_paths mem_smi_table[] = {
	[0] = PMEM_BUS_WIDTH(0), /* Off */
	[1] = PMEM_BUS_WIDTH(1), /* On */
};

static struct msm_bus_scale_pdata smi_client_pdata = {
	.usecase = mem_smi_table,
	.num_usecases = ARRAY_SIZE(mem_smi_table),
	.name = "mem_smi",
};

int request_smi_region(void *data)
{
	int bus_id = (int) data;

	msm_bus_scale_client_update_request(bus_id, 1);
	return 0;
}

int release_smi_region(void *data)
{
	int bus_id = (int) data;

	msm_bus_scale_client_update_request(bus_id, 0);
	return 0;
}

void *setup_smi_region(void)
{
	return (void *)msm_bus_scale_register_client(&smi_client_pdata);
}
#ifndef CONFIG_MSM_MULTIMEDIA_USE_ION
static struct android_pmem_platform_data android_pmem_smipool_pdata = {
	.name = "pmem_smipool",
	.allocator_type = PMEM_ALLOCATORTYPE_BITMAP,
	.cached = 0,
	.memory_type = MEMTYPE_SMI,
	.request_region = request_smi_region,
	.release_region = release_smi_region,
	.setup_region = setup_smi_region,
	.map_on_demand = 1,
};
static struct platform_device android_pmem_smipool_device = {
	.name = "android_pmem",
	.id = 7,
	.dev = { .platform_data = &android_pmem_smipool_pdata },
};

#endif
#endif

#ifdef CONFIG_ANDROID_RAM_CONSOLE
#define LGE_RAM_CONSOLE_SIZE (256 * SZ_1K)
#endif

#ifdef CONFIG_LGE_HANDLE_PANIC
#define LGE_CRASH_LOG_SIZE (4 * SZ_1K)
#endif

#ifdef CONFIG_ANDROID_RAM_CONSOLE
static struct resource ram_console_resource[] = {
	{
		.name = "ram_console",
		.flags = IORESOURCE_MEM,
	}
};
static struct platform_device ram_console_device = {
	.name = "ram_console",
	.id = -1,
	.num_resources = ARRAY_SIZE(ram_console_resource),
	.resource = ram_console_resource,
};

void __init lge_add_ramconsole_devices(void)
{
	struct resource* res = ram_console_resource;

	res->start = allocate_contiguous_ebi_nomap(LGE_RAM_CONSOLE_SIZE, SZ_4K);
	res->end = res->start + LGE_RAM_CONSOLE_SIZE -1;
	printk("RAM CONSOLE START ADDR : 0x%x\n", res->start);
	printk("RAM CONSOLE END ADDR   : 0x%x\n", res->end);

	platform_device_register(&ram_console_device);
}
#endif


#ifdef CONFIG_LGE_HANDLE_PANIC
static struct resource crash_log_resource[] = {
	{
		.name = "crash_log",
		.flags = IORESOURCE_MEM,
	}
};

static struct platform_device panic_handler_device = {
	.name = "panic-handler",
	.num_resources = ARRAY_SIZE(crash_log_resource),
	.resource = crash_log_resource,
	.dev = {
		.platform_data = NULL,
	}
};

void __init lge_add_panic_handler_devices(void)
{
	struct resource* res = crash_log_resource;
	res->start = allocate_contiguous_ebi_nomap(LGE_CRASH_LOG_SIZE, SZ_4K);
	res->end = res->start + LGE_CRASH_LOG_SIZE - 1;

	printk(KERN_INFO "CRASH LOG START ADDR : 0x%x\n", res->start);
	printk(KERN_INFO "CRASH LOG END ADDR   : 0x%x\n", res->end);

	platform_device_register(&panic_handler_device);
}
#endif

#define GPIO_DONGLE_PWR_EN 258
#define GPIO_BACKLIGHT_PWM0 0
#define GPIO_BACKLIGHT_PWM1 1


#ifdef CONFIG_SERIAL_MSM_HS
static int configure_uart_gpios(int on)
{
	int ret = 0, i;
	int uart_gpios[] = {53, 54, 55, 56};
	for (i = 0; i < ARRAY_SIZE(uart_gpios); i++) {
		if (on) {
			ret = msm_gpiomux_get(uart_gpios[i]);
			if (unlikely(ret))
				break;
		} else {
			ret = msm_gpiomux_put(uart_gpios[i]);
			if (unlikely(ret))
				return ret;
		}
	}
	if (ret)
		for (; i >= 0; i--)
			msm_gpiomux_put(uart_gpios[i]);
	return ret;
}
static struct msm_serial_hs_platform_data msm_uart_dm1_pdata = {
       .inject_rx_on_wakeup = 1,
       .rx_to_inject = 0xFD,
       .gpio_config = configure_uart_gpios,
};
#endif


#ifdef CONFIG_LGE_IRDA

#define MSM_GSBI8_PHYS		0x19800000

static struct msm_serial_hslite_platform_data msm_uart_gsbi8_pdata = {
    .config_gpio    = 1,
    .uart_tx_gpio   = 62,
    .uart_rx_gpio   = 63,
};
#endif

#ifdef CONFIG_LGE_FELICA
static struct msm_serial_hslite_platform_data msm_uart_gsbi10_pdata = {
    .config_gpio    = 1,
    .uart_tx_gpio   = 71,
    .uart_rx_gpio   = 70,
};
#endif

#if defined (CONFIG_LGE_TOUCH_SYNAPTICS_325) || defined (CONFIG_LGE_TOUCHSCREEN_MELFAS)

#define SYNAPTICS_T1320_TS_I2C_SDA		35
#define SYNAPTICS_T1320_TS_I2C_SCL		36
#define SYNAPTICS_T1320_TS_I2C_INT_GPIO	61

static struct touch_device_caps touch_caps = {
	.button_support 			= 1,
	.y_button_boundary			= 0,
	.button_margin				= 0,
	.number_of_button 			= 4,	// 325 kyle.jeon@lge.com
	.button_name 				= {KEY_MENU,KEY_HOME,KEY_BACK,KEY_SEARCH},
	.is_width_supported 		= 1,
	.is_pressure_supported 		= 1,
	.is_id_supported			= 1,
	.max_width 					= 15,
	.max_pressure 				= 0xFF,
	.max_id						= 10,
	.lcd_x						= 768,
	.lcd_y						= 1024,
};

static struct touch_operation_role touch_role = {
	.operation_mode 		= INTERRUPT_MODE,
	.report_mode			= CONTINUOUS_REPORT_MODE,
	.delta_pos_threshold 	= 0,
	.orientation 			= 0,
		.report_period			= 10000000,
	.booting_delay 			= 200,
	.reset_delay			= 20,
	.suspend_pwr			= POWER_OFF,
	.jitter_filter_enable	= 1,
	.jitter_curr_ratio		= 26,
        .accuracy_filter_enable = 0,
	.irqflags 				= IRQF_TRIGGER_FALLING,
};

int synaptics_t1320_power_on(int on)
{
	int rc = -EINVAL;
	static struct regulator *vreg_t1320;
	static struct regulator *vreg_lvs2;

	if(!vreg_t1320)
	{
	vreg_t1320 = regulator_get(NULL, "8058_l19");		/* +3V0_TOUCH */
	if (IS_ERR(vreg_t1320)) {
			pr_err("%s: regulator get of 8058_l19 failed (%ld)\n", __func__, PTR_ERR(vreg_t1320));
		rc = PTR_ERR(vreg_t1320);
		return rc;
	}
	}
	rc = regulator_set_voltage(vreg_t1320, 3000000, 3000000);
	gpio_request(61 , "touch_int");
	gpio_request(35 , "touch_sda");
	gpio_request(36 , "touch_scl");
	if(!vreg_lvs2)
	{
	vreg_lvs2 = regulator_get(NULL, "8901_lvs2");	/* +1V8_TOUCH_VIO */
	if (IS_ERR(vreg_lvs2)) {
		pr_err("%s: regulator get of 8901_lvs2 failed (%ld)\n", __func__, PTR_ERR(vreg_lvs2));
		rc = PTR_ERR(vreg_lvs2);
		return rc;
	}
	}
	rc = regulator_set_voltage(vreg_lvs2, 1800000, 1800000);

	if(on)
	{
		rc = gpio_tlmm_config(GPIO_CFG(61, 0, GPIO_CFG_INPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_2MA), GPIO_CFG_ENABLE);
		gpio_direction_input(61);

		gpio_direction_output(35, 1);
		gpio_direction_output(36, 1);

		rc = regulator_enable(vreg_t1320);
		rc = regulator_enable(vreg_lvs2);
	}
	else
	{
		rc = gpio_tlmm_config(GPIO_CFG(61, 0, GPIO_CFG_OUTPUT,  GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), GPIO_CFG_ENABLE);
		gpio_direction_output(61, 0);
		gpio_direction_output(35, 0);
		gpio_direction_output(36, 0);

		rc = regulator_disable(vreg_lvs2);
		rc = regulator_disable(vreg_t1320);
	}
        gpio_free(61);
	gpio_free(35);
	gpio_free(36);
	return rc;
}

static struct touch_power_module touch_pwr = {
	.use_regulator	= 0,
	.vdd			= "8058_l19",
	.vdd_voltage	= 3000000,
	.vio			= "8901_lvs2",
	.vio_voltage	= 1800000,
	.power			= synaptics_t1320_power_on,
};

static struct touch_platform_data  i_att_ts_data = {
	.int_pin	= SYNAPTICS_T1320_TS_I2C_INT_GPIO,
	.reset_pin	= 0,
	.maker		= "Synaptics",
	.caps		= &touch_caps,
	.role		= &touch_role,
	.pwr		= &touch_pwr,
};

static struct i2c_board_info msm_i2c_synaptics_ts_info[] = {
       {
               I2C_BOARD_INFO(LGE_TOUCH_NAME, 0x20),
               .platform_data = &i_att_ts_data,
               .irq = MSM_GPIO_TO_INT(SYNAPTICS_T1320_TS_I2C_INT_GPIO),
       }
};

int melfas_ts_power_on(int on, bool log_en)
{
	int rc = -EINVAL;
	static struct regulator *vreg_l19;

	if(log_en)
		printk(KERN_INFO "[Touch D] %s: power %s\n", __func__, on ? "On" : "Off");

	if(!vreg_l19)
	{
		vreg_l19 = regulator_get(NULL, "8058_l19");           /* +3V0_TOUCH */
		if (IS_ERR(vreg_l19)) {
			pr_err("%s: regulator get of 8058_l19 failed (%ld)\n", __func__, PTR_ERR(vreg_l19));
			rc = PTR_ERR(vreg_l19);
			return rc;
		}
	}
	rc = regulator_set_voltage(vreg_l19, 3000000, 3000000);


	if(on)
	{
		rc = regulator_enable(vreg_l19);


	}
	else
	{
		rc = regulator_disable(vreg_l19);
	}

	return rc;
}

static struct melfas_tsi_platform_data melfas_touch_platform_data[] = {
	{
		.max_x = 768,
		.max_y = 1024,
		.max_pressure = 255,
		.max_width = 255,
		.gpio_scl = SYNAPTICS_T1320_TS_I2C_SCL,
		.gpio_sda = SYNAPTICS_T1320_TS_I2C_SDA,
		.ic_booting_delay	= 800,		/* ms */
		.power_enable = melfas_ts_power_on,
		.fw_ver = 8,
	},
};

static struct i2c_board_info msm_i2c_melfas_ts_info[] = {
       {
               I2C_BOARD_INFO("melfas-ts",0x48),
               .platform_data = &melfas_touch_platform_data,
               .irq = MSM_GPIO_TO_INT(SYNAPTICS_T1320_TS_I2C_INT_GPIO),
       }
};
#endif

//20110308, seunghyup.ryoo@lge.com,  [START]
#if defined(CONFIG_LGE_NFC_NXP_PN544PN65N)

#include "nfc_nxp_pn544pn65n.h"

#define NXP_PN544PN65N_NFC_I2C_SDA				43
#define NXP_PN544PN65N_NFC_I2C_SCL				44
#define NXP_PN544PN65N_NFC_I2C_SLAVEADDR		0x28
#define NXP_PN544PN65N_NFC_GPIO_IRQ				123
#define NXP_PN544PN65N_NFC_GPIO_VEN				130
#define NXP_PN544PN65N_NFC_GPIO_FIRM			46

static struct nxp_pn544pn65n_platform_data nfc_nxp_pn544pn65n_platform_data[] = {
	{
		.ven_gpio 		= NXP_PN544PN65N_NFC_GPIO_VEN,
		.irq_gpio 	 	= NXP_PN544PN65N_NFC_GPIO_IRQ,
		.scl_gpio		= NXP_PN544PN65N_NFC_I2C_SCL,
		.sda_gpio		= NXP_PN544PN65N_NFC_I2C_SDA,
		.firm_gpio		= NXP_PN544PN65N_NFC_GPIO_FIRM,
	},
};

static struct i2c_board_info msm_i2c_nxp_nfc_info[] = {
	{
		I2C_BOARD_INFO("pn544", NXP_PN544PN65N_NFC_I2C_SLAVEADDR),
		.platform_data = &nfc_nxp_pn544pn65n_platform_data,
		.irq = MSM_GPIO_TO_INT(NXP_PN544PN65N_NFC_GPIO_IRQ),
	}
};

#endif
//20110308, seunghyup.ryoo@lge.com,  [END]
#if defined(CONFIG_MSM_RPM_LOG) || defined(CONFIG_MSM_RPM_LOG_MODULE)

static struct msm_rpm_log_platform_data msm_rpm_log_pdata = {
	.phys_addr_base = 0x00106000,
	.reg_offsets = {
		[MSM_RPM_LOG_PAGE_INDICES] = 0x00000C80,
		[MSM_RPM_LOG_PAGE_BUFFER]  = 0x00000CA0,
	},
	.phys_size = SZ_8K,
	.log_len = 4096,		  /* log's buffer length in bytes */
	.log_len_mask = (4096 >> 2) - 1,  /* length mask in units of u32 */
};

static struct platform_device msm_rpm_log_device = {
	.name	= "msm_rpm_log",
	.id	= -1,
	.dev	= {
		.platform_data = &msm_rpm_log_pdata,
	},
};
#endif

#ifdef CONFIG_BATTERY_MSM8X60
#ifdef CONFIG_LGE_CHARGER_VOLTAGE_CURRENT_SCENARIO
static struct msm_charger_platform_data msm_charger_data = {
	.safety_time = 480,
	.update_time = 1,
	.max_voltage = 4350,
	.min_voltage = 3500,
};
#else

static struct msm_charger_platform_data msm_charger_data = {
	.safety_time = 180,
	.update_time = 1,
	.max_voltage = 4200,
	.min_voltage = 3200,
};
#endif

static struct platform_device msm_charger_device = {
	.name = "msm-charger",
	.id = -1,
	.dev = {
		.platform_data = &msm_charger_data,
	}
};
#endif

// START sungchae.koo@lge.com 2011/06/24 P1_LAB_BSP : USB_CABLE_VALUE_TUNNELING {
int usb_cable_info = 0;

static int __init usb_cable_info_setup(char *usb_cable)
{
// START sungchae.koo@lge.com 2011/07/19 P1_LAB_BSP : ADAPTATION_USB_CABLE_INFO {
// cable value ref from "acc_cable_type@android\kernel\lge\include\lg_power_common.h"

    if(!strcmp(usb_cable, "pif_56k"))
        usb_cable_info = 6; // LT_CABLE_56K
    else if(!strcmp(usb_cable, "pif_130k"))
        usb_cable_info = 7; // LT_CABLE_130K
    else if(!strcmp(usb_cable, "pif_910k"))
        usb_cable_info = 11; // LT_CABLE_910K
    else if(!strcmp(usb_cable, "normal"))
        usb_cable_info = 8; // USB_CABLE_400MA
    else
        usb_cable_info = 8; // USB_CABLE_400MA

	printk(KERN_INFO "usb_cable_info : %s (%d)\n", usb_cable, usb_cable_info);
// END sungchae.koo@lge.com 2011/07/19 P1_LAB_BSP }

	return 1;
}
__setup("lge.usb_cable=", usb_cable_info_setup);
// END sungchae.koo@lge.com 2011/06/24 P1_LAB_BSP }

/*
 * Consumer specific regulator names:
 *			 regulator name		consumer dev_name
 */
static struct regulator_consumer_supply vreg_consumers_PM8058_L0[] = {
	REGULATOR_SUPPLY("8058_l0",		NULL),
};
static struct regulator_consumer_supply vreg_consumers_PM8058_L1[] = {
	REGULATOR_SUPPLY("8058_l1",		NULL),
};
#ifdef CONFIG_LGE_PMIC8058_REGULATOR
/* we don't use */
#else
static struct regulator_consumer_supply vreg_consumers_PM8058_L2[] = {
	REGULATOR_SUPPLY("8058_l2",		NULL),
};
#endif

static struct regulator_consumer_supply vreg_consumers_PM8058_L3[] = {
	REGULATOR_SUPPLY("8058_l3",		NULL),
};
static struct regulator_consumer_supply vreg_consumers_PM8058_L4[] = {
	REGULATOR_SUPPLY("8058_l4",		NULL),
};
static struct regulator_consumer_supply vreg_consumers_PM8058_L5[] = {
	REGULATOR_SUPPLY("8058_l5",		NULL),
};
static struct regulator_consumer_supply vreg_consumers_PM8058_L6[] = {
	REGULATOR_SUPPLY("8058_l6",		NULL),
};
static struct regulator_consumer_supply vreg_consumers_PM8058_L7[] = {
	REGULATOR_SUPPLY("8058_l7",		NULL),
};
static struct regulator_consumer_supply vreg_consumers_PM8058_L8[] = {
	REGULATOR_SUPPLY("8058_l8",		NULL),
};
static struct regulator_consumer_supply vreg_consumers_PM8058_L9[] = {
	REGULATOR_SUPPLY("8058_l9",		NULL),
};

#ifdef CONFIG_LGE_PMIC8058_REGULATOR
/* we don't use */
#else
static struct regulator_consumer_supply vreg_consumers_PM8058_L10[] = {
	REGULATOR_SUPPLY("8058_l10",		NULL),
};
#endif

static struct regulator_consumer_supply vreg_consumers_PM8058_L11[] = {
	REGULATOR_SUPPLY("8058_l11",		NULL),
};
static struct regulator_consumer_supply vreg_consumers_PM8058_L12[] = {
	REGULATOR_SUPPLY("8058_l12",		NULL),
};
static struct regulator_consumer_supply vreg_consumers_PM8058_L13[] = {
	REGULATOR_SUPPLY("8058_l13",		NULL),
};
static struct regulator_consumer_supply vreg_consumers_PM8058_L14[] = {
	REGULATOR_SUPPLY("8058_l14",		NULL),
};
static struct regulator_consumer_supply vreg_consumers_PM8058_L15[] = {
	REGULATOR_SUPPLY("8058_l15",		NULL),
	REGULATOR_SUPPLY("cam_vana",		"1-001a"),
};
static struct regulator_consumer_supply vreg_consumers_PM8058_L16[] = {
	REGULATOR_SUPPLY("8058_l16",		NULL),
};
static struct regulator_consumer_supply vreg_consumers_PM8058_L17[] = {
	REGULATOR_SUPPLY("8058_l17",		NULL),
};
static struct regulator_consumer_supply vreg_consumers_PM8058_L18[] = {
	REGULATOR_SUPPLY("8058_l18",		NULL),
};
static struct regulator_consumer_supply vreg_consumers_PM8058_L19[] = {
	REGULATOR_SUPPLY("8058_l19",		NULL),
};
static struct regulator_consumer_supply vreg_consumers_PM8058_L20[] = {
	REGULATOR_SUPPLY("8058_l20",		NULL),
};
static struct regulator_consumer_supply vreg_consumers_PM8058_L21[] = {
	REGULATOR_SUPPLY("8058_l21",		NULL),
};
static struct regulator_consumer_supply vreg_consumers_PM8058_L22[] = {
	REGULATOR_SUPPLY("8058_l22",		NULL),
};
static struct regulator_consumer_supply vreg_consumers_PM8058_L23[] = {
	REGULATOR_SUPPLY("8058_l23",		NULL),
};

#ifdef CONFIG_LGE_PMIC8058_REGULATOR
/* we don't use */
#else
static struct regulator_consumer_supply vreg_consumers_PM8058_L24[] = {
	REGULATOR_SUPPLY("8058_l24",		NULL),
};
static struct regulator_consumer_supply vreg_consumers_PM8058_L25[] = {
	REGULATOR_SUPPLY("8058_l25",		NULL),
	REGULATOR_SUPPLY("cam_vdig",		"1-001a"),
};
#endif

static struct regulator_consumer_supply vreg_consumers_PM8058_S0[] = {
	REGULATOR_SUPPLY("8058_s0",		NULL),
};
static struct regulator_consumer_supply vreg_consumers_PM8058_S1[] = {
	REGULATOR_SUPPLY("8058_s1",		NULL),
};
static struct regulator_consumer_supply vreg_consumers_PM8058_S2[] = {
	REGULATOR_SUPPLY("8058_s2",		NULL),
};
static struct regulator_consumer_supply vreg_consumers_PM8058_S3[] = {
	REGULATOR_SUPPLY("8058_s3",		NULL),
};
static struct regulator_consumer_supply vreg_consumers_PM8058_S4[] = {
	REGULATOR_SUPPLY("8058_s4",		NULL),
};
static struct regulator_consumer_supply vreg_consumers_PM8058_LVS0[] = {
	REGULATOR_SUPPLY("8058_lvs0",		NULL),
};
static struct regulator_consumer_supply vreg_consumers_PM8058_LVS1[] = {
	REGULATOR_SUPPLY("8058_lvs1",		NULL),
};
#ifdef CONFIG_LGE_PMIC8058_REGULATOR
/* we don't use */
#else
static struct regulator_consumer_supply vreg_consumers_PM8058_NCP[] = {
	REGULATOR_SUPPLY("8058_ncp",		NULL),
};

static struct regulator_consumer_supply vreg_consumers_PM8901_L0[] = {
	REGULATOR_SUPPLY("8901_l0",		NULL),
};
#endif

static struct regulator_consumer_supply vreg_consumers_PM8901_L1[] = {
	REGULATOR_SUPPLY("8901_l1",		NULL),
};
static struct regulator_consumer_supply vreg_consumers_PM8901_L2[] = {
	REGULATOR_SUPPLY("8901_l2",		NULL),
};
static struct regulator_consumer_supply vreg_consumers_PM8901_L3[] = {
	REGULATOR_SUPPLY("8901_l3",		NULL),
};
static struct regulator_consumer_supply vreg_consumers_PM8901_L4[] = {
	REGULATOR_SUPPLY("8901_l4",		NULL),
};
static struct regulator_consumer_supply vreg_consumers_PM8901_L5[] = {
	REGULATOR_SUPPLY("8901_l5",		NULL),
};
#ifdef CONFIG_LGE_PMIC8058_REGULATOR
/* we don't use */
#else
static struct regulator_consumer_supply vreg_consumers_PM8901_L6[] = {
	REGULATOR_SUPPLY("8901_l6",		NULL),
};
static struct regulator_consumer_supply vreg_consumers_PM8901_S2[] = {
	REGULATOR_SUPPLY("8901_s2",		NULL),
};
#endif

static struct regulator_consumer_supply vreg_consumers_PM8901_S3[] = {
	REGULATOR_SUPPLY("8901_s3",		NULL),
};
static struct regulator_consumer_supply vreg_consumers_PM8901_S4[] = {
	REGULATOR_SUPPLY("8901_s4",		NULL),
};
static struct regulator_consumer_supply vreg_consumers_PM8901_LVS0[] = {
	REGULATOR_SUPPLY("8901_lvs0",		NULL),
};
static struct regulator_consumer_supply vreg_consumers_PM8901_LVS1[] = {
	REGULATOR_SUPPLY("8901_lvs1",		NULL),
};
static struct regulator_consumer_supply vreg_consumers_PM8901_LVS2[] = {
	REGULATOR_SUPPLY("8901_lvs2",		NULL),
};
static struct regulator_consumer_supply vreg_consumers_PM8901_LVS3[] = {
	REGULATOR_SUPPLY("8901_lvs3",		NULL),
};
static struct regulator_consumer_supply vreg_consumers_PM8901_MVS0[] = {
	REGULATOR_SUPPLY("8901_mvs0",		NULL),
};

/* Pin control regulators */
static struct regulator_consumer_supply vreg_consumers_PM8058_L8_PC[] = {
	REGULATOR_SUPPLY("8058_l8_pc",		NULL),
};
static struct regulator_consumer_supply vreg_consumers_PM8058_L20_PC[] = {
	REGULATOR_SUPPLY("8058_l20_pc",		NULL),
};
static struct regulator_consumer_supply vreg_consumers_PM8058_L21_PC[] = {
	REGULATOR_SUPPLY("8058_l21_pc",		NULL),
};
static struct regulator_consumer_supply vreg_consumers_PM8058_S2_PC[] = {
	REGULATOR_SUPPLY("8058_s2_pc",		NULL),
};

#ifdef CONFIG_LGE_PMIC8058_REGULATOR
/* we don't use */
#else
static struct regulator_consumer_supply vreg_consumers_PM8901_L0_PC[] = {
	REGULATOR_SUPPLY("8901_l0_pc",		NULL),
};
#endif

static struct regulator_consumer_supply vreg_consumers_PM8901_S4_PC[] = {
	REGULATOR_SUPPLY("8901_s4_pc",		NULL),
};

#define RPM_VREG_INIT(_id, _min_uV, _max_uV, _modes, _ops, _apply_uV, \
		      _default_uV, _peak_uA, _avg_uA, _pull_down, _pin_ctrl, \
		      _freq, _pin_fn, _force_mode, _state, _sleep_selectable, \
		      _always_on) \
	{ \
		.init_data = { \
			.constraints = { \
				.valid_modes_mask = _modes, \
				.valid_ops_mask = _ops, \
				.min_uV = _min_uV, \
				.max_uV = _max_uV, \
				.input_uV = _min_uV, \
				.apply_uV = _apply_uV, \
				.always_on = _always_on, \
			}, \
			.consumer_supplies = vreg_consumers_##_id, \
			.num_consumer_supplies = \
				ARRAY_SIZE(vreg_consumers_##_id), \
		}, \
		.id			= RPM_VREG_ID_##_id, \
		.default_uV		= _default_uV, \
		.peak_uA		= _peak_uA, \
		.avg_uA			= _avg_uA, \
		.pull_down_enable	= _pull_down, \
		.pin_ctrl		= _pin_ctrl, \
		.freq			= RPM_VREG_FREQ_##_freq, \
		.pin_fn			= _pin_fn, \
		.force_mode		= _force_mode, \
		.state			= _state, \
		.sleep_selectable	= _sleep_selectable, \
	}

/* Pin control initialization */
#define RPM_PC(_id, _always_on, _pin_fn, _pin_ctrl) \
	{ \
		.init_data = { \
			.constraints = { \
				.valid_ops_mask	= REGULATOR_CHANGE_STATUS, \
				.always_on	= _always_on, \
			}, \
			.num_consumer_supplies	= \
					ARRAY_SIZE(vreg_consumers_##_id##_PC), \
			.consumer_supplies	= vreg_consumers_##_id##_PC, \
		}, \
		.id	  = RPM_VREG_ID_##_id##_PC, \
		.pin_fn	  = RPM_VREG_PIN_FN_8660_##_pin_fn, \
		.pin_ctrl = _pin_ctrl, \
	}

/*
 * The default LPM/HPM state of an RPM controlled regulator can be controlled
 * via the peak_uA value specified in the table below.  If the value is less
 * than the high power min threshold for the regulator, then the regulator will
 * be set to LPM.  Otherwise, it will be set to HPM.
 *
 * This value can be further overridden by specifying an initial mode via
 * .init_data.constraints.initial_mode.
 */

#define RPM_LDO(_id, _always_on, _pd, _sleep_selectable, _min_uV, _max_uV, \
		_init_peak_uA) \
	RPM_VREG_INIT(_id, _min_uV, _max_uV, REGULATOR_MODE_FAST | \
		      REGULATOR_MODE_NORMAL | REGULATOR_MODE_IDLE | \
		      REGULATOR_MODE_STANDBY, REGULATOR_CHANGE_VOLTAGE | \
		      REGULATOR_CHANGE_STATUS | REGULATOR_CHANGE_MODE | \
		      REGULATOR_CHANGE_DRMS, 0, _min_uV, _init_peak_uA, \
		      _init_peak_uA, _pd, RPM_VREG_PIN_CTRL_NONE, NONE, \
		      RPM_VREG_PIN_FN_8660_ENABLE, \
		      RPM_VREG_FORCE_MODE_8660_NONE, RPM_VREG_STATE_OFF, \
		      _sleep_selectable, _always_on)

#define RPM_SMPS(_id, _always_on, _pd, _sleep_selectable, _min_uV, _max_uV, \
		 _init_peak_uA, _freq) \
	RPM_VREG_INIT(_id, _min_uV, _max_uV, REGULATOR_MODE_FAST | \
		      REGULATOR_MODE_NORMAL | REGULATOR_MODE_IDLE | \
		      REGULATOR_MODE_STANDBY, REGULATOR_CHANGE_VOLTAGE | \
		      REGULATOR_CHANGE_STATUS | REGULATOR_CHANGE_MODE | \
		      REGULATOR_CHANGE_DRMS, 0, _min_uV, _init_peak_uA, \
		      _init_peak_uA, _pd, RPM_VREG_PIN_CTRL_NONE, _freq, \
		      RPM_VREG_PIN_FN_8660_ENABLE, \
		      RPM_VREG_FORCE_MODE_8660_NONE, RPM_VREG_STATE_OFF, \
		      _sleep_selectable, _always_on)

#define RPM_VS(_id, _always_on, _pd, _sleep_selectable) \
	RPM_VREG_INIT(_id, 0, 0, REGULATOR_MODE_NORMAL | REGULATOR_MODE_IDLE, \
		      REGULATOR_CHANGE_STATUS | REGULATOR_CHANGE_MODE, 0, 0, \
		      1000, 1000, _pd, RPM_VREG_PIN_CTRL_NONE, NONE, \
		      RPM_VREG_PIN_FN_8660_ENABLE, \
		      RPM_VREG_FORCE_MODE_8660_NONE, RPM_VREG_STATE_OFF, \
		      _sleep_selectable, _always_on)

#define RPM_NCP(_id, _always_on, _pd, _sleep_selectable, _min_uV, _max_uV) \
	RPM_VREG_INIT(_id, _min_uV, _max_uV, REGULATOR_MODE_NORMAL, \
		      REGULATOR_CHANGE_VOLTAGE | REGULATOR_CHANGE_STATUS, 0, \
		      _min_uV, 1000, 1000, _pd, RPM_VREG_PIN_CTRL_NONE, NONE, \
		      RPM_VREG_PIN_FN_8660_ENABLE, \
		      RPM_VREG_FORCE_MODE_8660_NONE, RPM_VREG_STATE_OFF, \
		      _sleep_selectable, _always_on)

#define LDO50HMIN	RPM_VREG_8660_LDO_50_HPM_MIN_LOAD
#define LDO150HMIN	RPM_VREG_8660_LDO_150_HPM_MIN_LOAD
#define LDO300HMIN	RPM_VREG_8660_LDO_300_HPM_MIN_LOAD
#define SMPS_HMIN	RPM_VREG_8660_SMPS_HPM_MIN_LOAD
#define FTS_HMIN	RPM_VREG_8660_FTSMPS_HPM_MIN_LOAD

/* RPM early regulator constraints */
static struct rpm_regulator_init_data rpm_regulator_early_init_data[] = {
	/*	 ID       a_on pd ss min_uV   max_uV   init_ip    freq */
	RPM_SMPS(PM8058_S0, 0, 1, 1,  500000, 1325000, SMPS_HMIN, 1p60),
	RPM_SMPS(PM8058_S1, 0, 1, 1,  500000, 1250000, SMPS_HMIN, 1p60),
};

/* RPM regulator constraints */
static struct rpm_regulator_init_data rpm_regulator_init_data[] = {
#ifdef CONFIG_LGE_PMIC8058_REGULATOR
	/*	ID        a_on pd ss min_uV   max_uV   init_ip */
	RPM_LDO(PM8058_L0,  0, 1, 0, 1200000, 1200000, LDO150HMIN), /* +1V2_MIPI */
	RPM_LDO(PM8058_L1,  0, 1, 0, 1200000, 1200000, LDO300HMIN), /* +1V2_CAM_DIGITAL */
//	RPM_LDO(PM8058_L2,  0, 1, 0, 1800000, 2600000, LDO300HMIN), /* 4.7uF CAP -GND */
	RPM_LDO(PM8058_L3,  0, 1, 0, 3300000, 3300000, LDO150HMIN), /* +3V3_MHL */
	RPM_LDO(PM8058_L4,  0, 1, 0, 2850000, 2850000,  LDO50HMIN), /* +2V85_TCXO */
	RPM_LDO(PM8058_L5,  0, 1, 0, 2850000, 2850000, LDO300HMIN),
#ifdef CONFIG_USB_G_LGE_ANDROID_FACTORY
    /* bump up maxV to 3.5v for usb1.1 full-speed */
	RPM_LDO(PM8058_L6,  0, 1, 0, 3050000, 3500000,  LDO50HMIN),
#else
	RPM_LDO(PM8058_L6,  0, 1, 0, 3050000, 3050000,  LDO50HMIN), /* +3V075_USB */
#endif
	RPM_LDO(PM8058_L7,  0, 1, 0, 1800000, 1800000,  LDO50HMIN), /* +1V8_USB */
	RPM_LDO(PM8058_L8,  0, 1, 0, 2800000, 2800000, LDO300HMIN), /* +2V8_CAM_VCM */
	RPM_LDO(PM8058_L9,  0, 1, 0, 2800000, 2800000, LDO300HMIN), /* +2V8_CAM_AVDD */
//	RPM_LDO(PM8058_L10, 0, 1, 0, 2600000, 2600000, LDO300HMIN), /*4.7uF CAP - GND*/
	RPM_LDO(PM8058_L11, 0, 1, 0, 3000000, 3000000, LDO150HMIN), /* +3V0_SENSORS */
	RPM_LDO(PM8058_L12, 0, 1, 0, 1800000, 1800000, LDO150HMIN), /* +1V8_MHL */
	RPM_LDO(PM8058_L13, 0, 1, 0, 2050000, 2050000, LDO300HMIN), /* +2V05_MSM_ANALOG */
	RPM_LDO(PM8058_L14, 0, 0, 0, 2950000, 2950000, LDO300HMIN),
#if defined(CONFIG_MACH_LGE_325_BOARD_VZW) || defined(CONFIG_MACH_LGE_325_BOARD_LGU)
	RPM_LDO(PM8058_L15, 0, 1, 0, 2800000, 2800000, LDO300HMIN), /* +2V85_PROXIMITY */
#else
	RPM_LDO(PM8058_L15, 0, 1, 0, 2850000, 2850000, LDO300HMIN), /* +2V85_PROXIMITY */
#endif
	RPM_LDO(PM8058_L16, 1, 1, 1, 1800000, 1800000, LDO300HMIN), /* +1V8_MSM_DIGITAL */
	RPM_LDO(PM8058_L17, 0, 1, 0, 2850000, 2850000, LDO150HMIN), /* +2V85_RF_SW */
	RPM_LDO(PM8058_L18, 0, 1, 1, 2200000, 2200000, LDO150HMIN), /* +2V2_HKADC */
#if defined(CONFIG_LGE_TOUCH_SYNAPTICS_325) && defined(CONFIG_MACH_LGE_325_BOARD)
	RPM_LDO(PM8058_L19, 0, 1, 0, 3000000, 3000000, LDO150HMIN),
#else
	RPM_LDO(PM8058_L19, 0, 1, 0, 2850000, 2850000, LDO150HMIN), /* +2V85_TOUCH */
#endif
#if defined(CONFIG_MACH_LGE_325_BOARD_VZW) || defined(CONFIG_MACH_LGE_325_BOARD_LGU) || defined(CONFIG_MACH_LGE_325_BOARD_DCM)
	RPM_LDO(PM8058_L20, 0, 1, 0, 1800000, 1800000, LDO150HMIN), /* +1V8_RF_XO */
#else
	RPM_LDO(PM8058_L20, 0, 1, 0, 0, 0, LDO150HMIN),
#endif
	RPM_LDO(PM8058_L21, 1, 1, 0, 1100000, 1100000, LDO150HMIN), /* +1V1_MSM_PLL */
	RPM_LDO(PM8058_L22, 0, 1, 0, 1200000, 1200000, LDO300HMIN), /* +1V2_DDR_EBI */
	RPM_LDO(PM8058_L23, 0, 1, 0, 1200000, 1200000, LDO300HMIN), /* +1V2_MHL */
//	RPM_LDO(PM8058_L24, 0, 1, 0, 0, 0, LDO150HMIN), /*4.7uF CAP - GND*/
//	RPM_LDO(PM8058_L25, 0, 1, 0, 0, 0, LDO150HMIN), /*4.7uF CAP - GND*/

	/*	 ID       a_on pd ss min_uV   max_uV   init_ip    freq */
	RPM_SMPS(PM8058_S2, 0, 1, 1, 1200000, 1400000,  SMPS_HMIN, 1p60), /* +1V3_RF */
	RPM_SMPS(PM8058_S3, 1, 1, 0, 1800000, 1800000,  SMPS_HMIN, 1p60), /* +1V8_MSM_EBI */
	RPM_SMPS(PM8058_S4, 1, 1, 0, 2200000, 2200000,  SMPS_HMIN, 1p60), /* +2V2_RF */

	RPM_VS(PM8058_LVS0, 0, 1, 0), /* +1.8V_CAM_VDDIO */
	RPM_VS(PM8058_LVS1, 0, 1, 0), /* +1V8_MSM_QFUSE */

//	RPM_NCP(PM8058_NCP, 0, 1, 0, 1800000, 1800000), /* -1V8_NCP */ /* we don't use */

//	RPM_LDO(PM8901_L0,  0, 1, 0, 0, 0, LDO300HMIN), /* NC */
	RPM_LDO(PM8901_L1,  0, 1, 0, 3000000, 3000000, LDO300HMIN), /* +3V0_MOTOR */ //sungwoo.cho@lge.com
	RPM_LDO(PM8901_L2,  0, 1, 0, 3000000, 3000000, LDO300HMIN), /* +3V0_LCD_VCC */
#if defined(CONFIG_LGE_DISPLAY_MIPI_HITACHI_VIDEO_HD_PT) && defined(CONFIG_MACH_LGE_325_BOARD)
	RPM_LDO(PM8901_L3,  0, 1, 0, 3000000, 3000000, LDO300HMIN),
#else
	/* LGE_CHANGE : jinho.jang 2011.03.07 - HD LCD */
	RPM_LDO(PM8901_L3,  0, 1, 0, 2850000, 3000000, LDO300HMIN), /* +3V0_LCD_VCI */
//	RPM_LDO(PM8901_L3,  0, 1, 0, 3000000, 3000000, LDO300HMIN), /* +3V0_LCD_VCI */
#endif
	RPM_LDO(PM8901_L4,  0, 1, 0, 2800000, 2800000, LDO300HMIN), /* 2V8_VTCAM */
	RPM_LDO(PM8901_L5,  0, 1, 0, 2850000, 2850000, LDO300HMIN), /* +2V85_eMMC_CORE */
//	RPM_LDO(PM8901_L6,  0, 1, 0, 0, 0, LDO300HMIN), /* NC */

//	RPM_SMPS(PM8901_S2, 0, 1, 0, 0, 0,   FTS_HMIN, 1p60), /* NC */
	RPM_SMPS(PM8901_S3, 0, 1, 0, 1100000, 1100000,   FTS_HMIN, 1p60), /* +1V1_MSM_DSP */
	RPM_SMPS(PM8901_S4, 0, 1, 0, 1200000, 1200000,   FTS_HMIN, 1p60), /* +1V2_DDR_CORE */

	RPM_VS(PM8901_LVS0, 1, 1, 0), /* +1V8_eMMC_VIO */
	RPM_VS(PM8901_LVS1, 0, 1, 0), /* +1V8_VTCAM_IOVDD */
#ifdef CONFIG_LGE_PM_CURRENT_CONSUMPTION_FIX
/* kiwone.seo,20110626, LVS2's LDO no pull down @sleep */
	RPM_VS(PM8901_LVS2, 0, 0, 0),	 /* +1V8_TOUCH_VIO */
#else
	RPM_VS(PM8901_LVS2, 0, 1, 0),    /* +1V8_TOUCH_VIO */
#endif
	RPM_VS(PM8901_LVS3, 0, 1, 0),    /* 1V8_VTCAM_DVDD */ //jisun.shin@lge.com
	RPM_VS(PM8901_MVS0, 0, 1, 0), /* +1V8_LCD_VIO */

		/*     ID         a_on pin_func pin_ctrl */
	RPM_PC(PM8058_L8,   0, SLEEP_B, RPM_VREG_PIN_CTRL_NONE), /* +2V8_CAM_VCM */
	RPM_PC(PM8058_L20,  0, SLEEP_B, RPM_VREG_PIN_CTRL_NONE), /* +1V8_RF_XO */
	RPM_PC(PM8058_L21,  1, SLEEP_B, RPM_VREG_PIN_CTRL_NONE), /* +1V1_MSM_PLL */
	RPM_PC(PM8058_S2,   0, ENABLE,  RPM_VREG_PIN_CTRL_PM8058_A0), /* +1V3_RF */
//	RPM_PC(PM8901_L0,   0, ENABLE,  RPM_VREG_PIN_CTRL_PM8901_A0), /* NC */
	RPM_PC(PM8901_S4,   0, ENABLE,  RPM_VREG_PIN_CTRL_PM8901_A0), /* +1V2_DDR_CORE */

#else // original
	/*	ID        a_on pd ss min_uV   max_uV   init_ip */
	RPM_LDO(PM8058_L0,  0, 1, 0, 1200000, 1200000, LDO150HMIN),
	RPM_LDO(PM8058_L1,  0, 1, 0, 1200000, 1200000, LDO300HMIN),
	RPM_LDO(PM8058_L2,  0, 1, 0, 1800000, 2600000, LDO300HMIN),
	RPM_LDO(PM8058_L3,  0, 1, 0, 1800000, 1800000, LDO150HMIN),
	RPM_LDO(PM8058_L4,  0, 1, 0, 2850000, 2850000,  LDO50HMIN),
	RPM_LDO(PM8058_L5,  0, 1, 0, 2850000, 2850000, LDO300HMIN),
	RPM_LDO(PM8058_L6,  0, 1, 0, 3000000, 3600000,  LDO50HMIN),
	RPM_LDO(PM8058_L7,  0, 1, 0, 1800000, 1800000,  LDO50HMIN),
	RPM_LDO(PM8058_L8,  0, 1, 0, 2900000, 3050000, LDO300HMIN),
	RPM_LDO(PM8058_L9,  0, 1, 0, 1800000, 1800000, LDO300HMIN),
	RPM_LDO(PM8058_L10, 0, 1, 0, 2600000, 2600000, LDO300HMIN),
	RPM_LDO(PM8058_L11, 0, 1, 0, 1500000, 1500000, LDO150HMIN),
	RPM_LDO(PM8058_L12, 0, 1, 0, 2900000, 2900000, LDO150HMIN),
	RPM_LDO(PM8058_L13, 0, 1, 0, 2050000, 2050000, LDO300HMIN),
	RPM_LDO(PM8058_L14, 0, 0, 0, 2850000, 2850000, LDO300HMIN),
	RPM_LDO(PM8058_L15, 0, 1, 0, 2850000, 2850000, LDO300HMIN),
	RPM_LDO(PM8058_L16, 1, 1, 0, 1800000, 1800000, LDO300HMIN),
	RPM_LDO(PM8058_L17, 0, 1, 0, 2600000, 2600000, LDO150HMIN),
	RPM_LDO(PM8058_L18, 0, 1, 0, 2200000, 2200000, LDO150HMIN),
	RPM_LDO(PM8058_L19, 0, 1, 0, 2500000, 2500000, LDO150HMIN),
	RPM_LDO(PM8058_L20, 0, 1, 0, 1800000, 1800000, LDO150HMIN),
	RPM_LDO(PM8058_L21, 1, 1, 0, 1200000, 1200000, LDO150HMIN),
	RPM_LDO(PM8058_L22, 0, 1, 0, 1150000, 1150000, LDO300HMIN),
	RPM_LDO(PM8058_L23, 0, 1, 0, 1200000, 1200000, LDO300HMIN),
	RPM_LDO(PM8058_L24, 0, 1, 0, 1200000, 1200000, LDO150HMIN),
	RPM_LDO(PM8058_L25, 0, 1, 0, 1200000, 1200000, LDO150HMIN),

	/*	 ID       a_on pd ss min_uV   max_uV   init_ip    freq */
	RPM_SMPS(PM8058_S2, 0, 1, 1, 1200000, 1400000, SMPS_HMIN, 1p60),
	RPM_SMPS(PM8058_S3, 1, 1, 0, 1800000, 1800000, SMPS_HMIN, 1p60),
	RPM_SMPS(PM8058_S4, 1, 1, 0, 2200000, 2200000, SMPS_HMIN, 1p60),

	/*     ID         a_on pd ss */
	RPM_VS(PM8058_LVS0, 0, 1, 0),
	RPM_VS(PM8058_LVS1, 0, 1, 0),

	/*	ID        a_on pd ss min_uV   max_uV */
	RPM_NCP(PM8058_NCP, 0, 1, 0, 1800000, 1800000),

	/*	ID        a_on pd ss min_uV   max_uV   init_ip */
	RPM_LDO(PM8901_L0,  0, 1, 0, 1200000, 1200000, LDO300HMIN),
	RPM_LDO(PM8901_L1,  0, 1, 0, 3300000, 3300000, LDO300HMIN),
	RPM_LDO(PM8901_L2,  0, 1, 0, 2850000, 3300000, LDO300HMIN),
	RPM_LDO(PM8901_L3,  0, 1, 0, 3300000, 3300000, LDO300HMIN),
	RPM_LDO(PM8901_L4,  0, 1, 0, 2600000, 2600000, LDO300HMIN),
	RPM_LDO(PM8901_L5,  0, 1, 0, 2850000, 2850000, LDO300HMIN),
	RPM_LDO(PM8901_L6,  0, 1, 0, 2200000, 2200000, LDO300HMIN),

	/*	 ID       a_on pd ss min_uV   max_uV   init_ip   freq */
	RPM_SMPS(PM8901_S2, 0, 1, 0, 1300000, 1300000, FTS_HMIN, 1p60),
	RPM_SMPS(PM8901_S3, 0, 1, 0, 1100000, 1100000, FTS_HMIN, 1p60),
	RPM_SMPS(PM8901_S4, 0, 1, 0, 1225000, 1225000, FTS_HMIN, 1p60),

	/*	ID        a_on pd ss */
	RPM_VS(PM8901_LVS0, 1, 1, 0),
	RPM_VS(PM8901_LVS1, 0, 1, 0),
	RPM_VS(PM8901_LVS2, 0, 1, 0),
	RPM_VS(PM8901_LVS3, 0, 1, 0),
	RPM_VS(PM8901_MVS0, 0, 1, 0),

	/*     ID         a_on pin_func pin_ctrl */
	RPM_PC(PM8058_L8,   0, SLEEP_B, RPM_VREG_PIN_CTRL_NONE),
	RPM_PC(PM8058_L20,  0, SLEEP_B, RPM_VREG_PIN_CTRL_NONE),
	RPM_PC(PM8058_L21,  1, SLEEP_B, RPM_VREG_PIN_CTRL_NONE),
	RPM_PC(PM8058_S2,   0, ENABLE,  RPM_VREG_PIN_CTRL_PM8058_A0),
	RPM_PC(PM8901_L0,   0, ENABLE,  RPM_VREG_PIN_CTRL_PM8901_A0),
	RPM_PC(PM8901_S4,   0, ENABLE,  RPM_VREG_PIN_CTRL_PM8901_A0),

#endif

};

static struct rpm_regulator_platform_data rpm_regulator_early_pdata = {
	.init_data		= rpm_regulator_early_init_data,
	.num_regulators		= ARRAY_SIZE(rpm_regulator_early_init_data),
	.version		= RPM_VREG_VERSION_8660,
	.vreg_id_vdd_mem	= RPM_VREG_ID_PM8058_S0,
	.vreg_id_vdd_dig	= RPM_VREG_ID_PM8058_S1,
};

static struct rpm_regulator_platform_data rpm_regulator_pdata = {
	.init_data		= rpm_regulator_init_data,
	.num_regulators		= ARRAY_SIZE(rpm_regulator_init_data),
	.version		= RPM_VREG_VERSION_8660,
};

static struct platform_device rpm_regulator_early_device = {
	.name	= "rpm-regulator",
	.id	= 0,
	.dev	= {
		.platform_data = &rpm_regulator_early_pdata,
	},
};

static struct platform_device rpm_regulator_device = {
	.name	= "rpm-regulator",
	.id	= 1,
	.dev	= {
		.platform_data = &rpm_regulator_pdata,
	},
};

static struct platform_device *early_regulators[] __initdata = {
	&msm_device_saw_s0,
	&msm_device_saw_s1,
	&rpm_regulator_early_device,
};

static struct platform_device *early_devices[] __initdata = {
#ifdef CONFIG_MSM_BUS_SCALING
	&msm_bus_apps_fabric,
	&msm_bus_sys_fabric,
	&msm_bus_mm_fabric,
	&msm_bus_sys_fpb,
	&msm_bus_cpss_fpb,
#endif
	&msm_device_dmov_adm0,
	&msm_device_dmov_adm1,
};

static struct platform_device msm_tsens_device = {
	.name   = "tsens-tm",
	.id = -1,
};

#ifdef CONFIG_LGE_HALLIC_CARKIT
static struct platform_device hallic_dock_device = {
	.name   = "hall-ic-dock",
	.id = -1,
	.dev = {
		.platform_data = NULL,
	},
};
#endif

#ifdef CONFIG_SENSORS_MSM_ADC
static struct adc_access_fn xoadc_fn = {
	pm8058_xoadc_select_chan_and_start_conv,
	pm8058_xoadc_read_adc_code,
	pm8058_xoadc_get_properties,
	pm8058_xoadc_slot_request,
	pm8058_xoadc_restore_slot,
	pm8058_xoadc_calibrate,
};

static struct msm_adc_channels msm_adc_channels_data[] = {
	{"vbatt", CHANNEL_ADC_VBATT, 0, &xoadc_fn, CHAN_PATH_TYPE2,
		ADC_CONFIG_TYPE2, ADC_CALIB_CONFIG_TYPE3, scale_default},
	{"vcoin", CHANNEL_ADC_VCOIN, 0, &xoadc_fn, CHAN_PATH_TYPE1,
		ADC_CONFIG_TYPE2, ADC_CALIB_CONFIG_TYPE2, scale_default},
	{"vcharger_channel", CHANNEL_ADC_VCHG, 0, &xoadc_fn, CHAN_PATH_TYPE3,
		ADC_CONFIG_TYPE2, ADC_CALIB_CONFIG_TYPE4, scale_default},
	{"charger_current_monitor", CHANNEL_ADC_CHG_MONITOR, 0, &xoadc_fn,
		CHAN_PATH_TYPE4,
		ADC_CONFIG_TYPE2, ADC_CALIB_CONFIG_TYPE1, scale_default},
	{"vph_pwr", CHANNEL_ADC_VPH_PWR, 0, &xoadc_fn, CHAN_PATH_TYPE5,
		ADC_CONFIG_TYPE2, ADC_CALIB_CONFIG_TYPE3, scale_default},
	{"usb_vbus", CHANNEL_ADC_USB_VBUS, 0, &xoadc_fn, CHAN_PATH_TYPE11,
		ADC_CONFIG_TYPE2, ADC_CALIB_CONFIG_TYPE3, scale_default},
	{"pmic_therm", CHANNEL_ADC_DIE_TEMP, 0, &xoadc_fn, CHAN_PATH_TYPE12,
		ADC_CONFIG_TYPE2, ADC_CALIB_CONFIG_TYPE1, scale_pmic_therm},
	{"pmic_therm_4K", CHANNEL_ADC_DIE_TEMP_4K, 0, &xoadc_fn,
		CHAN_PATH_TYPE12,
		ADC_CONFIG_TYPE1, ADC_CALIB_CONFIG_TYPE7, scale_pmic_therm},
	{"xo_therm", CHANNEL_ADC_XOTHERM, 0, &xoadc_fn, CHAN_PATH_TYPE_NONE,
		ADC_CONFIG_TYPE2, ADC_CALIB_CONFIG_TYPE5, tdkntcgtherm},
	{"xo_therm_4K", CHANNEL_ADC_XOTHERM_4K, 0, &xoadc_fn,
		CHAN_PATH_TYPE_NONE,
		ADC_CONFIG_TYPE1, ADC_CALIB_CONFIG_TYPE6, tdkntcgtherm},
#ifdef CONFIG_LGE_PMIC8058_MPP
#if defined(CONFIG_MACH_LGE_325_BOARD_VZW) || defined(CONFIG_MACH_LGE_325_BOARD_LGU)
	{"pcb_revision", CHANNEL_ADC_PCB_REVISION, 0, &xoadc_fn, CHAN_PATH_TYPE9,
		ADC_CONFIG_TYPE2, ADC_CALIB_CONFIG_TYPE2, scale_default},
#else
	{"pcb_revision", CHANNEL_ADC_PCB_REVISION, 0, &xoadc_fn, CHAN_PATH_TYPE9,
		ADC_CONFIG_TYPE2, ADC_CALIB_CONFIG_TYPE1, scale_default},
#endif
	{"msm_therm", CHANNEL_ADC_MSM_THERM, 0, &xoadc_fn, CHAN_PATH_TYPE8,
		ADC_CONFIG_TYPE2, ADC_CALIB_CONFIG_TYPE2, scale_msm_therm},
	{"batt_therm", CHANNEL_ADC_BATT_THERM, 0, &xoadc_fn, CHAN_PATH_TYPE7,
		ADC_CONFIG_TYPE2, ADC_CALIB_CONFIG_TYPE2, scale_batt_therm},
#if defined(CONFIG_MACH_LGE_325_BOARD_VZW) || defined(CONFIG_MACH_LGE_325_BOARD_LGU)
	{"acc_adc", CHANNEL_ADC_ACC, 0, &xoadc_fn, CHAN_PATH_TYPE6,
		ADC_CONFIG_TYPE2, ADC_CALIB_CONFIG_TYPE1, scale_default},
#else
	{"acc_adc", CHANNEL_ADC_ACC, 0, &xoadc_fn, CHAN_PATH_TYPE6,
		ADC_CONFIG_TYPE2, ADC_CALIB_CONFIG_TYPE2, scale_default},
#endif
	{"wireless_current", CHANNEL_ADC_WIRELESS_CURRENT, 0, &xoadc_fn, CHAN_PATH_TYPE10,
		ADC_CONFIG_TYPE2, ADC_CALIB_CONFIG_TYPE1, scale_xtern_chgr_cur},
#else
	{"hdset_detect", CHANNEL_ADC_HDSET, 0, &xoadc_fn, CHAN_PATH_TYPE6,
		ADC_CONFIG_TYPE2, ADC_CALIB_CONFIG_TYPE1, scale_default},
	{"chg_batt_amon", CHANNEL_ADC_BATT_AMON, 0, &xoadc_fn, CHAN_PATH_TYPE10,
		ADC_CONFIG_TYPE2, ADC_CALIB_CONFIG_TYPE1,
		scale_xtern_chgr_cur},
	{"msm_therm", CHANNEL_ADC_MSM_THERM, 0, &xoadc_fn, CHAN_PATH_TYPE8,
		ADC_CONFIG_TYPE2, ADC_CALIB_CONFIG_TYPE2, scale_msm_therm},
	{"batt_therm", CHANNEL_ADC_BATT_THERM, 0, &xoadc_fn, CHAN_PATH_TYPE7,
		ADC_CONFIG_TYPE2, ADC_CALIB_CONFIG_TYPE2, scale_batt_therm},
	{"batt_id", CHANNEL_ADC_BATT_ID, 0, &xoadc_fn, CHAN_PATH_TYPE9,
		ADC_CONFIG_TYPE2, ADC_CALIB_CONFIG_TYPE2, scale_default},
#endif
	{"ref_625mv", CHANNEL_ADC_625_REF, 0, &xoadc_fn, CHAN_PATH_TYPE15,
		ADC_CONFIG_TYPE2, ADC_CALIB_CONFIG_TYPE2, scale_default},
	{"ref_1250mv", CHANNEL_ADC_1250_REF, 0, &xoadc_fn, CHAN_PATH_TYPE13,
		ADC_CONFIG_TYPE2, ADC_CALIB_CONFIG_TYPE2, scale_default},
	{"ref_325mv", CHANNEL_ADC_325_REF, 0, &xoadc_fn, CHAN_PATH_TYPE14,
		ADC_CONFIG_TYPE2, ADC_CALIB_CONFIG_TYPE2, scale_default},
};

static struct msm_adc_platform_data msm_adc_pdata = {
	.channel = msm_adc_channels_data,
	.num_chan_supported = ARRAY_SIZE(msm_adc_channels_data),
};

static struct platform_device msm_adc_device = {
	.name   = "msm_adc",
	.id = -1,
	.dev = {
		.platform_data = &msm_adc_pdata,
	},
};

#ifdef CONFIG_LGE_PMIC8058_MPP
#if 0
static void pmic8058_xoadc_mpp_config(void)
{
	int rc;

#ifdef CONFIG_LGE_MACH_BOARD_REVB
	rc = pm8901_mpp_config_digital_out(XOADC_MPP_4,
			PM8901_MPP_DIG_LEVEL_S4, PM_MPP_DOUT_CTL_LOW);
	if (rc)
		pr_err("%s: Config mpp4 on pmic 8901 failed\n", __func__);
#endif
	rc = pm8058_mpp_config_analog_input(XOADC_MPP_3/*PCB_REVISION_ADC*/,
			PM_MPP_AIN_AMUX_CH8, PM_MPP_AOUT_CTL_DISABLE);
	if (rc)
		pr_err("%s: Config mpp3 on pmic 8058 failed\n", __func__);

#if 0 /* CSFB not used */
	rc = pm8058_mpp_config_analog_output(XOADC_MPP_6/*GSM_VREF*/,
			PM_MPP_AOUT_LVL_1V25, PM_MPP_AOUT_CTL_ENABLE);
	if (rc)
		pr_err("%s: Config mpp6 on pmic 8058 failed\n", __func__);
#endif
	rc = pm8058_mpp_config_analog_input(XOADC_MPP_7/*BATTERY_THERM_ADC_IN*/,
			PM_MPP_AIN_AMUX_CH6, PM_MPP_AOUT_CTL_DISABLE);
	if (rc)
		pr_err("%s: Config mpp7 on pmic 8058 failed\n", __func__);

	rc = pm8058_mpp_config_analog_input(XOADC_MPP_10/*MSM_THERM_ADC*/,
			PM_MPP_AIN_AMUX_CH7, PM_MPP_AOUT_CTL_DISABLE);
	if (rc)
		pr_err("%s: Config mpp10 on pmic 8058 failed\n", __func__);

#if 0 /* CSFB not used */
	rc = pm8058_mpp_config_analog_input(XOADC_MPP_4/*WIRELESS_CURRENT_ADC*/,
			PM_MPP_AIN_AMUX_CH9, PM_MPP_AOUT_CTL_DISABLE);
	if (rc)
		pr_err("%s: Config mpp4 on pmic 8058 failed\n", __func__);
#endif
	rc = pm8058_mpp_config_analog_input(XOADC_MPP_8/*ACC_ADC*/,
			PM_MPP_AIN_AMUX_CH5, PM_MPP_AOUT_CTL_DISABLE);
	if (rc)
		pr_err("%s: Config mpp8 on pmic 8058 failed\n", __func__);
}
#endif
static void pmic8058_xoadc_mpp_config(void)
{
	int rc, i;
	struct pm8xxx_mpp_init_info xoadc_mpps[] = {
		PM8058_MPP_INIT(XOADC_MPP_3, A_INPUT, PM8XXX_MPP_AIN_AMUX_CH8,
							AOUT_CTRL_DISABLE), /*PCB_REVISION_ADC*/
#ifdef CONFIG_MACH_LGE_325_BOARD
		/* batman do not use wireless charger */
#else
		PM8058_MPP_INIT(XOADC_MPP_4, A_INPUT, PM8XXX_MPP_AIN_AMUX_CH9,
							AOUT_CTRL_DISABLE), /*WIRELESS_CURRENT_ADC*/
#endif
#if defined(CONFIG_MACH_LGE_325_BOARD_VZW) || defined(CONFIG_MACH_LGE_325_BOARD_LGU)
		PM8058_MPP_INIT(XOADC_MPP_6, A_OUTPUT, PM8XXX_MPP_AOUT_LVL_1V25,
							AOUT_CTRL_DISABLE), /*GSM_VREF*/
#endif
		PM8058_MPP_INIT(XOADC_MPP_7, A_INPUT, PM8XXX_MPP_AIN_AMUX_CH6,
							AOUT_CTRL_DISABLE), /*BATTERY_THERM_ADC_IN*/
		PM8058_MPP_INIT(XOADC_MPP_8, A_INPUT, PM8XXX_MPP_AIN_AMUX_CH5,
							AOUT_CTRL_DISABLE), /*ACC_ADC*/
		PM8058_MPP_INIT(XOADC_MPP_10, A_INPUT, PM8XXX_MPP_AIN_AMUX_CH7,
							AOUT_CTRL_DISABLE), /*MSM_THERM_ADC*/
	};

	for (i = 0; i < ARRAY_SIZE(xoadc_mpps); i++) {
		rc = pm8xxx_mpp_config(xoadc_mpps[i].mpp,
					&xoadc_mpps[i].config);
		if (rc) {
			pr_err("%s: Config MPP %d of PM8058 failed\n",
					__func__, xoadc_mpps[i].mpp);
		}
	}
}

#else
static void pmic8058_xoadc_mpp_config(void)
{
	int rc, i;
	struct pm8xxx_mpp_init_info xoadc_mpps[] = {
		PM8058_MPP_INIT(XOADC_MPP_3, A_INPUT, PM8XXX_MPP_AIN_AMUX_CH5,
							AOUT_CTRL_DISABLE),
		PM8058_MPP_INIT(XOADC_MPP_5, A_INPUT, PM8XXX_MPP_AIN_AMUX_CH9,
							AOUT_CTRL_DISABLE),
		PM8058_MPP_INIT(XOADC_MPP_7, A_INPUT, PM8XXX_MPP_AIN_AMUX_CH6,
							AOUT_CTRL_DISABLE),
		PM8058_MPP_INIT(XOADC_MPP_8, A_INPUT, PM8XXX_MPP_AIN_AMUX_CH8,
							AOUT_CTRL_DISABLE),
		PM8058_MPP_INIT(XOADC_MPP_10, A_INPUT, PM8XXX_MPP_AIN_AMUX_CH7,
							AOUT_CTRL_DISABLE),
		PM8901_MPP_INIT(XOADC_MPP_4, D_OUTPUT, PM8901_MPP_DIG_LEVEL_S4,
							DOUT_CTRL_LOW),
	};

	for (i = 0; i < ARRAY_SIZE(xoadc_mpps); i++) {
		rc = pm8xxx_mpp_config(xoadc_mpps[i].mpp,
					&xoadc_mpps[i].config);
		if (rc) {
			pr_err("%s: Config MPP %d of PM8058 failed\n",
					__func__, xoadc_mpps[i].mpp);
		}
	}
}
#endif

static struct regulator *vreg_ldo18_adc;

static int pmic8058_xoadc_vreg_config(int on)
{
	int rc;

	if (on) {
		rc = regulator_enable(vreg_ldo18_adc);
		if (rc)
			pr_err("%s: Enable of regulator ldo18_adc "
						"failed\n", __func__);
	} else {
#ifdef CONFIG_LGE_PM_CURRENT_CABLE_TYPE
/* kiwone.seo@lge.com,  the adc voltage doesn't have a margine in time, so we always on,
   but, it must be turned off in sleep. check after.
*/
		rc = 0;
#else
		rc = regulator_disable(vreg_ldo18_adc);
		if (rc)
			pr_err("%s: Disable of regulator ldo18_adc "
						"failed\n", __func__);
#endif
	}

	return rc;
}

static int pmic8058_xoadc_vreg_setup(void)
{
	int rc;

	vreg_ldo18_adc = regulator_get(NULL, "8058_l18");
	if (IS_ERR(vreg_ldo18_adc)) {
		printk(KERN_ERR "%s: vreg get failed (%ld)\n",
			__func__, PTR_ERR(vreg_ldo18_adc));
		rc = PTR_ERR(vreg_ldo18_adc);
		goto fail;
	}

	rc = regulator_set_voltage(vreg_ldo18_adc, 2200000, 2200000);
	if (rc) {
		pr_err("%s: unable to set ldo18 voltage to 2.2V\n", __func__);
		goto fail;
	}

	return rc;
fail:
	regulator_put(vreg_ldo18_adc);
	return rc;
}

static void pmic8058_xoadc_vreg_shutdown(void)
{
	regulator_put(vreg_ldo18_adc);
}

/* usec. For this ADC,
 * this time represents clk rate @ txco w/ 1024 decimation ratio.
 * Each channel has different configuration, thus at the time of starting
 * the conversion, xoadc will return actual conversion time
 * */
static struct adc_properties pm8058_xoadc_data = {
	.adc_reference          = 2200, /* milli-voltage for this adc */
	.bitresolution         = 15,
	.bipolar                = 0,
	.conversiontime         = 54,
};

static struct xoadc_platform_data pm8058_xoadc_pdata = {
	.xoadc_prop = &pm8058_xoadc_data,
	.xoadc_mpp_config = pmic8058_xoadc_mpp_config,
	.xoadc_vreg_set = pmic8058_xoadc_vreg_config,
	.xoadc_num = XOADC_PMIC_0,
	.xoadc_vreg_setup = pmic8058_xoadc_vreg_setup,
	.xoadc_vreg_shutdown = pmic8058_xoadc_vreg_shutdown,
};
#endif

#ifdef CONFIG_MSM_SDIO_AL

static unsigned mdm2ap_status = 140;

static int configure_mdm2ap_status(int on)
{
	int ret = 0;
	if (on)
		ret = msm_gpiomux_get(mdm2ap_status);
	else
		ret = msm_gpiomux_put(mdm2ap_status);

	if (ret)
		pr_err("%s: mdm2ap_status config failed, on = %d\n", __func__,
		       on);

	return ret;
}


static int get_mdm2ap_status(void)
{
	return gpio_get_value(mdm2ap_status);
}

static struct sdio_al_platform_data sdio_al_pdata = {
	.config_mdm2ap_status = configure_mdm2ap_status,
	.get_mdm2ap_status = get_mdm2ap_status,
	.allow_sdioc_version_major_2 = 0,
	.peer_sdioc_version_minor = 0x0202,
	.peer_sdioc_version_major = 0x0004,
	.peer_sdioc_boot_version_minor = 0x0001,
	.peer_sdioc_boot_version_major = 0x0003
};

struct platform_device msm_device_sdio_al = {
	.name = "msm_sdio_al",
	.id = -1,
	.dev		= {
		.parent = &msm_charm_modem.dev,
		.platform_data	= &sdio_al_pdata,
	},
};

#endif /* CONFIG_MSM_SDIO_AL */

#ifdef CONFIG_LGE_PMIC8901_REGULATOR
/* we don't use */
#else
#define GPIO_VREG_ID_EXT_5V		0

static struct regulator_consumer_supply vreg_consumers_EXT_5V[] = {
	REGULATOR_SUPPLY("ext_5v",	NULL),
	REGULATOR_SUPPLY("8901_mpp0",	NULL),
};

#define GPIO_VREG_INIT(_id, _reg_name, _gpio_label, _gpio, _active_low) \
	[GPIO_VREG_ID_##_id] = { \
		.init_data = { \
			.constraints = { \
				.valid_ops_mask	= REGULATOR_CHANGE_STATUS, \
			}, \
			.num_consumer_supplies	= \
					ARRAY_SIZE(vreg_consumers_##_id), \
			.consumer_supplies	= vreg_consumers_##_id, \
		}, \
		.regulator_name	= _reg_name, \
		.active_low	= _active_low, \
		.gpio_label	= _gpio_label, \
		.gpio		= _gpio, \
	}

/* GPIO regulator constraints */
static struct gpio_regulator_platform_data msm_gpio_regulator_pdata[] = {
	GPIO_VREG_INIT(EXT_5V, "ext_5v", "ext_5v_en",
					PM8901_MPP_PM_TO_SYS(0), 0),
};

/* GPIO regulator */
static struct platform_device msm8x60_8901_mpp_vreg __devinitdata = {
	.name	= GPIO_REGULATOR_DEV_NAME,
	.id	= PM8901_MPP_PM_TO_SYS(0),
	.dev	= {
		.platform_data =
			&msm_gpio_regulator_pdata[GPIO_VREG_ID_EXT_5V],
	},
};

static void __init pm8901_vreg_mpp0_init(void)
{
	int rc;

	struct pm8xxx_mpp_init_info pm8901_vreg_mpp0 = {
		.mpp	= PM8901_MPP_PM_TO_SYS(0),
		.config =  {
			.type	= PM8XXX_MPP_TYPE_D_OUTPUT,
			.level	= PM8901_MPP_DIG_LEVEL_VPH,
		},
	};

	/*
	 * Set PMIC 8901 MPP0 active_high to 0 for surf and charm_surf. This
	 * implies that the regulator connected to MPP0 is enabled when
	 * MPP0 is low.
	 */
	if (machine_is_msm8x60_surf() || machine_is_msm8x60_fusion()) {
		msm_gpio_regulator_pdata[GPIO_VREG_ID_EXT_5V].active_low = 1;
		pm8901_vreg_mpp0.config.control = PM8XXX_MPP_DOUT_CTRL_HIGH;
	} else {
		msm_gpio_regulator_pdata[GPIO_VREG_ID_EXT_5V].active_low = 0;
		pm8901_vreg_mpp0.config.control = PM8XXX_MPP_DOUT_CTRL_LOW;
	}

	rc = pm8xxx_mpp_config(pm8901_vreg_mpp0.mpp, &pm8901_vreg_mpp0.config);
	if (rc)
		pr_err("%s: pm8xxx_mpp_config: rc=%d\n", __func__, rc);
}
#endif


static struct platform_device *charm_devices[] __initdata = {
	&msm_charm_modem,
#ifdef CONFIG_MSM_SDIO_AL
	&msm_device_sdio_al,
#endif
};

#ifdef CONFIG_SND_SOC_MSM8660_APQ
#endif

static struct platform_device *asoc_devices[] __initdata = {
	&asoc_msm_pcm,
	&asoc_msm_dai0,
	&asoc_msm_dai1,
};

#ifdef CONFIG_ATCMD_VIRTUAL_KBD
extern struct platform_device atcmd_virtual_kbd_device;
#endif

#ifdef CONFIG_ETA_EVENT_LOG
extern struct platform_device eta_event_logger_device;
#endif

#ifdef CONFIG_LGE_DIAGTEST
extern struct platform_device lg_fw_diagcmd_device;
extern struct platform_device lg_diag_cmd_device;
extern struct platform_device lg_diag_input_device;
#endif

static struct platform_device *surf_devices[] __initdata = {
	&msm_device_smd,
	&msm_device_uart_dm12,
	&msm_pil_q6v3,
	&msm_pil_modem,
	&msm_pil_tzapps,
#ifdef CONFIG_LGE_IRDA
	&msm_device_uart_gsbi8,
#endif
#ifdef CONFIG_I2C_QUP
#if defined (CONFIG_LGE_TOUCH_SYNAPTICS_325) || defined (CONFIG_LGE_TOUCHSCREEN_MELFAS)
	&msm_gsbi1_qup_i2c_device,
#endif
	&msm_gsbi3_qup_i2c_device,
	&msm_gsbi4_qup_i2c_device,
#ifdef CONFIG_LGE_FUEL_GAUGE
	&msm_gsbi5_qup_i2c_device,
#endif
#if defined (CONFIG_LGE_WIRELESS_CHARGER_MAX8971) || defined (CONFIG_LGE_WIRELESS_CHARGER_BQ24160)
	&msm_gsbi11_qup_i2c_device,
#endif
	&msm_gsbi7_qup_i2c_device,
	&msm_gsbi8_qup_i2c_device,
	&msm_gsbi9_qup_i2c_device,
	&msm_gsbi12_qup_i2c_device,

#ifdef CONFIG_LGE_SENSOR_ACCELEROMETER
	&msm_gsbi10_qup_i2c_device,
#endif

#endif
//[LGE_UPDATE_S] 20110420, sangyeol.lee@lge.com, [START]
//2011-05-05 ella.hwang added for 1Seg Driver SPI porting - [Begins]
#if defined(CONFIG_LGE_BROADCAST_DCM) || defined(CONFIG_LGE_BROADCAST_TDMB)
	&msm_gsbi11_qup_spi_device,
#endif
//2011-05-05 ella.hwang added for 1Seg Driver SPI porting - [Ends]
#ifdef CONFIG_SERIAL_MSM_HS
	&msm_device_uart_dm1,
#endif
#ifdef CONFIG_MSM_SSBI
	&msm_device_ssbi_pmic1,
	&msm_device_ssbi_pmic2,
#endif
#ifdef CONFIG_I2C_SSBI
	&msm_device_ssbi3,
#endif
#if defined(CONFIG_USB_PEHCI_HCD) || defined(CONFIG_USB_PEHCI_HCD_MODULE)
	&isp1763_device,
#endif

#if defined (CONFIG_MSM_8x60_VOIP)
	&asoc_msm_mvs,
	&asoc_mvs_dai0,
	&asoc_mvs_dai1,
#endif

#if defined(CONFIG_USB_GADGET_MSM_72K) || defined(CONFIG_USB_EHCI_HCD)
	&msm_device_otg,
#endif
#ifdef CONFIG_USB_GADGET_MSM_72K
	&msm_device_gadget_peripheral,
#endif
#ifdef CONFIG_USB_G_ANDROID
	&android_usb_device,
#endif
#ifdef CONFIG_BATTERY_MSM
	&msm_batt_device,
#endif
#ifdef CONFIG_ANDROID_PMEM
#ifndef CONFIG_MSM_MULTIMEDIA_USE_ION
	&android_pmem_device,
	&android_pmem_adsp_device,
	&android_pmem_smipool_device,
#endif
	&android_pmem_audio_device,
#endif
#ifdef CONFIG_MSM_ROTATOR
	&msm_rotator_device,
#endif
	&msm_kgsl_3d0,
	&msm_kgsl_2d0,
	&msm_kgsl_2d1,
#ifdef CONFIG_MSM_GEMINI
	&msm_gemini_device,
#endif
#ifdef CONFIG_MSM_VPE
#ifndef CONFIG_MSM_CAMERA_V4L2
	&msm_vpe_device,
#endif
#endif

#if defined(CONFIG_MSM_RPM_LOG) || defined(CONFIG_MSM_RPM_LOG_MODULE)
	&msm_rpm_log_device,
#endif
#if defined(CONFIG_MSM_RPM_STATS_LOG)
	&msm_rpm_stat_device,
#endif
	&msm_device_vidc,
#ifdef CONFIG_SENSORS_MSM_ADC
	&msm_adc_device,
#endif
	&rpm_regulator_device,

#if defined(CONFIG_CRYPTO_DEV_QCRYPTO) || \
		defined(CONFIG_CRYPTO_DEV_QCRYPTO_MODULE)
	&qcrypto_device,
#endif

#if defined(CONFIG_CRYPTO_DEV_QCEDEV) || \
		defined(CONFIG_CRYPTO_DEV_QCEDEV_MODULE)
	&qcedev_device,
#endif


#if defined(CONFIG_TSIF) || defined(CONFIG_TSIF_MODULE)
#ifdef CONFIG_MSM_USE_TSIF1
	&msm_device_tsif[1],
#else
	&msm_device_tsif[0],
#endif /* CONFIG_MSM_USE_TSIF1 */
#endif /* CONFIG_TSIF */

#ifdef CONFIG_HW_RANDOM_MSM
	&msm_device_rng,
#endif

	&msm_tsens_device,
	&msm_rpm_device,
#ifdef CONFIG_ION_MSM
	&ion_dev,
#endif
	&msm8660_device_watchdog,

#ifdef CONFIG_ATCMD_VIRTUAL_KBD
	&atcmd_virtual_kbd_device,
#endif

#ifdef CONFIG_ETA_EVENT_LOG
	&eta_event_logger_device,
#endif

#ifdef CONFIG_LGE_DIAGTEST
	&lg_fw_diagcmd_device,
	&lg_diag_cmd_device,
	&lg_diag_input_device,
#endif

#ifdef CONFIG_LGE_HALLIC_CARKIT
	&hallic_dock_device,
#endif

};

#ifdef CONFIG_ION_MSM
#ifdef CONFIG_MSM_MULTIMEDIA_USE_ION
static struct ion_cp_heap_pdata cp_mm_ion_pdata = {
	.permission_type = IPT_TYPE_MM_CARVEOUT,
	.align = PAGE_SIZE,
	.request_region = request_smi_region,
	.release_region = release_smi_region,
	.setup_region = setup_smi_region,
};

static struct ion_cp_heap_pdata cp_mfc_ion_pdata = {
	.permission_type = IPT_TYPE_MFC_SHAREDMEM,
	.align = PAGE_SIZE,
	.request_region = request_smi_region,
	.release_region = release_smi_region,
	.setup_region = setup_smi_region,
};

static struct ion_cp_heap_pdata cp_wb_ion_pdata = {
	.permission_type = IPT_TYPE_MDP_WRITEBACK,
	.align = PAGE_SIZE,
};

static struct ion_co_heap_pdata fw_co_ion_pdata = {
	.adjacent_mem_id = ION_CP_MM_HEAP_ID,
	.align = SZ_128K,
};

static struct ion_co_heap_pdata co_ion_pdata = {
	.adjacent_mem_id = INVALID_HEAP_ID,
	.align = PAGE_SIZE,
};
#endif

/**
 * These heaps are listed in the order they will be allocated. Due to
 * video hardware restrictions and content protection the FW heap has to
 * be allocated adjacent (below) the MM heap and the MFC heap has to be
 * allocated after the MM heap to ensure MFC heap is not more than 256MB
 * away from the base address of the FW heap.
 * However, the order of FW heap and MM heap doesn't matter since these
 * two heaps are taken care of by separate code to ensure they are adjacent
 * to each other.
 * Don't swap the order unless you know what you are doing!
 */
static struct ion_platform_data ion_pdata = {
	.nr = MSM_ION_HEAP_NUM,
	.heaps = {
		{
			.id	= ION_SYSTEM_HEAP_ID,
			.type	= ION_HEAP_TYPE_SYSTEM,
			.name	= ION_VMALLOC_HEAP_NAME,
		},
#ifdef CONFIG_MSM_MULTIMEDIA_USE_ION
		{
			.id	= ION_CP_MM_HEAP_ID,
			.type	= ION_HEAP_TYPE_CP,
			.name	= ION_MM_HEAP_NAME,
			.size	= MSM_ION_MM_SIZE,
			.memory_type = ION_SMI_TYPE,
			.extra_data = (void *) &cp_mm_ion_pdata,
		},
		{
			.id	= ION_MM_FIRMWARE_HEAP_ID,
			.type	= ION_HEAP_TYPE_CARVEOUT,
			.name	= ION_MM_FIRMWARE_HEAP_NAME,
			.size	= MSM_ION_MM_FW_SIZE,
			.memory_type = ION_SMI_TYPE,
			.extra_data = (void *) &fw_co_ion_pdata,
		},
		{
			.id	= ION_CP_MFC_HEAP_ID,
			.type	= ION_HEAP_TYPE_CP,
			.name	= ION_MFC_HEAP_NAME,
			.size	= MSM_ION_MFC_SIZE,
			.memory_type = ION_SMI_TYPE,
			.extra_data = (void *) &cp_mfc_ion_pdata,
		},
		{
			.id	= ION_SF_HEAP_ID,
			.type	= ION_HEAP_TYPE_CARVEOUT,
			.name	= ION_SF_HEAP_NAME,
			.size	= MSM_ION_SF_SIZE,
			.memory_type = ION_EBI_TYPE,
			.extra_data = (void *)&co_ion_pdata,
		},
#if defined(CONFIG_MACH_LGE_325_BOARD_SKT) || defined(CONFIG_MACH_LGE_325_BOARD_LGU)
	/* korean-model doesn't use CAMERA-ION for increasing 30MB memory */
#else
		{
			.id	= ION_CAMERA_HEAP_ID,
			.type	= ION_HEAP_TYPE_CARVEOUT,
			.name	= ION_CAMERA_HEAP_NAME,
			.size	= MSM_ION_CAMERA_SIZE,
			.memory_type = ION_EBI_TYPE,
			.extra_data = &co_ion_pdata,
		},
#endif
		{
			.id	= ION_CP_WB_HEAP_ID,
			.type	= ION_HEAP_TYPE_CP,
			.name	= ION_WB_HEAP_NAME,
			.size	= MSM_ION_WB_SIZE,
			.memory_type = ION_EBI_TYPE,
			.extra_data = (void *) &cp_wb_ion_pdata,
		},
		{
			.id	= ION_QSECOM_HEAP_ID,
			.type	= ION_HEAP_TYPE_CARVEOUT,
			.name	= ION_QSECOM_HEAP_NAME,
			.size	= MSM_ION_QSECOM_SIZE,
			.memory_type = ION_EBI_TYPE,
			.extra_data = (void *) &co_ion_pdata,
		},
		{
			.id	= ION_AUDIO_HEAP_ID,
			.type	= ION_HEAP_TYPE_CARVEOUT,
			.name	= ION_AUDIO_HEAP_NAME,
			.size	= MSM_ION_AUDIO_SIZE,
			.memory_type = ION_EBI_TYPE,
			.extra_data = (void *)&co_ion_pdata,
		},
#endif
	}
};

static struct platform_device ion_dev = {
	.name = "ion-msm",
	.id = 1,
	.dev = { .platform_data = &ion_pdata },
};
#endif


static struct memtype_reserve msm8x60_reserve_table[] __initdata = {
	/* Kernel SMI memory pool for video core, used for firmware */
	/* and encoder, decoder scratch buffers */
	/* Kernel SMI memory pool should always precede the user space */
	/* SMI memory pool, as the video core will use offset address */
	/* from the Firmware base */
	[MEMTYPE_SMI_KERNEL] = {
		.start	=	KERNEL_SMI_BASE,
		.limit	=	KERNEL_SMI_SIZE,
		.size	=	KERNEL_SMI_SIZE,
		.flags	=	MEMTYPE_FLAGS_FIXED,
	},
	/* User space SMI memory pool for video core */
	/* used for encoder, decoder input & output buffers  */
	[MEMTYPE_SMI] = {
		.start	=	USER_SMI_BASE,
		.limit	=	USER_SMI_SIZE,
		.flags	=	MEMTYPE_FLAGS_FIXED,
	},
	[MEMTYPE_EBI0] = {
		.flags	=	MEMTYPE_FLAGS_1M_ALIGN,
	},
	[MEMTYPE_EBI1] = {
		.flags	=	MEMTYPE_FLAGS_1M_ALIGN,
	},
};

static void reserve_ion_memory(void)
{
#if defined(CONFIG_ION_MSM) && defined(CONFIG_MSM_MULTIMEDIA_USE_ION)
	unsigned int i;

	if (hdmi_is_primary) {
		msm_ion_sf_size = MSM_HDMI_PRIM_ION_SF_SIZE;
		for (i = 0; i < ion_pdata.nr; i++) {
			if (ion_pdata.heaps[i].id == ION_SF_HEAP_ID) {
				ion_pdata.heaps[i].size = msm_ion_sf_size;
				pr_debug("msm_ion_sf_size 0x%x\n",
					msm_ion_sf_size);
				break;
			}
		}
	}

	msm8x60_reserve_table[MEMTYPE_EBI1].size += msm_ion_sf_size;
	msm8x60_reserve_table[MEMTYPE_SMI].size += MSM_ION_MM_FW_SIZE;
	msm8x60_reserve_table[MEMTYPE_SMI].size += MSM_ION_MM_SIZE;
	msm8x60_reserve_table[MEMTYPE_SMI].size += MSM_ION_MFC_SIZE;
#if defined(CONFIG_MACH_LGE_325_BOARD_SKT) || defined(CONFIG_MACH_LGE_325_BOARD_LGU)
	/* korean-model doesn't use CAMERA-ION for increasing 30MB memory */
#else
	msm8x60_reserve_table[MEMTYPE_EBI1].size += MSM_ION_CAMERA_SIZE;
#endif
	msm8x60_reserve_table[MEMTYPE_EBI1].size += MSM_ION_WB_SIZE;
	msm8x60_reserve_table[MEMTYPE_EBI1].size += MSM_ION_AUDIO_SIZE;
#endif
}

static void __init size_pmem_devices(void)
{
#ifdef CONFIG_ANDROID_PMEM
#ifndef CONFIG_MSM_MULTIMEDIA_USE_ION
	android_pmem_adsp_pdata.size = pmem_adsp_size;
	android_pmem_smipool_pdata.size = MSM_PMEM_SMIPOOL_SIZE;

	if (hdmi_is_primary)
		pmem_sf_size = MSM_HDMI_PRIM_PMEM_SF_SIZE;
	android_pmem_pdata.size = pmem_sf_size;
#endif
	android_pmem_audio_pdata.size = MSM_PMEM_AUDIO_SIZE;
#endif
}

#ifdef CONFIG_ANDROID_RAM_CONSOLE
static void __init reserve_ram_console_memory(void)
{
	msm8x60_reserve_table[MEMTYPE_EBI1].size += LGE_RAM_CONSOLE_SIZE;
}
#endif

#ifdef CONFIG_LGE_HANDLE_PANIC
static void __init reserve_crash_log_memory(void)
{
	msm8x60_reserve_table[MEMTYPE_EBI1].size += LGE_CRASH_LOG_SIZE;
}
#endif

static void __init reserve_memory_for(struct android_pmem_platform_data *p)
{
	msm8x60_reserve_table[p->memory_type].size += p->size;
}

static void __init reserve_pmem_memory(void)
{
#ifdef CONFIG_ANDROID_PMEM
#ifndef CONFIG_MSM_MULTIMEDIA_USE_ION
	reserve_memory_for(&android_pmem_adsp_pdata);
	reserve_memory_for(&android_pmem_smipool_pdata);
	reserve_memory_for(&android_pmem_pdata);
#endif
	reserve_memory_for(&android_pmem_audio_pdata);
	msm8x60_reserve_table[MEMTYPE_EBI1].size += pmem_kernel_ebi1_size;
#endif
}

static void __init reserve_mdp_memory(void)
{
	msm8x60_mdp_writeback(msm8x60_reserve_table);
}

static void __init msm8x60_calculate_reserve_sizes(void)
{
	size_pmem_devices();
	reserve_pmem_memory();
	reserve_ion_memory();
	reserve_mdp_memory();
#ifdef CONFIG_ANDROID_RAM_CONSOLE
	reserve_ram_console_memory();
#endif
#ifdef CONFIG_LGE_HANDLE_PANIC
	reserve_crash_log_memory();
#endif
}

static int msm8x60_paddr_to_memtype(unsigned int paddr)
{
	if (paddr >= 0x40000000 && paddr < 0x60000000)
		return MEMTYPE_EBI1;
	if (paddr >= 0x38000000 && paddr < 0x40000000)
		return MEMTYPE_SMI;
	return MEMTYPE_NONE;
}

static struct reserve_info msm8x60_reserve_info __initdata = {
	.memtype_reserve_table = msm8x60_reserve_table,
	.calculate_reserve_sizes = msm8x60_calculate_reserve_sizes,
	.paddr_to_memtype = msm8x60_paddr_to_memtype,
};

static char prim_panel_name[PANEL_NAME_MAX_LEN];
static char ext_panel_name[PANEL_NAME_MAX_LEN];
static int __init prim_display_setup(char *param)
{
	if (strnlen(param, PANEL_NAME_MAX_LEN))
		strlcpy(prim_panel_name, param, PANEL_NAME_MAX_LEN);
	return 0;
}
early_param("prim_display", prim_display_setup);

static int __init ext_display_setup(char *param)
{
	if (strnlen(param, PANEL_NAME_MAX_LEN))
		strlcpy(ext_panel_name, param, PANEL_NAME_MAX_LEN);
	return 0;
}
early_param("ext_display", ext_display_setup);

static void __init msm8x60_reserve(void)
{
	msm8x60_set_display_params(prim_panel_name, ext_panel_name);
	reserve_info = &msm8x60_reserve_info;
	msm_reserve();
}


#if defined(CONFIG_SMB137B_CHARGER) || defined(CONFIG_SMB137B_CHARGER_MODULE)
#endif

#ifdef CONFIG_LGE_WIRELESS_CHARGER_MAX8971
#define WIRELESS_CHG_MPP_11 10
#define WIRELESS_CHG_MPP_12 11
static int max8971_detection_setup(void)
{
	int ret = 0;

	ret = pm8058_mpp_config_digital_in(WIRELESS_CHG_MPP_11,
					   PM8058_MPP_DIG_LEVEL_S3,
					   PM_MPP_DIN_TO_INT);
	ret |=  pm8058_mpp_config_bi_dir(WIRELESS_CHG_MPP_12,
					   PM8058_MPP_DIG_LEVEL_S3,
					   PM_MPP_BI_PULLUP_10KOHM
					   );
	return ret;
}

static struct max8971_platform_data max8971_data __initdata = {

	.chgcc = 0x0E,			// Fast Charge Current - 600mA
	.fchgtime = 0x03,			// Fast Charge Time - 6hrs

	.chgrstrt = 0x0,			// Fast Charge Restart Threshold - 150mV
	.dcilmt = 0x3F, 			// Input Current Limit Selection

	.topofftime = 0x03,		// Top Off Timer Setting  - 30min
	.topofftshld = 0x03,		// Done Current Threshold - 200mA
	.chgcv = 0x02,				// Charger Termination Voltage - 4.35V

//	.regtemp;			// Die temperature thermal regulation loop setpoint
//	.safetyreg;			// JEITA Safety region selection
	.thm_config = 0x1, 		// Thermal monitor configuration - thermistor disable

	.int_mask = 0xFE,			// CHGINT_MASK - mask all

	.valid_n_gpio = PM8058_MPP_PM_TO_SYS(WIRELESS_CHG_MPP_11),
	.chg_detection_config = max8971_detection_setup,
};

static struct i2c_board_info max8971_charger_i2c_info[] __initdata = {
	{
		I2C_BOARD_INFO("max8971", 0x35),
		.irq = PM8058_CBLPWR_IRQ(PM8058_IRQ_BASE),
		.platform_data = &max8971_data,
	},
};
#endif

#ifdef CONFIG_LGE_WIRELESS_CHARGER_BQ24160
#define WIRELESS_CHG_MPP_11 10
#define WIRELESS_CHG_MPP_12 11
#define WIRELESS_CHG_IRQ_N	124

static struct pm8xxx_mpp_init_info wireless_chg_mpp[] = {
           PM8058_MPP_INIT(WIRELESS_CHG_MPP_11, D_INPUT,
                     PM8058_MPP_DIG_LEVEL_S3, DIN_TO_INT),
           PM8058_MPP_INIT(WIRELESS_CHG_MPP_12, D_BI_DIR,
                     PM8058_MPP_DIG_LEVEL_S3, BI_PULLUP_10KOHM),
};

static int bq24160_detection_setup(void)
{
#if 1
           int ret = 0, i;

           for (i = 0; i < ARRAY_SIZE(wireless_chg_mpp); i++) {
                     ret = pm8xxx_mpp_config(wireless_chg_mpp[i].mpp,
                                                     &wireless_chg_mpp[i].config);
                     if (ret) {
                                pr_err("%s: Config MPP %d of PM8058 failed\n",
                                                                __func__, wireless_chg_mpp[i].mpp);
                                return ret;
                     }
           }

           return ret;
#else
	int ret = 0;

	ret = pm8058_mpp_config_digital_in(WIRELESS_CHG_MPP_11,
					   PM8058_MPP_DIG_LEVEL_S3,
					   PM_MPP_DIN_TO_INT);
	ret |=  pm8058_mpp_config_bi_dir(WIRELESS_CHG_MPP_12,
					   PM8058_MPP_DIG_LEVEL_S3,
					   PM_MPP_BI_PULLUP_10KOHM
					   );
	return ret;
#endif
}

static struct bq24160_platform_data bq24160_data __initdata = {
	.tmr_rst = 0b1,
	.supply_sel = 0b1,

	.reset = 0b0,			/* reset all reg to default values : 0 => no effect, 1 => reset all */
	.iusblimit = 0b100,		/* usb current limit : 100 => USB3.0 host 900mA current limit */
	.enstat = 0b1,			/* enable STAT : 0 => disable, 1 => enable */
	.te = 0b1,				/* enable charger termination : 0 => disable, 1 => enable */
	.ce = 0b0,				/* charger enable : 0 => enable, 1 => disable */
	.hz_mode = 0b0,			/* high impedance mode : 0 => no high impedance, 1 => high impedance */

	.vbatt_reg = 0b101011,	/* battery regulation voltage : 101011 => 4.36V*/
	.inlimit_in = 0b0,		/* input limit for IN input : 0 => 1.5A, 1 => 2.5A */
	.dpdm_en = 0b0,			/* D+/D- detention : 0 => normal state, 1 => force D+/D- detection */

	.chgcrnt = 0b00001,		/* charge current : 00010 => 700mA */
	.termcrnt = 0b011,		/* termination current sense : 011 => 200mA*/

	.minsys_stat = 0b0,		/* minimum system voltage mode : 0 => not active, 1 => active */
	.dpm_stat = 0b1,		/* Vin-DPM mode : 0 => not active, 1 => active */
	.vindpm_usb = 0b000,	/* usb input Vin-dpm voltage : 000 => 4.2V */
	.vindpm_in = 0b000,		/* IN input Vin-dpm voltage */

	.tmr2x_en = 0b1,		/* timer slowed by 2x */
	.safety_tmr = 0b01,		/* safety timer : 01 => 6hrs*/
	.ts_en = 0b0,			/* ts function enable */
	.ts_fault = 0b00,		/* ts fault mode */

	.valid_n_gpio = PM8058_MPP_PM_TO_SYS(WIRELESS_CHG_MPP_11),
	.chg_detection_config = bq24160_detection_setup,
};

static struct i2c_board_info bq24160_charger_i2c_info[] __initdata = {
	{
		I2C_BOARD_INFO("bq24160", 0x6B),
//		.irq = PM8058_CBLPWR_IRQ(PM8058_IRQ_BASE),
		.irq = PM8058_IRQ_BASE + PM8058_CBLPWR_IRQ,
		.platform_data = &bq24160_data,
	},
};
#endif

#ifdef CONFIG_BATTERY_325
#ifdef CONFIG_BATMAN_VZW_KERNEL_GPIO
#define SWITCHING_CHG_IRQ_N    156
#else
#define SWITCHING_CHG_IRQ_N    155
#endif

static struct bq24160_platform_data bq24160_data = {

     .tmr_rst = 0b1,
       .supply_sel = 0b1,

       .reset = 0b0,       /* reset all reg to default values : 0 => no effect, 1 => reset all */
       .iusblimit = 0b100,       /* usb current limit : 100 => USB3.0 host 900mA current limit */
       .enstat = 0b1,       /* enable STAT : 0 => disable, 1 => enable */
       .te = 0b1,       /* enable charger termination : 0 => disable, 1 => enable */
       .ce = 0b0,       /* charger enable : 0 => enable, 1 => disable */
       .hz_mode = 0b0,       /* high impedance mode : 0 => no high impedance, 1 => high impedance */

       .vbatt_reg = 0b101011, /* battery regulation voltage : 101011 => 4.36V*/
       .inlimit_in = 0b0,       /* input limit for IN input : 0 => 1.5A, 1 => 2.5A */
       .dpdm_en = 0b0,       /* D+/D- detention : 0 => normal state, 1 => force D+/D- detection */

       .chgcrnt = 0b00001,       /* charge current : 00010 => 700mA */
       .termcrnt = 0b011,       /* termination current sense : 011 => 200mA*/

       .minsys_stat = 0b0,       /* minimum system voltage mode : 0 => not active, 1 => active */
       .dpm_stat = 0b1,       /* Vin-DPM mode : 0 => not active, 1 => active */
       .vindpm_usb = 0b000,   /* usb input Vin-dpm voltage : 000 => 4.2V */
       .vindpm_in = 0b000,       /* IN input Vin-dpm voltage */

       .tmr2x_en = 0b1,       /* timer slowed by 2x */
      .safety_tmr = 0b01,      /* safety timer : 01 => 6hrs*/
       .ts_en = 0b0,       /* ts function enable */
       .ts_fault = 0b00,       /* ts fault mode */
};

//b325
static struct platform_device b325_battery_device = {
       .name = "b325_battery",
       .id = -1,
       .dev = {
       .platform_data = &bq24160_data,
       }
	};

static struct i2c_board_info bq24160_charger_i2c_info[] = {
       {
       I2C_BOARD_INFO("bq24160", 0x6B),
       .irq = MSM_GPIO_TO_INT(SWITCHING_CHG_IRQ_N),
       },
	};

int lge_battery_info = 0;

#endif

#ifdef CONFIG_BATTERY_MSM8X60

#ifdef CONFIG_LGE_CHARGER_VOLTAGE_CURRENT_SCENARIO
int lge_battery_info;

static int __init battery_information_setup(char *batt_info)
{
    if(!strcmp(batt_info, "ds2704"))
    {
            lge_battery_info = BATT_DS2704;

            msm_charger_data.max_voltage = 4350;
            msm_charger_data.min_voltage = 3500;
	//FIXME 3160_conflict
	//msm_charger_data.resume_voltage = 4250;

#ifdef CONFIG_LGE_WIRELESS_CHARGER_BQ24160
            bq24160_data.vbatt_reg = 0b101011;
#endif //CONFIG_LGE_WIRELESS_CHARGER_BQ24160
    }
    else if(!strcmp(batt_info, "isl6296"))
    {
            lge_battery_info = BATT_ISL6296;

            msm_charger_data.max_voltage = 4200;
            msm_charger_data.min_voltage = 3200;
	//FIXME 3160_conflict
	// msm_charger_data.resume_voltage = 4100;

#ifdef CONFIG_LGE_WIRELESS_CHARGER_BQ24160
            bq24160_data.vbatt_reg = 0b100011;
#endif //CONFIG_LGE_WIRELESS_CHARGER_BQ24160
    }
    printk(KERN_INFO "Battery : %s %d\n", batt_info, lge_battery_info);

    return 1;
}
__setup("lge.batt_info=", battery_information_setup);
#endif
#endif


#ifdef CONFIG_PMIC8058
#define PMIC_GPIO_SDC3_DET 22
#ifdef CONFIG_LGE_PMIC8058_GPIO

#ifdef CONFIG_LGE_HALLIC_CARKIT
#define CRADLE_DETECT_2			17
#define CRADLE_DETECT 			18
#endif
#define HPH_DETECT_N 			19
#define HPH_KEY_INT 			20

#define WCA_AUDIO_RESET_N 		21

#define MICRO_SD_CARD_DETECT_N	22
#define PMIC_SSBI_CLK			39
#endif

static int pm8058_gpios_init(void)
{
	int i;
	int rc;
	struct pm8058_gpio_cfg {
		int                gpio;
		struct pm_gpio	   cfg;
	};

	struct pm8058_gpio_cfg gpio_cfgs[] = {
#if defined(CONFIG_LGE_PMIC8058_GPIO) && defined(CONFIG_BATMAN_VZW_KERNEL_GPIO)

#ifdef CONFIG_LGE_HALLIC_CARKIT
		{ /* CRADLE_DETECT_2 - GPIO17*/
			PM8058_GPIO_PM_TO_SYS(CRADLE_DETECT_2 - 1),
			{
				.direction		= PM_GPIO_DIR_IN,
				.pull			= PM_GPIO_PULL_UP_30,
				.vin_sel		= 2,
				.function		= PM_GPIO_FUNC_NORMAL,
				.inv_int_pol	= 0,
			},
		},
		{ /* CRADLE_DETECT - GPIO18*/
			PM8058_GPIO_PM_TO_SYS(CRADLE_DETECT - 1),
			{
				.direction		= PM_GPIO_DIR_IN,
				.pull			= PM_GPIO_PULL_UP_30,
				.vin_sel		= 2,
				.function		= PM_GPIO_FUNC_NORMAL,
				.inv_int_pol	= 0,
			},
		},
#endif
		{ /* HPH_DETECT_N / EAR_SENSE_N - GPIO19*/
			PM8058_GPIO_PM_TO_SYS(HPH_DETECT_N - 1),
			{
				.direction		= PM_GPIO_DIR_IN,
				.pull			= PM_GPIO_PULL_UP_30,
				.vin_sel		= 2,
				.function		= PM_GPIO_FUNC_NORMAL,
				.inv_int_pol	= 0,
			},
		},
		{ /* HPH_KEY_INT / EAR_KEY_INT - GPIO20*/
			PM8058_GPIO_PM_TO_SYS(HPH_KEY_INT - 1),
			{
				.direction		= PM_GPIO_DIR_IN,
				.pull			= PM_GPIO_PULL_UP_30,
				.vin_sel		= 2,
				.function		= PM_GPIO_FUNC_NORMAL,
				.inv_int_pol	= 0,
			},
		},
		{ /* WCA_AUDIO_RESET_N / QTR_CODEC_RESET_N -GPIO21 */
			PM8058_GPIO_PM_TO_SYS(WCA_AUDIO_RESET_N - 1),
			{
				.direction	= PM_GPIO_DIR_OUT,
				.output_value	= 1,
				.output_buffer	= PM_GPIO_OUT_BUF_CMOS,
				.pull		= PM_GPIO_PULL_DN,
				.out_strength	= PM_GPIO_STRENGTH_HIGH,
				.function	= PM_GPIO_FUNC_NORMAL,
				.vin_sel	= 2,
				.inv_int_pol	= 0,

			},
		},
#ifdef CONFIG_BATMAN_VZW_KERNEL_GPIO
		/* Batman Does not support External SD Card */
#else
		{ /* MICRO_SD_CARD_DETECT_N - GPIO22*/
			PM8058_GPIO_PM_TO_SYS(MICRO_SD_CARD_DETECT_N - 1),
			{
				.direction		= PM_GPIO_DIR_IN,
				.pull			= PM_GPIO_PULL_UP_30,
				.vin_sel		= 2,
				.function		= PM_GPIO_FUNC_NORMAL,
				.inv_int_pol	= 0,
			},
		},
#endif
		{ /* PMIC_SSBI_CLK - GPIO39*/
			PM8058_GPIO_PM_TO_SYS(PMIC_SSBI_CLK - 1),
			{
				.direction		= PM_GPIO_DIR_IN,
				.pull			= PM_GPIO_PULL_NO,
				.vin_sel		= 2,
				.function		= PM_GPIO_FUNC_NORMAL,
				.inv_int_pol	= 0,
			},
		},

#else
		{ /* FFA ethernet */
			PM8058_GPIO_PM_TO_SYS(6),
			{
				.direction      = PM_GPIO_DIR_IN,
				.pull           = PM_GPIO_PULL_DN,
				.vin_sel        = 2,
				.function       = PM_GPIO_FUNC_NORMAL,
				.inv_int_pol    = 0,
			},
		},
#ifdef CONFIG_MMC_MSM_CARD_HW_DETECTION
		{
			PM8058_GPIO_PM_TO_SYS(PMIC_GPIO_SDC3_DET - 1),
			{
				.direction      = PM_GPIO_DIR_IN,
#if defined(CONFIG_LGE_PMIC8058_GPIO)
				.pull			= PM_GPIO_PULL_NO,
#else
				.pull			= PM_GPIO_PULL_UP_30,
#endif
				.vin_sel        = 2,
				.function       = PM_GPIO_FUNC_NORMAL,
				.inv_int_pol    = 0,
			},
		},
#endif
		{ /* core&surf gpio expander */
			PM8058_GPIO_PM_TO_SYS(UI_INT1_N),
			{
				.direction      = PM_GPIO_DIR_IN,
				.pull           = PM_GPIO_PULL_NO,
				.vin_sel        = PM8058_GPIO_VIN_S3,
				.function       = PM_GPIO_FUNC_NORMAL,
				.inv_int_pol    = 0,
			},
		},
		{ /* docking gpio expander */
			PM8058_GPIO_PM_TO_SYS(UI_INT2_N),
			{
				.direction      = PM_GPIO_DIR_IN,
				.pull           = PM_GPIO_PULL_NO,
				.vin_sel        = PM8058_GPIO_VIN_S3,
				.function       = PM_GPIO_FUNC_NORMAL,
				.inv_int_pol    = 0,
			},
		},
		{ /* FHA/keypad gpio expanders */
			PM8058_GPIO_PM_TO_SYS(UI_INT3_N),
			{
				.direction      = PM_GPIO_DIR_IN,
				.pull           = PM_GPIO_PULL_NO,
				.vin_sel        = PM8058_GPIO_VIN_S3,
				.function       = PM_GPIO_FUNC_NORMAL,
				.inv_int_pol    = 0,
			},
		},
#ifdef CONFIG_LGE_AUDIO
		{ /* RCV AMP RESET */
			PM8058_GPIO_PM_TO_SYS(PMIC_GPIO_RCV_AMP_RESET - 1),
			{
				.direction	= PM_GPIO_DIR_OUT,
				.output_value	= 0,
				.output_buffer	= PM_GPIO_OUT_BUF_CMOS,
				.pull		= PM_GPIO_PULL_DN,
				.out_strength	= PM_GPIO_STRENGTH_HIGH,
				.function	= PM_GPIO_FUNC_NORMAL,
				.vin_sel	= 2,
				.inv_int_pol	= 0,

			}
		},
		{ /* CAMERA MIC ENABLE */
			PM8058_GPIO_PM_TO_SYS(PMIC_GPIO_CAMCORDER_MIC_EN - 1),
			{
				.direction	= PM_GPIO_DIR_OUT,
				.output_value	= 0,
				.output_buffer	= PM_GPIO_OUT_BUF_CMOS,
				.pull		= PM_GPIO_PULL_DN,
				.out_strength	= PM_GPIO_STRENGTH_HIGH,
				.function	= PM_GPIO_FUNC_NORMAL,
				.vin_sel	= 2,
				.inv_int_pol	= 0,

			}
		},
#endif
		{ /* TouchDisc Interrupt */
			PM8058_GPIO_PM_TO_SYS(5),
			{
				.direction      = PM_GPIO_DIR_IN,
				.pull           = PM_GPIO_PULL_UP_1P5,
				.vin_sel        = 2,
				.function       = PM_GPIO_FUNC_NORMAL,
				.inv_int_pol    = 0,
			}
		},
		{ /* Timpani Reset */
			PM8058_GPIO_PM_TO_SYS(20),
			{
				.direction	= PM_GPIO_DIR_OUT,
				.output_value	= 1,
				.output_buffer	= PM_GPIO_OUT_BUF_CMOS,
				.pull		= PM_GPIO_PULL_DN,
				.out_strength	= PM_GPIO_STRENGTH_HIGH,
				.function	= PM_GPIO_FUNC_NORMAL,
				.vin_sel	= 2,
				.inv_int_pol	= 0,
			}
		},

#ifdef CONFIG_LGE_HEADSET_DETECTION_FSA8008
		{ /* Ear Sense */
			PM8058_GPIO_PM_TO_SYS(PMIC_GPIO_EAR_SENSE_N - 1),
				{
					.direction	= PM_GPIO_DIR_IN,
					.pull		= PM_GPIO_PULL_NO,
					.function	= PM_GPIO_FUNC_NORMAL,
					.vin_sel	= 2,
					.inv_int_pol	= 0,
				}
		},

		{ /* Ear Key */
			PM8058_GPIO_PM_TO_SYS(PMIC_GPIO_EAR_KEY_INT - 1),
				{
					.direction	= PM_GPIO_DIR_IN,
					.pull		= PM_GPIO_PULL_NO,
					.function	= PM_GPIO_FUNC_NORMAL,
					.vin_sel	= 2,
					.inv_int_pol	= 0,
				}
		},
#endif

		{ /* PMIC ID interrupt */
			PM8058_GPIO_PM_TO_SYS(36),
			{
				.direction	= PM_GPIO_DIR_IN,
				.pull		= PM_GPIO_PULL_UP_1P5,
				.function	= PM_GPIO_FUNC_NORMAL,
				.vin_sel	= 2,
				.inv_int_pol	= 0,
			}
		},
#endif
	};

#if defined(CONFIG_PMIC8058_OTHC) || defined(CONFIG_PMIC8058_OTHC_MODULE)
#ifndef CONFIG_LGE_HEADSET_DETECTION_FSA8008
	struct pm8058_gpio_cfg line_in_gpio_cfg = {
			PM8058_GPIO_PM_TO_SYS(18),
			{
				.direction	= PM_GPIO_DIR_IN,
				.pull           = PM_GPIO_PULL_UP_1P5,
				.vin_sel        = 2,
				.function       = PM_GPIO_FUNC_NORMAL,
				.inv_int_pol    = 0,
			}
	};

#if defined(CONFIG_PMIC8058_OTHC) || defined(CONFIG_PMIC8058_OTHC_MODULE)
	/* Line_in only for 8660 ffa & surf */
	if (machine_is_msm8x60_ffa() || machine_is_msm8x60_surf() ||
		machine_is_lge_i_board() || machine_is_msm8x60_fusion() || machine_is_msm8x60_dragon() ||
		machine_is_msm8x60_fusn_ffa() || machine_is_lge_325_board()) {
		rc = pm8058_gpio_config(line_in_gpio_cfg.gpio,
				&line_in_gpio_cfg.cfg);
		if (rc < 0) {
			pr_err("%s pmic line_in gpio config failed\n",
							__func__);
			return rc;
		}
	}
#endif
#endif
#endif

	for (i = 0; i < ARRAY_SIZE(gpio_cfgs); ++i) {
		rc = pm8xxx_gpio_config(gpio_cfgs[i].gpio,
				&gpio_cfgs[i].cfg);
		if (rc < 0) {
			pr_err("%s pmic gpio config failed\n",
				__func__);
			return rc;
		}
	}

	return 0;
}

static const unsigned int batman_keymap[] = {
	KEY(0, 0, KEY_VOLUMEUP),
	KEY(0, 1, KEY_VOLUMEDOWN),
	KEY(1, 0, KEY_HIGHLIGHER),
	KEY(2, 1, KEY_PTN_UNLOCK),
};

static struct matrix_keymap_data batman_keymap_data = {
	.keymap_size	= ARRAY_SIZE(batman_keymap),
	.keymap		= batman_keymap,
};

static struct pm8xxx_keypad_platform_data batman_keypad_data = {
#if defined(CONFIG_MACH_LGE_325_BOARD_SKT)
	.input_name		= "batman_skt-keypad",
	.input_phys_device	= "batman_skt-keypad/input0",
#elif defined(CONFIG_MACH_LGE_325_BOARD_DCM)
	.input_name 	= "batman_dcm-keypad",
	.input_phys_device	= "batman_dcm-keypad/input0",
#elif defined (CONFIG_MACH_LGE_325_BOARD_LGU)
	.input_name 	= "batman_lgu-keypad",
	.input_phys_device	= "batman_lgu-keypad/input0",
#elif defined(CONFIG_MACH_LGE_325_BOARD_VZW)
	.input_name 	= "batman_vzw-keypad",
	.input_phys_device	= "batman_vzw-keypad/input0",
#endif
	.num_rows		= 6,
	.num_cols		= 5,
	.rows_gpio_start	= PM8058_GPIO_PM_TO_SYS(8),
	.cols_gpio_start	= PM8058_GPIO_PM_TO_SYS(0),
	.debounce_ms		= 15,
	.scan_delay_ms		= 32,
	.row_hold_ns            = 91500,
	.wakeup			= 1,
	.keymap_data		= &batman_keymap_data,
};

static struct pm8xxx_rtc_platform_data pm8058_rtc_pdata = {
	.rtc_write_enable = false,
	.rtc_alarm_powerup = false,
};

static struct pm8xxx_pwrkey_platform_data pm8058_pwrkey_pdata = {
	.pull_up		= 1,
	.kpd_trigger_delay_us   = 15625,
	.wakeup			= 1,
	.pwrkey_time_ms		= 500,
};

#if defined(CONFIG_PMIC8058_OTHC) || defined(CONFIG_PMIC8058_OTHC_MODULE)
#define PM8058_OTHC_CNTR_BASE0	0xA0
#define PM8058_OTHC_CNTR_BASE1	0x134
#define PM8058_OTHC_CNTR_BASE2	0x137

#ifndef CONFIG_LGE_HEADSET_DETECTION_FSA8008
#define PM8058_LINE_IN_DET_GPIO	PM8058_GPIO_PM_TO_SYS(18)

static struct othc_accessory_info othc_accessories[]  = {
	{
		.accessory = OTHC_SVIDEO_OUT,
		.detect_flags = OTHC_MICBIAS_DETECT | OTHC_SWITCH_DETECT
							| OTHC_ADC_DETECT,
		.key_code = SW_VIDEOOUT_INSERT,
		.enabled = false,
		.adc_thres = {
				.min_threshold = 20,
				.max_threshold = 40,
			},
	},
	{
		.accessory = OTHC_ANC_HEADPHONE,
		.detect_flags = OTHC_MICBIAS_DETECT | OTHC_GPIO_DETECT |
							OTHC_SWITCH_DETECT,
		.gpio = PM8058_LINE_IN_DET_GPIO,
		.active_low = 1,
		.key_code = SW_HEADPHONE_INSERT,
		.enabled = true,
	},
	{
		.accessory = OTHC_ANC_HEADSET,
		.detect_flags = OTHC_MICBIAS_DETECT | OTHC_GPIO_DETECT,
		.gpio = PM8058_LINE_IN_DET_GPIO,
		.active_low = 1,
		.key_code = SW_HEADPHONE_INSERT,
		.enabled = true,
	},
	{
		.accessory = OTHC_HEADPHONE,
		.detect_flags = OTHC_MICBIAS_DETECT | OTHC_SWITCH_DETECT,
		.key_code = SW_HEADPHONE_INSERT,
		.enabled = true,
	},
	{
		.accessory = OTHC_MICROPHONE,
		.detect_flags = OTHC_GPIO_DETECT,
		.gpio = PM8058_LINE_IN_DET_GPIO,
		.active_low = 1,
		.key_code = SW_MICROPHONE_INSERT,
		.enabled = true,
	},
	{
		.accessory = OTHC_HEADSET,
		.detect_flags = OTHC_MICBIAS_DETECT,
		.key_code = SW_HEADPHONE_INSERT,
		.enabled = true,
	},
};

static struct othc_switch_info switch_info[] = {
	{
		.min_adc_threshold = 0,
		.max_adc_threshold = 100,
		.key_code = KEY_PLAYPAUSE,
	},
	{
		.min_adc_threshold = 100,
		.max_adc_threshold = 200,
		.key_code = KEY_REWIND,
	},
	{
		.min_adc_threshold = 200,
		.max_adc_threshold = 500,
		.key_code = KEY_FASTFORWARD,
	},
};

static struct othc_n_switch_config switch_config = {
	.voltage_settling_time_ms = 0,
	.num_adc_samples = 3,
	.adc_channel = CHANNEL_ADC_HDSET,
	.switch_info = switch_info,
	.num_keys = ARRAY_SIZE(switch_info),
	.default_sw_en = true,
	.default_sw_idx = 0,
};

static struct hsed_bias_config hsed_bias_config = {
	/* HSED mic bias config info */
	.othc_headset = OTHC_HEADSET_NO,
	.othc_lowcurr_thresh_uA = 100,
	.othc_highcurr_thresh_uA = 600,
	.othc_hyst_prediv_us = 7800,
	.othc_period_clkdiv_us = 62500,
	.othc_hyst_clk_us = 121000,
	.othc_period_clk_us = 312500,
	.othc_wakeup = 1,
};

static struct othc_hsed_config hsed_config_1 = {
	.hsed_bias_config = &hsed_bias_config,
	/*
	 * The detection delay and switch reporting delay are
	 * required to encounter a hardware bug (spurious switch
	 * interrupts on slow insertion/removal of the headset).
	 * This will introduce a delay in reporting the accessory
	 * insertion and removal to the userspace.
	 */
	.detection_delay_ms = 1500,
	/* Switch info */
	.switch_debounce_ms = 1500,
	.othc_support_n_switch = false,
	.switch_config = &switch_config,
	.ir_gpio = -1,
	/* Accessory info */
	.accessories_support = true,
	.accessories = othc_accessories,
	.othc_num_accessories = ARRAY_SIZE(othc_accessories),
};
#endif

static struct othc_regulator_config othc_reg = {
	.regulator	 = "8058_l5",
#ifdef CONFIG_LGE_PMIC8058_REGULATOR
	.max_uV		 = 2850000,
	.min_uV		 = 2850000,
#else
/* LGE_CHANGE_S platform@lge.com H/W team asked to change 2850000 -> 2900000 */
	.max_uV		 = 2950000, //2850000
	.min_uV		 = 2950000, //2850000
/* LGE_CHANGE_E platform@lge.com H/W team asked to change 2850000 -> 2950000 */
#endif
};

/* MIC_BIAS0 is configured as normal MIC BIAS */
static struct pmic8058_othc_config_pdata othc_config_pdata_0 = {
	.micbias_select = OTHC_MICBIAS_0,
	.micbias_capability = OTHC_MICBIAS,
	.micbias_enable = OTHC_SIGNAL_OFF,
	.micbias_regulator = &othc_reg,
};

/* MIC_BIAS1 is configured as HSED_BIAS for OTHC */
static struct pmic8058_othc_config_pdata othc_config_pdata_1 = {
	.micbias_select = OTHC_MICBIAS_1,
#ifdef CONFIG_LGE_HEADSET_DETECTION_FSA8008
	.micbias_capability = OTHC_MICBIAS,
	.micbias_enable = OTHC_SIGNAL_OFF,
#else
	.micbias_capability = OTHC_MICBIAS_HSED,
	.micbias_enable = OTHC_SIGNAL_PWM_TCXO,
	.micbias_regulator = &othc_reg,
	.hsed_config = &hsed_config_1,
	.hsed_name = "8660_handset",
#endif
	.micbias_regulator = &othc_reg,
};

/* MIC_BIAS2 is configured as normal MIC BIAS */
static struct pmic8058_othc_config_pdata othc_config_pdata_2 = {
	.micbias_select = OTHC_MICBIAS_2,
	.micbias_capability = OTHC_MICBIAS,
	.micbias_enable = OTHC_SIGNAL_OFF,
	.micbias_regulator = &othc_reg,
};


#ifndef CONFIG_LGE_HEADSET_DETECTION_FSA8008
static void __init msm8x60_init_pm8058_othc(void)
{
	int i;

	if (SOCINFO_VERSION_MAJOR(socinfo_get_version()) == 2 ||
		machine_is_msm8x60_fluid() || machine_is_lge_i_board() || machine_is_msm8x60_fusion() ||
		machine_is_msm8x60_fusn_ffa() || machine_is_lge_325_board()) {
		/* 3-switch headset supported only by V2 FFA and FLUID */
		hsed_config_1.accessories_adc_support = true,
		/* ADC based accessory detection works only on V2 and FLUID */
		hsed_config_1.accessories_adc_channel = CHANNEL_ADC_HDSET,
		hsed_config_1.othc_support_n_switch = true;
	}

	/* IR GPIO is absent on FLUID */
	if (machine_is_msm8x60_fluid())
		hsed_config_1.ir_gpio = -1;

	for (i = 0; i < ARRAY_SIZE(othc_accessories); i++) {
		if (machine_is_msm8x60_fluid()) {
			switch (othc_accessories[i].accessory) {
			case OTHC_ANC_HEADPHONE:
			case OTHC_ANC_HEADSET:
				othc_accessories[i].gpio = GPIO_HEADSET_DET_N;
				break;
			case OTHC_MICROPHONE:
				othc_accessories[i].enabled = false;
				break;
			case OTHC_SVIDEO_OUT:
				othc_accessories[i].enabled = true;
				hsed_config_1.video_out_gpio = GPIO_HS_SW_DIR;
				break;
			}
		}
	}
}
#endif
#endif


static int pm8058_pwm_config(struct pwm_device *pwm, int ch, int on)
{
	struct pm_gpio pwm_gpio_config = {
		.direction      = PM_GPIO_DIR_OUT,
		.output_buffer  = PM_GPIO_OUT_BUF_CMOS,
		.output_value   = 0,
		.pull           = PM_GPIO_PULL_NO,
		.vin_sel        = PM8058_GPIO_VIN_VPH,
		.out_strength   = PM_GPIO_STRENGTH_HIGH,
		.function       = PM_GPIO_FUNC_2,
	};

	int rc = -EINVAL;
	int id, mode, max_mA;

	id = mode = max_mA = 0;
	switch (ch) {
	case 0:
	case 1:
	case 2:
		if (on) {
			id = 24 + ch;
			rc = pm8xxx_gpio_config(PM8058_GPIO_PM_TO_SYS(id - 1),
							&pwm_gpio_config);
			if (rc)
				pr_err("%s: pm8xxx_gpio_config(%d): rc=%d\n",
					__func__, id, rc);
		}
		break;

	case 6:
		id = PM_PWM_LED_FLASH;
		mode = PM_PWM_CONF_PWM1;
		max_mA = 300;
		break;

	case 7:
		id = PM_PWM_LED_FLASH1;
		mode = PM_PWM_CONF_PWM1;
		max_mA = 300;
		break;

	default:
		break;
	}

	if (ch >= 6 && ch <= 7) {
		if (!on) {
			mode = PM_PWM_CONF_NONE;
			max_mA = 0;
		}
		rc = pm8058_pwm_config_led(pwm, id, mode, max_mA);
		if (rc)
			pr_err("%s: pm8058_pwm_config_led(ch=%d): rc=%d\n",
			       __func__, ch, rc);
	}
	return rc;

}

static struct pm8058_pwm_pdata pm8058_pwm_data = {
	.config		= pm8058_pwm_config,
};

#define PM8058_GPIO_INT           88

#if defined(CONFIG_LGE_PMIC8058_KEYPAD) || defined(CONFIG_MACH_LGE_325_BOARD)
static struct pmic8058_led pmic8058_flash_leds[] = {
	#if defined(CONFIG_MACH_LGE_325_BOARD_DCM)
	[0] = {
		.name		= "button-backlight",//"keypad:drv",
		.max_brightness = 2,	    // DCM HW request, 2012-0412, key led brightness 4mA
		.id		= PMIC8058_ID_LED_2,
	},
	#else
	[0] = {
		.name		= "button-backlight",//"keypad:drv",
		.max_brightness = 7,	    // yeonhwa.so@lge.com, 2011-1206, key led brightness 14mA
		.id		= PMIC8058_ID_LED_2,
	},
	#endif
	[1] = {
		.name		= "keyboard-backlight", //Power Button LED light
		.max_brightness = 10,
		.id		= PMIC8058_ID_LED_0,
	},/* 40 mA led0 sink */
};
#else
static struct pmic8058_led pmic8058_flash_leds[] = {
	[0] = {
		.name		= "camera:flash0",
		.max_brightness = 15,
		.id		= PMIC8058_ID_FLASH_LED_0,
	},
	[1] = {
		.name		= "camera:flash1",
		.max_brightness = 15,
		.id		= PMIC8058_ID_FLASH_LED_1,
	},
#ifdef CONFIG_LEDS_PMIC8058
	[2] = {
		.name		= "button-backlight",//"keypad:drv",
		.max_brightness = 15,
		.id		= PMIC8058_ID_LED_2,
	},/* 300 mA keypad drv sink */
#endif
};
#endif


static struct pmic8058_leds_platform_data pm8058_flash_leds_data = {
	.num_leds = ARRAY_SIZE(pmic8058_flash_leds),
	.leds	= pmic8058_flash_leds,
};
#if defined(CONFIG_LGE_PMIC8058_KEYPAD) && (defined(CONFIG_MACH_LGE_325_BOARD_SKT) || defined(CONFIG_MACH_LGE_325_BOARD_LGU))
static struct pmic8058_led pmic8058_flash_leds_for_old_HW[] = {
	[0] = {
		.name		= "button-backlight",//"keypad:drv",
		.max_brightness = 1,
		.id		= PMIC8058_ID_LED_KB_LIGHT,
	},
	[1] = {
		.name		= "keyboard-backlight", //Power Button LED light
		.max_brightness = 10,
		.id		= PMIC8058_ID_LED_0,
	},/* 40 mA led0 sink */
};

static struct pmic8058_leds_platform_data pm8058_flash_leds_data_for_old_HW= {
	.num_leds = ARRAY_SIZE(pmic8058_flash_leds),
	.leds	= pmic8058_flash_leds_for_old_HW,
};
#endif

static struct pm8xxx_misc_platform_data pm8058_misc_pdata = {
	.priority		= 0,
};

static struct pm8xxx_irq_platform_data pm8058_irq_pdata = {
	.irq_base		= PM8058_IRQ_BASE,
	.devirq			= MSM_GPIO_TO_INT(PM8058_GPIO_INT),
	.irq_trigger_flag	= IRQF_TRIGGER_LOW,
};

static struct pm8xxx_gpio_platform_data pm8058_gpio_pdata = {
	.gpio_base	= PM8058_GPIO_PM_TO_SYS(0),
};

static struct pm8xxx_mpp_platform_data pm8058_mpp_pdata = {
	.mpp_base	= PM8058_MPP_PM_TO_SYS(0),
};

#if 1 /* platform-bsp@lge.com : pmic8058 charger */
static struct pmic8058_charger_data pm8058_charger_pdata = {
        .charger_data_valid = false,
};
#endif

static struct pm8058_platform_data pm8058_platform_data = {
	.irq_pdata		= &pm8058_irq_pdata,
	.gpio_pdata		= &pm8058_gpio_pdata,
	.mpp_pdata		= &pm8058_mpp_pdata,
	.rtc_pdata		= &pm8058_rtc_pdata,
	.pwrkey_pdata		= &pm8058_pwrkey_pdata,
	.othc0_pdata		= &othc_config_pdata_0,
	.othc1_pdata		= &othc_config_pdata_1,
	.othc2_pdata		= &othc_config_pdata_2,
	.pwm_pdata		= &pm8058_pwm_data,
	.misc_pdata		= &pm8058_misc_pdata,
#ifdef CONFIG_SENSORS_MSM_ADC
	.xoadc_pdata		= &pm8058_xoadc_pdata,
#endif
#if 1 /* platform-bsp@lge.com : pmic8058 charger */
	.charger_pdata		= &pm8058_charger_pdata,
#endif
};

#ifdef CONFIG_MSM_SSBI
static struct msm_ssbi_platform_data msm8x60_ssbi_pm8058_pdata __devinitdata = {
	.controller_type = MSM_SBI_CTRL_PMIC_ARBITER,
	.slave	= {
		.name			= "pm8058-core",
		.platform_data		= &pm8058_platform_data,
	},
};
#endif
#endif  /* CONFIG_PMIC8058 */

#if defined(CONFIG_TOUCHDISC_VTD518_SHINETSU) || \
		defined(CONFIG_TOUCHDISC_VTD518_SHINETSU_MODULE)
#define TDISC_I2C_SLAVE_ADDR	0x67
#define PMIC_GPIO_TDISC		PM8058_GPIO_PM_TO_SYS(5)
#define TDISC_INT		PM8058_GPIO_IRQ(PM8058_IRQ_BASE, 5)

static const char *vregs_tdisc_name[] = {
	"8058_l5",
	"8058_s3",
};

static const int vregs_tdisc_val[] = {
	2850000,/* uV */
	1800000,
};
static struct regulator *vregs_tdisc[ARRAY_SIZE(vregs_tdisc_name)];

static int tdisc_shinetsu_setup(void)
{
	int rc, i;

	rc = gpio_request(PMIC_GPIO_TDISC, "tdisc_interrupt");
	if (rc) {
		pr_err("%s: gpio_request failed for PMIC_GPIO_TDISC\n",
								__func__);
		return rc;
	}

	rc = gpio_request(GPIO_JOYSTICK_EN, "tdisc_oe");
	if (rc) {
		pr_err("%s: gpio_request failed for GPIO_JOYSTICK_EN\n",
							__func__);
		goto fail_gpio_oe;
	}

	rc = gpio_direction_output(GPIO_JOYSTICK_EN, 1);
	if (rc) {
		pr_err("%s: gpio_direction_output failed for GPIO_JOYSTICK_EN\n",
								__func__);
		gpio_free(GPIO_JOYSTICK_EN);
		goto fail_gpio_oe;
	}

	for (i = 0; i < ARRAY_SIZE(vregs_tdisc_name); i++) {
		vregs_tdisc[i] = regulator_get(NULL, vregs_tdisc_name[i]);
		if (IS_ERR(vregs_tdisc[i])) {
			printk(KERN_ERR "%s: regulator get %s failed (%ld)\n",
				__func__, vregs_tdisc_name[i],
				PTR_ERR(vregs_tdisc[i]));
			rc = PTR_ERR(vregs_tdisc[i]);
			goto vreg_get_fail;
		}

		rc = regulator_set_voltage(vregs_tdisc[i],
				vregs_tdisc_val[i], vregs_tdisc_val[i]);
		if (rc) {
			printk(KERN_ERR "%s: regulator_set_voltage() = %d\n",
				__func__, rc);
			goto vreg_set_voltage_fail;
		}
	}

	return rc;
vreg_set_voltage_fail:
	i++;
vreg_get_fail:
	while (i)
		regulator_put(vregs_tdisc[--i]);
fail_gpio_oe:
	gpio_free(PMIC_GPIO_TDISC);
	return rc;
}

static void tdisc_shinetsu_release(void)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(vregs_tdisc_name); i++)
		regulator_put(vregs_tdisc[i]);

	gpio_free(PMIC_GPIO_TDISC);
	gpio_free(GPIO_JOYSTICK_EN);
}

static int tdisc_shinetsu_enable(void)
{
	int i, rc = -EINVAL;

	for (i = 0; i < ARRAY_SIZE(vregs_tdisc_name); i++) {
		rc = regulator_enable(vregs_tdisc[i]);
		if (rc < 0) {
			printk(KERN_ERR "%s: vreg %s enable failed (%d)\n",
				__func__, vregs_tdisc_name[i], rc);
			goto vreg_fail;
		}
	}

	/* Enable the OE (output enable) gpio */
	gpio_set_value_cansleep(GPIO_JOYSTICK_EN, 1);
	/* voltage and gpio stabilization delay */
	msleep(50);

	return 0;
vreg_fail:
	while (i)
		regulator_disable(vregs_tdisc[--i]);
	return rc;
}

static int tdisc_shinetsu_disable(void)
{
	int i, rc;

	for (i = 0; i < ARRAY_SIZE(vregs_tdisc_name); i++) {
		rc = regulator_disable(vregs_tdisc[i]);
		if (rc < 0) {
			printk(KERN_ERR "%s: vreg %s disable failed (%d)\n",
				__func__, vregs_tdisc_name[i], rc);
			goto tdisc_reg_fail;
		}
	}

	/* Disable the OE (output enable) gpio */
	gpio_set_value_cansleep(GPIO_JOYSTICK_EN, 0);

	return 0;

tdisc_reg_fail:
	while (i)
		regulator_enable(vregs_tdisc[--i]);
	return rc;
}

static struct tdisc_abs_values tdisc_abs = {
	.x_max = 32,
	.y_max = 32,
	.x_min = -32,
	.y_min = -32,
	.pressure_max = 32,
	.pressure_min = 0,
};

static struct tdisc_platform_data tdisc_data = {
	.tdisc_setup = tdisc_shinetsu_setup,
	.tdisc_release = tdisc_shinetsu_release,
	.tdisc_enable = tdisc_shinetsu_enable,
	.tdisc_disable = tdisc_shinetsu_disable,
	.tdisc_wakeup  = 0,
	.tdisc_gpio = PMIC_GPIO_TDISC,
	.tdisc_report_keys = true,
	.tdisc_report_relative = true,
	.tdisc_report_absolute = false,
	.tdisc_report_wheel = false,
	.tdisc_reverse_x = false,
	.tdisc_reverse_y = true,
	.tdisc_abs  = &tdisc_abs,
};

static struct i2c_board_info msm_i2c_gsbi3_tdisc_info[] = {
	{
		I2C_BOARD_INFO("vtd518", TDISC_I2C_SLAVE_ADDR),
		.irq =  TDISC_INT,
		.platform_data = &tdisc_data,
	},
};
#endif

#define PM_GPIO_CDC_RST_N 20
#define GPIO_CDC_RST_N PM8058_GPIO_PM_TO_SYS(PM_GPIO_CDC_RST_N)

static struct regulator *vreg_timpani_1;
static struct regulator *vreg_timpani_2;

static unsigned int msm_timpani_setup_power(void)
{
	int rc;

	vreg_timpani_1 = regulator_get(NULL, "8058_l0");
	if (IS_ERR(vreg_timpani_1)) {
		pr_err("%s: Unable to get 8058_l0\n", __func__);
		return -ENODEV;
	}

	vreg_timpani_2 = regulator_get(NULL, "8058_s3");
	if (IS_ERR(vreg_timpani_2)) {
		pr_err("%s: Unable to get 8058_s3\n", __func__);
		regulator_put(vreg_timpani_1);
		return -ENODEV;
	}

	rc = regulator_set_voltage(vreg_timpani_1, 1200000, 1200000);
	if (rc) {
		pr_err("%s: unable to set L0 voltage to 1.2V\n", __func__);
		goto fail;
	}

	rc = regulator_set_voltage(vreg_timpani_2, 1800000, 1800000);
	if (rc) {
		pr_err("%s: unable to set S3 voltage to 1.8V\n", __func__);
		goto fail;
	}

	rc = regulator_enable(vreg_timpani_1);
	if (rc) {
		pr_err("%s: Enable regulator 8058_l0 failed\n", __func__);
		goto fail;
	}

	/* The settings for LDO0 should be set such that
	*  it doesn't require to reset the timpani. */
	rc = regulator_set_optimum_mode(vreg_timpani_1, 5000);
	if (rc < 0) {
		pr_err("Timpani regulator optimum mode setting failed\n");
		goto fail;
	}

	rc = regulator_enable(vreg_timpani_2);
	if (rc) {
		pr_err("%s: Enable regulator 8058_s3 failed\n", __func__);
		regulator_disable(vreg_timpani_1);
		goto fail;
	}

	rc = gpio_request(GPIO_CDC_RST_N, "CDC_RST_N");
	if (rc) {
		pr_err("%s: GPIO Request %d failed\n", __func__,
			GPIO_CDC_RST_N);
		regulator_disable(vreg_timpani_1);
		regulator_disable(vreg_timpani_2);
		goto fail;
	} else {
		gpio_direction_output(GPIO_CDC_RST_N, 1);
		usleep_range(1000, 1050);
		gpio_direction_output(GPIO_CDC_RST_N, 0);
		usleep_range(1000, 1050);
		gpio_direction_output(GPIO_CDC_RST_N, 1);
		gpio_free(GPIO_CDC_RST_N);
	}
	return rc;

fail:
	regulator_put(vreg_timpani_1);
	regulator_put(vreg_timpani_2);
	return rc;
}

static void msm_timpani_shutdown_power(void)
{
	int rc;

	rc = regulator_disable(vreg_timpani_1);
	if (rc)
		pr_err("%s: Disable regulator 8058_l0 failed\n", __func__);

	regulator_put(vreg_timpani_1);

	rc = regulator_disable(vreg_timpani_2);
	if (rc)
		pr_err("%s: Disable regulator 8058_s3 failed\n", __func__);

	regulator_put(vreg_timpani_2);
}

/* Power analog function of codec */
static struct regulator *vreg_timpani_cdc_apwr;
static int msm_timpani_codec_power(int vreg_on)
{
	int rc = 0;

	if (!vreg_timpani_cdc_apwr) {

		vreg_timpani_cdc_apwr = regulator_get(NULL, "8058_s4");

		if (IS_ERR(vreg_timpani_cdc_apwr)) {
			pr_err("%s: vreg_get failed (%ld)\n",
			__func__, PTR_ERR(vreg_timpani_cdc_apwr));
			rc = PTR_ERR(vreg_timpani_cdc_apwr);
			return rc;
		}
	}

	if (vreg_on) {

		rc = regulator_set_voltage(vreg_timpani_cdc_apwr,
				2200000, 2200000);
		if (rc) {
			pr_err("%s: unable to set 8058_s4 voltage to 2.2 V\n",
					__func__);
			goto vreg_fail;
		}

		rc = regulator_enable(vreg_timpani_cdc_apwr);
		if (rc) {
			pr_err("%s: vreg_enable failed %d\n", __func__, rc);
			goto vreg_fail;
		}
	} else {
		rc = regulator_disable(vreg_timpani_cdc_apwr);
		if (rc) {
			pr_err("%s: vreg_disable failed %d\n",
			__func__, rc);
			goto vreg_fail;
		}
	}

	return 0;

vreg_fail:
	regulator_put(vreg_timpani_cdc_apwr);
	vreg_timpani_cdc_apwr = NULL;
	return rc;
}

static struct marimba_codec_platform_data timpani_codec_pdata = {
	.marimba_codec_power =  msm_timpani_codec_power,
};

#define TIMPANI_SLAVE_ID_CDC_ADDR		0X77
#define TIMPANI_SLAVE_ID_QMEMBIST_ADDR		0X66

static struct marimba_platform_data timpani_pdata = {
	.slave_id[MARIMBA_SLAVE_ID_CDC]	= TIMPANI_SLAVE_ID_CDC_ADDR,
	.slave_id[MARIMBA_SLAVE_ID_QMEMBIST] = TIMPANI_SLAVE_ID_QMEMBIST_ADDR,
	.marimba_setup = msm_timpani_setup_power,
	.marimba_shutdown = msm_timpani_shutdown_power,
	.codec = &timpani_codec_pdata,
	.tsadc_ssbi_adap = MARIMBA_SSBI_ADAP,
};

#define TIMPANI_I2C_SLAVE_ADDR	0xD

static struct i2c_board_info msm_i2c_gsbi7_timpani_info[] = {
	{
		I2C_BOARD_INFO("timpani", TIMPANI_I2C_SLAVE_ADDR),
		.platform_data = &timpani_pdata,
	},
};

#ifdef CONFIG_SND_SOC_WM8903
static struct wm8903_platform_data wm8903_pdata = {
	.gpio_cfg[2] = 0x3A8,
};

#define WM8903_I2C_SLAVE_ADDR 0x34
static struct i2c_board_info wm8903_codec_i2c_info[] = {
	{
		I2C_BOARD_INFO("wm8903", WM8903_I2C_SLAVE_ADDR >> 1),
		.platform_data = &wm8903_pdata,
	},
};
#endif
#ifdef CONFIG_PMIC8901

#define PM8901_GPIO_INT           91
/*
 * Consumer specific regulator names:
 *			 regulator name		consumer dev_name
 */

#ifdef CONFIG_LGE_PMIC8058_MPP
;
#else
static struct regulator_consumer_supply vreg_consumers_8901_USB_OTG[] = {
	REGULATOR_SUPPLY("8901_usb_otg",	NULL),
};
static struct regulator_consumer_supply vreg_consumers_8901_HDMI_MVS[] = {
	REGULATOR_SUPPLY("8901_hdmi_mvs",	NULL),
};
#endif

#define PM8901_VREG_INIT(_id, _min_uV, _max_uV, _modes, _ops, _apply_uV, \
			 _always_on) \
	{ \
		.init_data = { \
			.constraints = { \
				.valid_modes_mask = _modes, \
				.valid_ops_mask = _ops, \
				.min_uV = _min_uV, \
				.max_uV = _max_uV, \
				.input_uV = _min_uV, \
				.apply_uV = _apply_uV, \
				.always_on = _always_on, \
			}, \
			.consumer_supplies = vreg_consumers_8901_##_id, \
			.num_consumer_supplies = \
				ARRAY_SIZE(vreg_consumers_8901_##_id), \
		}, \
		.id = PM8901_VREG_ID_##_id, \
	}

#define PM8901_VREG_INIT_VS(_id) \
	PM8901_VREG_INIT(_id, 0, 0, REGULATOR_MODE_NORMAL, \
			REGULATOR_CHANGE_STATUS, 0, 0)

static struct pm8901_vreg_pdata pm8901_vreg_init[] = {
#ifdef CONFIG_LGE_PMIC8058_MPP
//we don't use
#else
	PM8901_VREG_INIT_VS(USB_OTG),
	PM8901_VREG_INIT_VS(HDMI_MVS),
#endif
};

static struct pm8xxx_misc_platform_data pm8901_misc_pdata = {
	.priority		= 1,
};

static struct pm8xxx_irq_platform_data pm8901_irq_pdata = {
	.irq_base		= PM8901_IRQ_BASE,
	.devirq			= MSM_GPIO_TO_INT(PM8901_GPIO_INT),
	.irq_trigger_flag	= IRQF_TRIGGER_LOW,
};

static struct pm8xxx_mpp_platform_data pm8901_mpp_pdata = {
	.mpp_base		= PM8901_MPP_PM_TO_SYS(0),
};

static struct pm8901_platform_data pm8901_platform_data = {
	.irq_pdata		= &pm8901_irq_pdata,
	.mpp_pdata		= &pm8901_mpp_pdata,
	.regulator_pdatas	= pm8901_vreg_init,
	.num_regulators		= ARRAY_SIZE(pm8901_vreg_init),
	.misc_pdata		= &pm8901_misc_pdata,
};

static struct msm_ssbi_platform_data msm8x60_ssbi_pm8901_pdata __devinitdata = {
	.controller_type = MSM_SBI_CTRL_PMIC_ARBITER,
	.slave	= {
		.name = "pm8901-core",
		.platform_data = &pm8901_platform_data,
	},
};
#endif /* CONFIG_PMIC8901 */

#if defined(CONFIG_MARIMBA_CORE)

#endif /* CONFIG_MAIMBA_CORE */

#ifdef CONFIG_LGE_MHL_SII9244

static struct regulator *vreg_l23_mhl; // VREG_L23 - 1.2 V
static struct regulator *vreg_l3_mhl;  // VREG_L3 - 3.3 V
static struct regulator *vreg_l12_mhl; // VREG_L12 - 1.8V

static int mhl_power_onoff(int on)
{
	static bool power_state = 0;
	int rc =0;

	if (power_state == on)
	{
		printk("sii_power_state is already %s ", power_state?"on":"off");
		return rc;
	}

	power_state = on;

	if(on)
	{
		printk("sii9234_cfg_power on\n");

		rc = regulator_enable(vreg_l23_mhl);	 // VREG_L23 - 1.2 V
		if (rc) {
			pr_err("%s: l25 vreg enable failed (%d)\n", __func__, rc);
			return rc;
		}

		rc = regulator_enable(vreg_l3_mhl);		// VREG_L3 - 3.3 V
		if (rc) {
			pr_err("%s: l2 vreg enable failed (%d)\n", __func__, rc);
			return rc;
		}


		rc = regulator_enable(vreg_l12_mhl);	// VREG_L12 - 1.8V
		if (rc) {
			pr_err("%s: l2 vreg enable failed (%d)\n", __func__, rc);
			return rc;
		}
	}
	else
	{
		printk("sii9234_cfg_power off\n");

		rc = regulator_disable(vreg_l23_mhl);
		if (rc) {
			pr_err("%s: vreg_l23_mhl vreg enable failed (%d)\n", __func__, rc);
			return rc;
		}

		rc = regulator_disable(vreg_l3_mhl);
		if (rc) {
			pr_err("%s: vreg_l3_mhl vreg enable failed (%d)\n", __func__, rc);
			return rc;
		}

		rc = regulator_disable(vreg_l12_mhl);
		if (rc) {
			pr_err("%s: vreg_l12_mhl vreg enable failed (%d)\n", __func__, rc);
			return rc;
		}
	}

	return rc;

}

static int mhl_power_config(void)
{
	int rc = 0;

	printk("%s\n",__func__);

	vreg_l23_mhl = regulator_get(NULL, "8058_l23");
	if (IS_ERR(vreg_l23_mhl)) {
		rc = PTR_ERR(vreg_l23_mhl);
		pr_err("%s: vreg_l23_mhl get failed (%d)\n", __func__, rc);
		return rc;
	}

	rc = regulator_set_voltage(vreg_l23_mhl, 1200000, 1200000);
	if (rc) {
		pr_err("%s: vreg_l23_mhl set level failed (%d)\n", __func__, rc);
		return rc;
	}

	vreg_l3_mhl = regulator_get(NULL, "8058_l3");
	if (IS_ERR(vreg_l3_mhl)) {
		rc = PTR_ERR(vreg_l3_mhl);
		pr_err("%s: vreg_l3_mhl get failed (%d)\n", __func__, rc);
		return rc;
	}

	rc = regulator_set_voltage(vreg_l3_mhl, 3300000, 3300000);
	if (rc) {
		pr_err("%s: vreg_l3_mhl set level failed (%d)\n", __func__, rc);
		return rc;
	}

	vreg_l12_mhl = regulator_get(NULL, "8058_l12");
	if (IS_ERR(vreg_l12_mhl)) {
		rc = PTR_ERR(vreg_l12_mhl);
		pr_err("%s: mvs0 get failed (%d)\n", __func__, rc);
		return rc;
	}

	rc = regulator_set_voltage(vreg_l12_mhl, 1800000, 1800000);
	if (rc) {
		pr_err("%s: vreg_l12_mhl set level failed (%d)\n", __func__, rc);
		return rc;
	}

	return rc;

}


#define GPIO_MHL_INT          		30
#define GPIO_MHL_WAKEUP       	153
#define GPIO_MHL_RESET_N      	142
#define GPIO_MHL_SELECT_CSFB  	139
#define GPIO_MHL_SELECT_SVLTE 	  33
#if defined(CONFIG_MACH_LGE_325_BOARD_LGU) || defined(CONFIG_MACH_LGE_325_BOARD_VZW)
#define GPIO_MHL_SELECT      GPIO_MHL_SELECT_SVLTE // GPIO_MHL_SELECT_CSFB
#else //CONFIG_MACH_LGE_325_BOARD_SKT or CONFIG_MACH_LGE_325_BOARD_DCM
#define GPIO_MHL_SELECT       GPIO_MHL_SELECT_CSFB
#endif

static struct mhl_platform_data mhl_pdata = {
	.is_support = 1,
	.interrupt_pin = GPIO_MHL_INT,
	.reset_pin = GPIO_MHL_RESET_N,
	.select_pin = GPIO_MHL_SELECT,
	.wakeup_pin = GPIO_MHL_WAKEUP,
	.power = mhl_power_onoff,
	.power_config = mhl_power_config,
};

static struct platform_device mhl_sii9244_device = {
	.name = "sii9244_driver",
	.id = 0,
	.dev.platform_data = &mhl_pdata,
};

/*
  CI2CA    LOW          HIGH
    HDMI : 0x92(0x49)   0x96(0x4B)
    MHL  : 0x72(0x39)   0x76(0x3B)
    TPI  : 0x7A(0x3D)   0x7E(0x3F)
    CBUS : 0xC8(0x64)   0xCC(0x66)
*/

static struct i2c_board_info sii9244_p0_mhl_info[] = {
	{
		I2C_BOARD_INFO("sii9244_i2c_page0", 0x39),
	}
};

static struct i2c_board_info sii9244_p1_mhl_info[] = {
	{
		I2C_BOARD_INFO("sii9244_i2c_page1", 0x3D),
	}
};

static struct i2c_board_info sii9244_p2_mhl_info[] = {
	{
		I2C_BOARD_INFO("sii9244_i2c_page2", 0x49),
	}
};

static struct i2c_board_info sii9244_p3_mhl_info[] = {
	{
		I2C_BOARD_INFO("sii9244_i2c_page3", 0x64),
	}
};

#endif /*CONFIG_LGE_MHL_SII9244 */

#ifdef CONFIG_LGE_FUEL_GAUGE
static int max17047_battery_online(void)
{
	return 0;
};

static int max17047_charger_online(void)
{
	return 0;
};

static int max17047_charger_enable(void)
{
	return 0;
};

static struct max17047_platform_data max17047_pdata = {
	.battery_online = max17047_battery_online,
	.charger_online = max17047_charger_online,
	.charger_enable = max17047_charger_enable,
};

static struct i2c_board_info max17047_i2c_info[] = {
	{
#ifdef CONFIG_MACH_LGE_325_BOARD_DCM
		I2C_BOARD_INFO("max17047",0x36),
#else
		I2C_BOARD_INFO("max17040",0x36),
#endif
		.platform_data = &max17047_pdata,
	}
};
#endif

#ifdef CONFIG_I2C
#define I2C_SURF 1
#define I2C_FFA  (1 << 1)
#define I2C_RUMI (1 << 2)
#define I2C_SIM  (1 << 3)
#define I2C_FLUID (1 << 4)
#define I2C_DRAGON (1 << 5)

struct i2c_registry {
	u8                     machs;
	int                    bus;
	struct i2c_board_info *info;
	int                    len;
};

static struct i2c_registry msm8x60_i2c_devices[] __initdata = {
/* neo.kang@lge.com */
#if 0
#ifdef CONFIG_PMIC8058
	{
		I2C_SURF | I2C_FFA | I2C_FLUID | I2C_DRAGON,
		MSM_SSBI1_I2C_BUS_ID,
		pm8058_boardinfo,
		ARRAY_SIZE(pm8058_boardinfo),
	},
#endif
#endif

#if defined(CONFIG_TOUCHDISC_VTD518_SHINETSU) || \
		defined(CONFIG_TOUCHDISC_VTD518_SHINETSU_MODULE)
	{
		I2C_SURF | I2C_FFA | I2C_FLUID | I2C_DRAGON,
		MSM_GSBI3_QUP_I2C_BUS_ID,
		msm_i2c_gsbi3_tdisc_info,
		ARRAY_SIZE(msm_i2c_gsbi3_tdisc_info),
	},
#endif
//20110308, seunghyup.ryoo@lge.com,  [START]
#if defined(CONFIG_LGE_NFC_NXP_PN544PN65N)
	{
		I2C_SURF | I2C_FFA | I2C_FLUID,
		MSM_GSBI3_QUP_I2C_BUS_ID,
		msm_i2c_nxp_nfc_info,
		ARRAY_SIZE(msm_i2c_nxp_nfc_info),
	},
#endif
//20110308, seunghyup.ryoo@lge.com,  [END]
#if 0 // defined (CONFIG_LGE_TOUCHSCREEN_SYNAPTICS_RMI4_I2C) || defined (CONFIG_LGE_325_TOUCH_SYNAPTICS)
       {
               I2C_SURF | I2C_FFA | I2C_FLUID,
               MSM_GSBI1_QUP_I2C_BUS_ID,
               msm_i2c_synaptics_ts_info,
               ARRAY_SIZE(msm_i2c_synaptics_ts_info),
       },
#endif
	{
		I2C_SURF | I2C_FFA | I2C_FLUID,
		MSM_GSBI7_QUP_I2C_BUS_ID,
		msm_i2c_gsbi7_timpani_info,
		ARRAY_SIZE(msm_i2c_gsbi7_timpani_info),
	},
#if defined(CONFIG_MARIMBA_CORE)
#endif /* CONFIG_MARIMBA_CORE */
#if defined(CONFIG_SND_SOC_WM8903) || defined(CONFIG_SND_SOC_WM8903_MODULE)
	{
		I2C_DRAGON,
		MSM_GSBI8_QUP_I2C_BUS_ID,
		wm8903_codec_i2c_info,
		ARRAY_SIZE(wm8903_codec_i2c_info),
	},
#endif
#ifdef CONFIG_LGE_MHL_SII9244
	{
		I2C_SURF | I2C_FFA | I2C_FLUID,
		MSM_GSBI8_QUP_I2C_BUS_ID,
		sii9244_p0_mhl_info,
		ARRAY_SIZE(sii9244_p0_mhl_info),
	},
	{
		I2C_SURF | I2C_FFA | I2C_FLUID,
		MSM_GSBI8_QUP_I2C_BUS_ID,
		sii9244_p1_mhl_info,
		ARRAY_SIZE(sii9244_p1_mhl_info),
	},
	{
		I2C_SURF | I2C_FFA | I2C_FLUID,
		MSM_GSBI8_QUP_I2C_BUS_ID,
		sii9244_p2_mhl_info,
		ARRAY_SIZE(sii9244_p2_mhl_info),
	},
	{
		I2C_SURF | I2C_FFA | I2C_FLUID,
		MSM_GSBI8_QUP_I2C_BUS_ID,
		sii9244_p3_mhl_info,
		ARRAY_SIZE(sii9244_p3_mhl_info),
	},
#endif
#ifdef CONFIG_LGE_FUEL_GAUGE
	{
		I2C_SURF | I2C_FFA | I2C_FLUID,
		MSM_GSBI5_QUP_I2C_BUS_ID,
		max17047_i2c_info,
		ARRAY_SIZE(max17047_i2c_info),
	},
#endif

#ifdef CONFIG_BATTERY_325
{
         I2C_SURF | I2C_FFA | I2C_FLUID,
        MSM_GSBI5_QUP_I2C_BUS_ID,
         bq24160_charger_i2c_info,
         ARRAY_SIZE(bq24160_charger_i2c_info),
         },
#endif

#ifdef CONFIG_LGE_WIRELESS_CHARGER_MAX8971
	{
		I2C_SURF | I2C_FFA | I2C_FLUID,
		MSM_GSBI11_QUP_I2C_BUS_ID,
		max8971_charger_i2c_info,
		ARRAY_SIZE(max8971_charger_i2c_info),
	},
#endif
#ifdef CONFIG_LGE_WIRELESS_CHARGER_BQ24160
	{
		I2C_SURF | I2C_FFA | I2C_FLUID,
		MSM_GSBI11_QUP_I2C_BUS_ID,
		bq24160_charger_i2c_info,
		ARRAY_SIZE(bq24160_charger_i2c_info),
	},
#endif
};
#endif /* CONFIG_I2C */

static void fixup_i2c_configs(void)
{
#ifdef CONFIG_I2C
#endif
}


#if defined (CONFIG_LGE_TOUCH_SYNAPTICS_325) || defined (CONFIG_LGE_TOUCHSCREEN_MELFAS)
#if defined (CONFIG_MACH_LGE_325_BOARD_SKT) || defined (CONFIG_MACH_LGE_325_BOARD_LGU) || defined(CONFIG_MACH_LGE_325_BOARD_VZW)
#define TOUCH_ARBITER	 71
#endif
static struct i2c_registry lge_325_touch_i2c_devices[] __initdata = {
	     {
		 	I2C_SURF | I2C_FFA | I2C_FLUID,
			MSM_GSBI1_QUP_I2C_BUS_ID,
			msm_i2c_synaptics_ts_info,
			ARRAY_SIZE(msm_i2c_synaptics_ts_info)
		},
		{
			I2C_SURF | I2C_FFA | I2C_FLUID,
			MSM_GSBI1_QUP_I2C_BUS_ID,
			msm_i2c_melfas_ts_info,
			ARRAY_SIZE(msm_i2c_melfas_ts_info),
		},
};


int touch_i2c_arbiter;

#ifdef CONFIG_MACH_LGE_325_BOARD_DCM
int TOUCH_ARBITER;
#endif

static void lge325_touch_i2c_register(void)
{
	char *ts_str[] = {"SYNAPTICS", "MELFAS"};

	touch_i2c_arbiter = 0;

#ifdef CONFIG_MACH_LGE_325_BOARD_DCM
if(lge_bd_rev < LGE_REV_D) {
	TOUCH_ARBITER = 63;
#endif
	touch_i2c_arbiter = gpio_request(TOUCH_ARBITER, "Touch Arbiter");

	gpio_tlmm_config(GPIO_CFG(TOUCH_ARBITER, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), GPIO_CFG_ENABLE);


	if (touch_i2c_arbiter) {
		pr_err("%s: TOUCH_ARBITER %d request failed\n",
			__func__, TOUCH_ARBITER);
		} else {
			touch_i2c_arbiter = gpio_direction_input(TOUCH_ARBITER);
			if (!touch_i2c_arbiter) {

				touch_i2c_arbiter = gpio_get_value(TOUCH_ARBITER);
				printk("[%s]LGE 325 Touch Device is %s\n",__FUNCTION__,ts_str[touch_i2c_arbiter]);
				}
		}
#ifdef CONFIG_MACH_LGE_325_BOARD_DCM
} else if (lge_bd_rev >= LGE_REV_D) {
		touch_i2c_arbiter = 0;
		printk("[%s]LGE 325 Touch Device is %s\n",__FUNCTION__,ts_str[touch_i2c_arbiter]);
}
		touch_caps.x_max		=  1535;
		touch_caps.y_max		=  2047;
		touch_role.key_type		=  TOUCH_HARD_KEY;
#elif defined (CONFIG_MACH_LGE_325_BOARD_SKT) || defined (CONFIG_MACH_LGE_325_BOARD_LGU)
		touch_caps.x_max		=  ((lge_bd_rev < LGE_REV_D) ? 1572 : 1535);
		touch_caps.y_max		=  ((lge_bd_rev < LGE_REV_D) ? 2244 : 2047);
		touch_role.key_type		=  ((lge_bd_rev < LGE_REV_D) ? TOUCH_SOFT_KEY : TOUCH_HARD_KEY);
#elif defined (CONFIG_MACH_LGE_325_BOARD_VZW)
		touch_caps.x_max		=  1535;
		touch_caps.y_max		=  2047;
		touch_role.key_type		=  TOUCH_HARD_KEY;
#endif

#if defined (CONFIG_MACH_LGE_325_BOARD_SKT) || defined (CONFIG_MACH_LGE_325_BOARD_LGU)
     if (lge_bd_rev < LGE_REV_D){
	i2c_register_board_info(lge_325_touch_i2c_devices[touch_i2c_arbiter].bus,
		lge_325_touch_i2c_devices[touch_i2c_arbiter].info,
		lge_325_touch_i2c_devices[touch_i2c_arbiter].len);
        }  else {
	i2c_register_board_info(lge_325_touch_i2c_devices[0].bus,
		lge_325_touch_i2c_devices[0].info,
		lge_325_touch_i2c_devices[0].len);
      }
#else
	i2c_register_board_info(lge_325_touch_i2c_devices[0].bus,
		lge_325_touch_i2c_devices[0].info,
		lge_325_touch_i2c_devices[0].len);
#endif

#ifdef CONFIG_MACH_LGE_325_BOARD_DCM
	if(lge_bd_rev < LGE_REV_D)
#endif
	gpio_free(TOUCH_ARBITER);

}
#endif

static void register_i2c_devices(void)
{
#ifdef CONFIG_I2C
	u8 mach_mask = 0;
	int i;
#ifdef CONFIG_MSM_CAMERA_V4L2
	struct i2c_registry msm8x60_camera_i2c_devices = {
		I2C_SURF | I2C_FFA | I2C_FLUID,
		MSM_GSBI4_QUP_I2C_BUS_ID,
		msm8x60_camera_board_info.board_info,
		msm8x60_camera_board_info.num_i2c_board_info,
	};
#endif

	/* Build the matching 'supported_machs' bitmask */
	if (machine_is_msm8x60_surf() || machine_is_msm8x60_fusion())
		mach_mask = I2C_SURF;
	else if (machine_is_msm8x60_ffa() || machine_is_msm8x60_fusn_ffa() || machine_is_lge_i_board() || machine_is_lge_325_board())
		mach_mask = I2C_FFA;
	else if (machine_is_msm8x60_rumi3())
		mach_mask = I2C_RUMI;
	else if (machine_is_msm8x60_sim())
		mach_mask = I2C_SIM;
	else if (machine_is_msm8x60_fluid())
		mach_mask = I2C_FLUID;
	else if (machine_is_msm8x60_dragon())
		mach_mask = I2C_DRAGON;
	else
		pr_err("unmatched machine ID in register_i2c_devices\n");

	/* Run the array and install devices as appropriate */
	for (i = 0; i < ARRAY_SIZE(msm8x60_i2c_devices); ++i) {
		if (msm8x60_i2c_devices[i].machs & mach_mask)
			i2c_register_board_info(msm8x60_i2c_devices[i].bus,
						msm8x60_i2c_devices[i].info,
						msm8x60_i2c_devices[i].len);
	}
#ifdef CONFIG_MSM_CAMERA_V4L2
	if (msm8x60_camera_i2c_devices.machs & mach_mask)
		i2c_register_board_info(msm8x60_camera_i2c_devices.bus,
			msm8x60_camera_i2c_devices.info,
			msm8x60_camera_i2c_devices.len);
#endif
#if defined (CONFIG_LGE_TOUCH_SYNAPTICS_325) || defined (CONFIG_LGE_TOUCHSCREEN_MELFAS)
	/* kyle.jeon@lge.com for various touchscreens */
	lge325_touch_i2c_register();
#endif
	/* neo.kang@lge.com */
	i2c_register_backlight_info();

#ifdef CONFIG_LGE_SENSOR
	i2c_register_input_sensor_info();
#endif
	/* jisun.shin@lge.com Camera Platform*/
#ifdef CONFIG_LGE_CAMERA
	i2c_register_camera_info();
#endif


#endif
}

static void __init msm8x60_init_uart12dm(void)
{
#if !defined(CONFIG_USB_PEHCI_HCD) && !defined(CONFIG_USB_PEHCI_HCD_MODULE)
	/* 0x1D000000 now belongs to EBI2:CS3 i.e. USB ISP Controller */
	void *fpga_mem = ioremap_nocache(0x1D000000, SZ_4K);

	if (!fpga_mem)
		pr_err("%s(): Error getting memory\n", __func__);

	/* Advanced mode */
	writew(0xFFFF, fpga_mem + 0x15C);
	/* FPGA_UART_SEL */
	writew(0, fpga_mem + 0x172);
	/* FPGA_GPIO_CONFIG_117 */
	writew(1, fpga_mem + 0xEA);
	/* FPGA_GPIO_CONFIG_118 */
	writew(1, fpga_mem + 0xEC);
	mb();
	iounmap(fpga_mem);
#endif
}

#define MSM_GSBI9_PHYS		0x19900000
#define MSM_GSBI10_PHYS     0x19A00000
#define GSBI_DUAL_MODE_CODE	0x60

static void __init msm8x60_init_buses(void)
{
#ifdef CONFIG_I2C_QUP
	void *gsbi_mem = ioremap_nocache(0x19C00000, 4);
	/* Setting protocol code to 0x60 for dual UART/I2C in GSBI12 */
	writel_relaxed(0x6 << 4, gsbi_mem);
	/* Ensure protocol code is written before proceeding further */
	mb();
	iounmap(gsbi_mem);

#if defined (CONFIG_LGE_TOUCH_SYNAPTICS_325) || defined (CONFIG_LGE_TOUCHSCREEN_MELFAS)
	msm_gsbi1_qup_i2c_device.dev.platform_data = &msm_gsbi1_qup_i2c_pdata;
#endif
	msm_gsbi3_qup_i2c_device.dev.platform_data = &msm_gsbi3_qup_i2c_pdata;
	msm_gsbi4_qup_i2c_device.dev.platform_data = &msm_gsbi4_qup_i2c_pdata;
#ifdef CONFIG_LGE_FUEL_GAUGE
  msm_gsbi5_qup_i2c_device.dev.platform_data = &msm_gsbi5_qup_i2c_pdata;
#endif
	msm_gsbi7_qup_i2c_device.dev.platform_data = &msm_gsbi7_qup_i2c_pdata;
	msm_gsbi8_qup_i2c_device.dev.platform_data = &msm_gsbi8_qup_i2c_pdata;

#ifdef CONFIG_MSM_GSBI9_UART
	if (machine_is_lge_i_board() || machine_is_msm8x60_fusion() || machine_is_msm8x60_fusn_ffa() || machine_is_lge_325_board()) {
		/* Setting protocol code to 0x60 for dual UART/I2C in GSBI9 */
		gsbi_mem = ioremap_nocache(MSM_GSBI9_PHYS, 4);
		writel_relaxed(GSBI_DUAL_MODE_CODE, gsbi_mem);
		iounmap(gsbi_mem);
		msm_gsbi9_qup_i2c_pdata.use_gsbi_shared_mode = 1;
	}
#endif
	msm_gsbi9_qup_i2c_device.dev.platform_data = &msm_gsbi9_qup_i2c_pdata;
	msm_gsbi12_qup_i2c_device.dev.platform_data = &msm_gsbi12_qup_i2c_pdata;

#ifdef CONFIG_LGE_SENSOR_ACCELEROMETER
	msm_gsbi10_qup_i2c_device.dev.platform_data = &msm_gsbi10_qup_i2c_pdata;
#endif

#if defined (CONFIG_LGE_WIRELESS_CHARGER_MAX8971) || defined (CONFIG_LGE_WIRELESS_CHARGER_BQ24160)
	msm_gsbi11_qup_i2c_device.dev.platform_data = &msm_gsbi11_qup_i2c_pdata;
#endif

#endif
#if defined(CONFIG_SPI_QUP) || defined(CONFIG_SPI_QUP_MODULE)
	msm_gsbi1_qup_spi_device.dev.platform_data = &msm_gsbi1_qup_spi_pdata;
#ifndef CONFIG_LGE_FELICA
	msm_gsbi10_qup_spi_device.dev.platform_data = &msm_gsbi10_qup_spi_pdata;
#endif
#endif
#ifdef CONFIG_I2C_SSBI
	msm_device_ssbi3.dev.platform_data = &msm_ssbi3_pdata;
#endif


//2011-05-05 ella.hwang added for 1Seg Driver SPI porting - [Begins]
#if defined(CONFIG_LGE_BROADCAST_DCM) || defined(CONFIG_LGE_BROADCAST_TDMB)
	msm_gsbi11_qup_spi_device.dev.platform_data = &msm_gsbi11_qup_spi_pdata;
#endif
//2011-05-05 ella.hwang added for 1Seg Driver SPI porting - [Ends]
#ifdef CONFIG_MSM_SSBI
	msm_device_ssbi_pmic1.dev.platform_data =
				&msm8x60_ssbi_pm8058_pdata;
	msm_device_ssbi_pmic2.dev.platform_data =
				&msm8x60_ssbi_pm8901_pdata;
#endif


#if defined(CONFIG_USB_GADGET_MSM_72K) || defined(CONFIG_USB_EHCI_HCD)
	/*
	 * We can not put USB regulators (8058_l6 and 8058_l7) in LPM
	 * when we depend on USB PHY for VBUS/ID notifications. VBUS
	 * and ID notifications are available only on V2 surf and FFA
	 * with a hardware workaround.
	 */
	if (SOCINFO_VERSION_MAJOR(socinfo_get_version()) == 2 &&
			(machine_is_msm8x60_surf() ||
			(machine_is_msm8x60_ffa() &&
			pmic_id_notif_supported)))
		msm_otg_pdata.phy_can_powercollapse = 1;
	msm_device_otg.dev.platform_data = &msm_otg_pdata;
#endif

#ifdef CONFIG_USB_GADGET_MSM_72K
	msm_device_gadget_peripheral.dev.platform_data = &msm_gadget_pdata;
#endif

#ifdef CONFIG_SERIAL_MSM_HS
	msm_uart_dm1_pdata.wakeup_irq = gpio_to_irq(54); /* GSBI6(2) */
	msm_device_uart_dm1.dev.platform_data = &msm_uart_dm1_pdata;
#endif
#ifdef CONFIG_MSM_GSBI9_UART
	if (machine_is_lge_i_board() || machine_is_msm8x60_fusion() || machine_is_msm8x60_fusn_ffa() || machine_is_lge_325_board()) {
		msm_device_uart_gsbi9 = msm_add_gsbi9_uart();
		if (IS_ERR(msm_device_uart_gsbi9))
			pr_err("%s(): Failed to create uart gsbi9 device\n",
								__func__);
	}
#endif

#ifdef CONFIG_LGE_IRDA
    /* Setting protocol code to 0x60 for dual UART/I2C in GSBI8 */
	{
		void *gsbi8_mem = ioremap_nocache(MSM_GSBI8_PHYS, 4);
	    writel(GSBI_DUAL_MODE_CODE, gsbi8_mem);
	    iounmap(gsbi8_mem);
	    msm_gsbi8_qup_i2c_pdata.use_gsbi_shared_mode = 1;

	    if (machine_is_lge_325_board()) {
	        msm_device_uart_gsbi8.dev.platform_data =
	            &msm_uart_gsbi8_pdata;
    }
	}
#endif
#ifdef CONFIG_LGE_FELICA
    /* Setting protocol code to 0x60 for dual UART/I2C in GSBI8 */
    gsbi_mem = ioremap_nocache(MSM_GSBI10_PHYS, 4);
    writel(GSBI_DUAL_MODE_CODE, gsbi_mem);
    iounmap(gsbi_mem);
    msm_gsbi10_qup_i2c_pdata.use_gsbi_shared_mode = 1;

    if (machine_is_lge_i_board() || machine_is_lge_325_board()) {
        msm_device_uart_gsbi10.dev.platform_data =
            &msm_uart_gsbi10_pdata;
        platform_device_register(&msm_device_uart_gsbi10);
    }
#endif

#ifdef CONFIG_MSM_BUS_SCALING

	/* RPM calls are only enabled on V2 */
	if (SOCINFO_VERSION_MAJOR(socinfo_get_version()) == 2) {
		msm_bus_apps_fabric_pdata.rpm_enabled = 1;
		msm_bus_sys_fabric_pdata.rpm_enabled = 1;
		msm_bus_mm_fabric_pdata.rpm_enabled = 1;
		msm_bus_sys_fpb_pdata.rpm_enabled = 1;
		msm_bus_cpss_fpb_pdata.rpm_enabled = 1;
	}

	msm_bus_apps_fabric.dev.platform_data = &msm_bus_apps_fabric_pdata;
	msm_bus_sys_fabric.dev.platform_data = &msm_bus_sys_fabric_pdata;
	msm_bus_mm_fabric.dev.platform_data = &msm_bus_mm_fabric_pdata;
	msm_bus_sys_fpb.dev.platform_data = &msm_bus_sys_fpb_pdata;
	msm_bus_cpss_fpb.dev.platform_data = &msm_bus_cpss_fpb_pdata;
#endif
}

static void __init msm8x60_map_io(void)
{
	msm_shared_ram_phys = MSM_SHARED_RAM_PHYS;
	msm_map_msm8x60_io();

	if (socinfo_init() < 0)
		pr_err("socinfo_init() failed!\n");
}

/*
 * Most segments of the EBI2 bus are disabled by default.
 */
static void __init msm8x60_init_ebi2(void)
{
	uint32_t ebi2_cfg;
	void *ebi2_cfg_ptr;
	struct clk *mem_clk = clk_get_sys("msm_ebi2", "mem_clk");

	if (IS_ERR(mem_clk)) {
		pr_err("%s: clk_get_sys(%s,%s), failed", __func__,
					"msm_ebi2", "mem_clk");
		return;
	}
	clk_enable(mem_clk);
	clk_put(mem_clk);

	ebi2_cfg_ptr = ioremap_nocache(0x1a100000, sizeof(uint32_t));
	if (ebi2_cfg_ptr != 0) {
		ebi2_cfg = readl_relaxed(ebi2_cfg_ptr);

		if (machine_is_msm8x60_surf() || machine_is_msm8x60_ffa() ||
			machine_is_msm8x60_fluid() ||
			machine_is_msm8x60_dragon())
			ebi2_cfg |= (1 << 4) | (1 << 5); /* CS2, CS3 */
		else if (machine_is_msm8x60_sim())
			ebi2_cfg |= (1 << 4); /* CS2 */
		else if (machine_is_msm8x60_rumi3())
			ebi2_cfg |= (1 << 5); /* CS3 */

		writel_relaxed(ebi2_cfg, ebi2_cfg_ptr);
		iounmap(ebi2_cfg_ptr);
	}

	if (machine_is_msm8x60_surf() || machine_is_msm8x60_ffa() ||
	    machine_is_msm8x60_fluid() || machine_is_msm8x60_dragon()) {
		ebi2_cfg_ptr = ioremap_nocache(0x1a110000, SZ_4K);
		if (ebi2_cfg_ptr != 0) {
			/* EBI2_XMEM_CFG:PWRSAVE_MODE off */
			writel_relaxed(0UL, ebi2_cfg_ptr);

			/* CS2: Delay 9 cycles (140ns@64MHz) between SMSC
			 * LAN9221 Ethernet controller reads and writes.
			 * The lowest 4 bits are the read delay, the next
			 * 4 are the write delay. */
			writel_relaxed(0x031F1C99, ebi2_cfg_ptr + 0x10);
#if defined(CONFIG_USB_PEHCI_HCD) || defined(CONFIG_USB_PEHCI_HCD_MODULE)
			/*
			 * RECOVERY=5, HOLD_WR=1
			 * INIT_LATENCY_WR=1, INIT_LATENCY_RD=1
			 * WAIT_WR=1, WAIT_RD=2
			 */
			writel_relaxed(0x51010112, ebi2_cfg_ptr + 0x14);
			/*
			 * HOLD_RD=1
			 * ADV_OE_RECOVERY=0, ADDR_HOLD_ENA=1
			 */
			writel_relaxed(0x01000020, ebi2_cfg_ptr + 0x34);
#else
			/* EBI2 CS3 muxed address/data,
			* two cyc addr enable */
			writel_relaxed(0xA3030020, ebi2_cfg_ptr + 0x34);

#endif
			iounmap(ebi2_cfg_ptr);
		}
	}
}

static void __init msm8x60_init_tlmm(void)
{
	if (machine_is_msm8x60_rumi3())
		msm_gpio_install_direct_irq(0, 0, 1);
}

#if (defined(CONFIG_MMC_MSM_SDC1_SUPPORT)\
	|| defined(CONFIG_MMC_MSM_SDC2_SUPPORT)\
	|| defined(CONFIG_MMC_MSM_SDC3_SUPPORT)\
	|| defined(CONFIG_MMC_MSM_SDC4_SUPPORT)\
	|| defined(CONFIG_MMC_MSM_SDC5_SUPPORT))

/* 8x60 has 5 SDCC controllers */
#define MAX_SDCC_CONTROLLER	5

struct msm_sdcc_gpio {
	/* maximum 10 GPIOs per SDCC controller */
	s16 no;
	/* name of this GPIO */
	const char *name;
	bool always_on;
	bool is_enabled;
};

#ifdef CONFIG_MMC_MSM_SDC1_SUPPORT
static struct msm_sdcc_gpio sdc1_gpio_cfg[] = {
	{159, "sdc1_dat_0"},
	{160, "sdc1_dat_1"},
	{161, "sdc1_dat_2"},
	{162, "sdc1_dat_3"},
#ifdef CONFIG_MMC_MSM_SDC1_8_BIT_SUPPORT
	{163, "sdc1_dat_4"},
	{164, "sdc1_dat_5"},
	{165, "sdc1_dat_6"},
	{166, "sdc1_dat_7"},
#endif
	{167, "sdc1_clk"},
	{168, "sdc1_cmd"}
};
#endif

#ifdef CONFIG_MMC_MSM_SDC2_SUPPORT
static struct msm_sdcc_gpio sdc2_gpio_cfg[] = {
	{143, "sdc2_dat_0"},
	{144, "sdc2_dat_1", 1},
	{145, "sdc2_dat_2"},
	{146, "sdc2_dat_3"},
#ifdef CONFIG_MMC_MSM_SDC2_8_BIT_SUPPORT
	{147, "sdc2_dat_4"},
	{148, "sdc2_dat_5"},
	{149, "sdc2_dat_6"},
	{150, "sdc2_dat_7"},
#endif
	{151, "sdc2_cmd"},
	{152, "sdc2_clk", 1}
};
#endif

#ifdef CONFIG_MMC_MSM_SDC5_SUPPORT
static struct msm_sdcc_gpio sdc5_gpio_cfg[] = {
	{95, "sdc5_cmd"},
	{96, "sdc5_dat_3"},
	{97, "sdc5_clk", 1},
	{98, "sdc5_dat_2"},
	{99, "sdc5_dat_1", 1},
	{100, "sdc5_dat_0"}
};
#endif

struct msm_sdcc_pad_pull_cfg {
	enum msm_tlmm_pull_tgt pull;
	u32 pull_val;
};

struct msm_sdcc_pad_drv_cfg {
	enum msm_tlmm_hdrive_tgt drv;
	u32 drv_val;
};

#ifdef CONFIG_MMC_MSM_SDC3_SUPPORT
static struct msm_sdcc_pad_drv_cfg sdc3_pad_on_drv_cfg[] = {
	{TLMM_HDRV_SDC3_CLK, GPIO_CFG_8MA},
	{TLMM_HDRV_SDC3_CMD, GPIO_CFG_8MA},
	{TLMM_HDRV_SDC3_DATA, GPIO_CFG_8MA}
};

static struct msm_sdcc_pad_pull_cfg sdc3_pad_on_pull_cfg[] = {
	{TLMM_PULL_SDC3_CMD, GPIO_CFG_PULL_UP},
	{TLMM_PULL_SDC3_DATA, GPIO_CFG_PULL_UP}
};

static struct msm_sdcc_pad_drv_cfg sdc3_pad_off_drv_cfg[] = {
	{TLMM_HDRV_SDC3_CLK, GPIO_CFG_2MA},
	{TLMM_HDRV_SDC3_CMD, GPIO_CFG_2MA},
	{TLMM_HDRV_SDC3_DATA, GPIO_CFG_2MA}
};

#ifdef CONFIG_LGE_PM_CURRENT_CONSUMPTION_FIX  //hyogook.lee@lge.com, 2011.08.24, SDC3 DATA Line PU for reduce sleep (200uA)
static struct msm_sdcc_pad_pull_cfg sdc3_pad_off_pull_cfg[] = {
	{TLMM_PULL_SDC3_CMD, GPIO_CFG_PULL_UP},
	{TLMM_PULL_SDC3_DATA, GPIO_CFG_PULL_UP}
};
#else
static struct msm_sdcc_pad_pull_cfg sdc3_pad_off_pull_cfg[] = {
	{TLMM_PULL_SDC3_CMD, GPIO_CFG_PULL_DOWN},
	{TLMM_PULL_SDC3_DATA, GPIO_CFG_PULL_DOWN}
};
#endif

#endif
#ifdef CONFIG_MMC_MSM_SDC4_SUPPORT
static struct msm_sdcc_pad_drv_cfg sdc4_pad_on_drv_cfg[] = {
	{TLMM_HDRV_SDC4_CLK, GPIO_CFG_8MA},
	{TLMM_HDRV_SDC4_CMD, GPIO_CFG_8MA},
	{TLMM_HDRV_SDC4_DATA, GPIO_CFG_8MA}
};

static struct msm_sdcc_pad_pull_cfg sdc4_pad_on_pull_cfg[] = {
	{TLMM_PULL_SDC4_CMD, GPIO_CFG_PULL_UP},
	{TLMM_PULL_SDC4_DATA, GPIO_CFG_PULL_UP}
};

static struct msm_sdcc_pad_drv_cfg sdc4_pad_off_drv_cfg[] = {
	{TLMM_HDRV_SDC4_CLK, GPIO_CFG_2MA},
	{TLMM_HDRV_SDC4_CMD, GPIO_CFG_2MA},
	{TLMM_HDRV_SDC4_DATA, GPIO_CFG_2MA}
};

static struct msm_sdcc_pad_pull_cfg sdc4_pad_off_pull_cfg[] = {
	{TLMM_PULL_SDC4_CMD, GPIO_CFG_PULL_DOWN},
	{TLMM_PULL_SDC4_DATA, GPIO_CFG_PULL_DOWN}
};
#endif

struct msm_sdcc_pin_cfg {
	/*
	 * = 1 if controller pins are using gpios
	 * = 0 if controller has dedicated MSM pins
	 */
	u8 is_gpio;
	u8 cfg_sts;
	u8 gpio_data_size;
	struct msm_sdcc_gpio *gpio_data;
	struct msm_sdcc_pad_drv_cfg *pad_drv_on_data;
	struct msm_sdcc_pad_drv_cfg *pad_drv_off_data;
	struct msm_sdcc_pad_pull_cfg *pad_pull_on_data;
	struct msm_sdcc_pad_pull_cfg *pad_pull_off_data;
	u8 pad_drv_data_size;
	u8 pad_pull_data_size;
	u8 sdio_lpm_gpio_cfg;
};


static struct msm_sdcc_pin_cfg sdcc_pin_cfg_data[MAX_SDCC_CONTROLLER] = {
#ifdef CONFIG_MMC_MSM_SDC1_SUPPORT
	[0] = {
		.is_gpio = 1,
		.gpio_data_size = ARRAY_SIZE(sdc1_gpio_cfg),
		.gpio_data = sdc1_gpio_cfg
	},
#endif
#ifdef CONFIG_MMC_MSM_SDC2_SUPPORT
	[1] = {
		.is_gpio = 1,
		.gpio_data_size = ARRAY_SIZE(sdc2_gpio_cfg),
		.gpio_data = sdc2_gpio_cfg
	},
#endif
#ifdef CONFIG_MMC_MSM_SDC3_SUPPORT
	[2] = {
		.is_gpio = 0,
		.pad_drv_on_data = sdc3_pad_on_drv_cfg,
		.pad_drv_off_data = sdc3_pad_off_drv_cfg,
		.pad_pull_on_data = sdc3_pad_on_pull_cfg,
		.pad_pull_off_data = sdc3_pad_off_pull_cfg,
		.pad_drv_data_size = ARRAY_SIZE(sdc3_pad_on_drv_cfg),
		.pad_pull_data_size = ARRAY_SIZE(sdc3_pad_on_pull_cfg)
	},
#endif
#ifdef CONFIG_MMC_MSM_SDC4_SUPPORT
	[3] = {
		.is_gpio = 0,
		.pad_drv_on_data = sdc4_pad_on_drv_cfg,
		.pad_drv_off_data = sdc4_pad_off_drv_cfg,
		.pad_pull_on_data = sdc4_pad_on_pull_cfg,
		.pad_pull_off_data = sdc4_pad_off_pull_cfg,
		.pad_drv_data_size = ARRAY_SIZE(sdc4_pad_on_drv_cfg),
		.pad_pull_data_size = ARRAY_SIZE(sdc4_pad_on_pull_cfg)
	},
#endif
#ifdef CONFIG_MMC_MSM_SDC5_SUPPORT
	[4] = {
		.is_gpio = 1,
		.gpio_data_size = ARRAY_SIZE(sdc5_gpio_cfg),
		.gpio_data = sdc5_gpio_cfg
	}
#endif
};

static int msm_sdcc_setup_gpio(int dev_id, unsigned int enable)
{
	int rc = 0;
	struct msm_sdcc_pin_cfg *curr;
	int n;

	curr = &sdcc_pin_cfg_data[dev_id - 1];
	if (!curr->gpio_data)
		goto out;

	for (n = 0; n < curr->gpio_data_size; n++) {
		if (enable) {

			if (curr->gpio_data[n].always_on &&
				curr->gpio_data[n].is_enabled)
				continue;
			pr_debug("%s: enable: %s\n", __func__,
					curr->gpio_data[n].name);
			rc = gpio_request(curr->gpio_data[n].no,
				curr->gpio_data[n].name);
			if (rc) {
				pr_err("%s: gpio_request(%d, %s)"
					"failed", __func__,
					curr->gpio_data[n].no,
					curr->gpio_data[n].name);
				goto free_gpios;
			}
			/* set direction as output for all GPIOs */
			rc = gpio_direction_output(
				curr->gpio_data[n].no, 1);
			if (rc) {
				pr_err("%s: gpio_direction_output"
					"(%d, 1) failed\n", __func__,
					curr->gpio_data[n].no);
				goto free_gpios;
			}
			curr->gpio_data[n].is_enabled = 1;
		} else {
			/*
			 * now free this GPIO which will put GPIO
			 * in low power mode and will also put GPIO
			 * in input mode
			 */
			if (curr->gpio_data[n].always_on)
				continue;
			pr_debug("%s: disable: %s\n", __func__,
					curr->gpio_data[n].name);
			gpio_free(curr->gpio_data[n].no);
			curr->gpio_data[n].is_enabled = 0;
		}
	}
	curr->cfg_sts = enable;
	goto out;

free_gpios:
	for (; n >= 0; n--)
		gpio_free(curr->gpio_data[n].no);
out:
	return rc;
}

static int msm_sdcc_setup_pad(int dev_id, unsigned int enable)
{
	int rc = 0;
	struct msm_sdcc_pin_cfg *curr;
	int n;

	curr = &sdcc_pin_cfg_data[dev_id - 1];
	if (!curr->pad_drv_on_data || !curr->pad_pull_on_data)
		goto out;

	if (enable) {
		/*
		 * set up the normal driver strength and
		 * pull config for pads
		 */
		for (n = 0; n < curr->pad_drv_data_size; n++) {
			if (curr->sdio_lpm_gpio_cfg) {
				if (curr->pad_drv_on_data[n].drv ==
						TLMM_HDRV_SDC4_DATA)
					continue;
			}
			msm_tlmm_set_hdrive(curr->pad_drv_on_data[n].drv,
				curr->pad_drv_on_data[n].drv_val);
		}
		for (n = 0; n < curr->pad_pull_data_size; n++) {
			if (curr->sdio_lpm_gpio_cfg) {
				if (curr->pad_pull_on_data[n].pull ==
						TLMM_PULL_SDC4_DATA)
					continue;
			}
			msm_tlmm_set_pull(curr->pad_pull_on_data[n].pull,
				curr->pad_pull_on_data[n].pull_val);
		}
	} else {
		/* set the low power config for pads */
		for (n = 0; n < curr->pad_drv_data_size; n++) {
			if (curr->sdio_lpm_gpio_cfg) {
				if (curr->pad_drv_off_data[n].drv ==
						TLMM_HDRV_SDC4_DATA)
					continue;
			}
			msm_tlmm_set_hdrive(
				curr->pad_drv_off_data[n].drv,
				curr->pad_drv_off_data[n].drv_val);
		}
		for (n = 0; n < curr->pad_pull_data_size; n++) {
			if (curr->sdio_lpm_gpio_cfg) {
				if (curr->pad_pull_off_data[n].pull ==
						TLMM_PULL_SDC4_DATA)
					continue;
			}
			msm_tlmm_set_pull(
				curr->pad_pull_off_data[n].pull,
				curr->pad_pull_off_data[n].pull_val);
		}
	}
	curr->cfg_sts = enable;
out:
	return rc;
}

struct sdcc_reg {
	/* VDD/VCC/VCCQ regulator name on PMIC8058/PMIC8089*/
	const char *reg_name;
	/*
	 * is set voltage supported for this regulator?
	 * 0 = not supported, 1 = supported
	 */
	unsigned char set_voltage_sup;
	/* voltage level to be set */
	unsigned int level;
	/* VDD/VCC/VCCQ voltage regulator handle */
	struct regulator *reg;
	/* is this regulator enabled? */
	bool enabled;
	/* is this regulator needs to be always on? */
	bool always_on;
	/* is operating power mode setting required for this regulator? */
	bool op_pwr_mode_sup;
	/* Load values for low power and high power mode */
	unsigned int lpm_uA;
	unsigned int hpm_uA;
};
/* all SDCC controllers require VDD/VCC voltage */
static struct sdcc_reg sdcc_vdd_reg_data[MAX_SDCC_CONTROLLER];
/* only SDCC1 requires VCCQ voltage */
static struct sdcc_reg sdcc_vccq_reg_data[1];
/* all SDCC controllers may require voting for VDD PAD voltage */
static struct sdcc_reg sdcc_vddp_reg_data[MAX_SDCC_CONTROLLER];

struct sdcc_reg_data {
	struct sdcc_reg *vdd_data; /* keeps VDD/VCC regulator info */
	struct sdcc_reg *vccq_data; /* keeps VCCQ regulator info */
	struct sdcc_reg *vddp_data; /* keeps VDD Pad regulator info */
	unsigned char sts; /* regulator enable/disable status */
};
/* msm8x60 has 5 SDCC controllers */
static struct sdcc_reg_data sdcc_vreg_data[MAX_SDCC_CONTROLLER];

static int msm_sdcc_vreg_init_reg(struct sdcc_reg *vreg)
{
	int rc = 0;

	/* Get the regulator handle */
	vreg->reg = regulator_get(NULL, vreg->reg_name);
	if (IS_ERR(vreg->reg)) {
		rc = PTR_ERR(vreg->reg);
		pr_err("%s: regulator_get(%s) failed. rc=%d\n",
			__func__, vreg->reg_name, rc);
		goto out;
	}

	/* Set the voltage level if required */
	if (vreg->set_voltage_sup) {
		rc = regulator_set_voltage(vreg->reg, vreg->level,
					vreg->level);
		if (rc) {
			pr_err("%s: regulator_set_voltage(%s) failed rc=%d\n",
				__func__, vreg->reg_name, rc);
			goto vreg_put;
		}
	}
	goto out;

vreg_put:
	regulator_put(vreg->reg);
out:
	return rc;
}

static inline void msm_sdcc_vreg_deinit_reg(struct sdcc_reg *vreg)
{
	regulator_put(vreg->reg);
}

/* this init function should be called only once for each SDCC */
static int msm_sdcc_vreg_init(int dev_id, unsigned char init)
{
	int rc = 0;
	struct sdcc_reg *curr_vdd_reg, *curr_vccq_reg, *curr_vddp_reg;
	struct sdcc_reg_data *curr;

	curr = &sdcc_vreg_data[dev_id - 1];
	curr_vdd_reg = curr->vdd_data;
	curr_vccq_reg = curr->vccq_data;
	curr_vddp_reg = curr->vddp_data;

	if (init) {
		/*
		 * get the regulator handle from voltage regulator framework
		 * and then try to set the voltage level for the regulator
		 */
		if (curr_vdd_reg) {
			rc = msm_sdcc_vreg_init_reg(curr_vdd_reg);
			if (rc)
				goto out;
		}
		if (curr_vccq_reg) {
			rc = msm_sdcc_vreg_init_reg(curr_vccq_reg);
			if (rc)
				goto vdd_reg_deinit;
		}
		if (curr_vddp_reg) {
			rc = msm_sdcc_vreg_init_reg(curr_vddp_reg);
			if (rc)
				goto vccq_reg_deinit;
		}
		goto out;
	} else
		/* deregister with all regulators from regulator framework */
		goto vddp_reg_deinit;

vddp_reg_deinit:
	if (curr_vddp_reg)
		msm_sdcc_vreg_deinit_reg(curr_vddp_reg);
vccq_reg_deinit:
	if (curr_vccq_reg)
		msm_sdcc_vreg_deinit_reg(curr_vccq_reg);
vdd_reg_deinit:
	if (curr_vdd_reg)
		msm_sdcc_vreg_deinit_reg(curr_vdd_reg);
out:
	return rc;
}

static int msm_sdcc_vreg_enable(struct sdcc_reg *vreg)
{
	int rc;

	if (!vreg->enabled) {
		rc = regulator_enable(vreg->reg);
		if (rc) {
			pr_err("%s: regulator_enable(%s) failed. rc=%d\n",
				__func__, vreg->reg_name, rc);
			goto out;
		}
		vreg->enabled = 1;
	}

	/* Put always_on regulator in HPM (high power mode) */
	if (vreg->always_on && vreg->op_pwr_mode_sup) {
		rc = regulator_set_optimum_mode(vreg->reg, vreg->hpm_uA);
		if (rc < 0) {
			pr_err("%s: reg=%s: HPM setting failed"
				" hpm_uA=%d, rc=%d\n",
				__func__, vreg->reg_name,
				vreg->hpm_uA, rc);
			goto vreg_disable;
		}
		rc = 0;
	}
	goto out;

vreg_disable:
	regulator_disable(vreg->reg);
	vreg->enabled = 0;
out:
	return rc;
}

static int msm_sdcc_vreg_disable(struct sdcc_reg *vreg)
{
	int rc;

	/* Never disable always_on regulator */
	if (!vreg->always_on) {
		rc = regulator_disable(vreg->reg);
		if (rc) {
			pr_err("%s: regulator_disable(%s) failed. rc=%d\n",
				__func__, vreg->reg_name, rc);
			goto out;
		}
		vreg->enabled = 0;
	}

	/* Put always_on regulator in LPM (low power mode) */
	if (vreg->always_on && vreg->op_pwr_mode_sup) {
		rc = regulator_set_optimum_mode(vreg->reg, vreg->lpm_uA);
		if (rc < 0) {
			pr_err("%s: reg=%s: LPM setting failed"
				" lpm_uA=%d, rc=%d\n",
				__func__,
				vreg->reg_name,
				vreg->lpm_uA, rc);
			goto out;
		}
		rc = 0;
	}

out:
	return rc;
}

static int msm_sdcc_setup_vreg(int dev_id, unsigned char enable)
{
	int rc = 0;
	struct sdcc_reg *curr_vdd_reg, *curr_vccq_reg, *curr_vddp_reg;
	struct sdcc_reg_data *curr;

	curr = &sdcc_vreg_data[dev_id - 1];
	curr_vdd_reg = curr->vdd_data;
	curr_vccq_reg = curr->vccq_data;
	curr_vddp_reg = curr->vddp_data;

	/* check if regulators are initialized or not? */
	if ((curr_vdd_reg && !curr_vdd_reg->reg) ||
		(curr_vccq_reg && !curr_vccq_reg->reg) ||
		(curr_vddp_reg && !curr_vddp_reg->reg)) {
		/* initialize voltage regulators required for this SDCC */
		rc = msm_sdcc_vreg_init(dev_id, 1);
		if (rc) {
			pr_err("%s: regulator init failed = %d\n",
				__func__, rc);
			goto out;
		}
	}

	if (curr->sts == enable)
		goto out;

	if (curr_vdd_reg) {
		if (enable)
			rc = msm_sdcc_vreg_enable(curr_vdd_reg);
		else
			rc = msm_sdcc_vreg_disable(curr_vdd_reg);
		if (rc)
			goto out;
	}

	if (curr_vccq_reg) {
		if (enable)
			rc = msm_sdcc_vreg_enable(curr_vccq_reg);
		else
			rc = msm_sdcc_vreg_disable(curr_vccq_reg);
		if (rc)
			goto out;
	}

	if (curr_vddp_reg) {
		if (enable)
			rc = msm_sdcc_vreg_enable(curr_vddp_reg);
		else
			rc = msm_sdcc_vreg_disable(curr_vddp_reg);
		if (rc)
			goto out;
	}
	curr->sts = enable;

out:
	return rc;
}

static u32 msm_sdcc_setup_power(struct device *dv, unsigned int vdd)
{
	u32 rc_pin_cfg = 0;
	u32 rc_vreg_cfg = 0;
	u32 rc = 0;
	struct platform_device *pdev;
	struct msm_sdcc_pin_cfg *curr_pin_cfg;

	pdev = container_of(dv, struct platform_device, dev);

	/* setup gpio/pad */
	curr_pin_cfg = &sdcc_pin_cfg_data[pdev->id - 1];
	if (curr_pin_cfg->cfg_sts == !!vdd)
		goto setup_vreg;

	if (curr_pin_cfg->is_gpio)
		rc_pin_cfg = msm_sdcc_setup_gpio(pdev->id, !!vdd);
	else
		rc_pin_cfg = msm_sdcc_setup_pad(pdev->id, !!vdd);

setup_vreg:
	/* setup voltage regulators */
	rc_vreg_cfg = msm_sdcc_setup_vreg(pdev->id, !!vdd);

	if (rc_pin_cfg || rc_vreg_cfg)
		rc = rc_pin_cfg ? rc_pin_cfg : rc_vreg_cfg;

	return rc;
}

static void msm_sdcc_sdio_lpm_gpio(struct device *dv, unsigned int active)
{
	struct msm_sdcc_pin_cfg *curr_pin_cfg;
	struct platform_device *pdev;

	pdev = container_of(dv, struct platform_device, dev);
	/* setup gpio/pad */
	curr_pin_cfg = &sdcc_pin_cfg_data[pdev->id - 1];

	if (curr_pin_cfg->cfg_sts == active)
		return;

	curr_pin_cfg->sdio_lpm_gpio_cfg = 1;
	if (curr_pin_cfg->is_gpio)
		msm_sdcc_setup_gpio(pdev->id, active);
	else
		msm_sdcc_setup_pad(pdev->id, active);
	curr_pin_cfg->sdio_lpm_gpio_cfg = 0;
}

#ifdef CONFIG_LGE_MMC_MSM_SDC3_WP_SUPPORT
static int msm_sdc3_get_wpswitch(struct device *dev)
{
	struct platform_device *pdev;
	int status;
	pdev = container_of(dev, struct platform_device, dev);

	status = gpio_request(GPIO_SDC_WP, "SD_WP_Switch");
	if (status) {
		pr_err("%s:Failed to request GPIO %d\n",
					__func__, GPIO_SDC_WP);
	} else {
		status = gpio_direction_input(GPIO_SDC_WP);
		if (!status) {
			status = gpio_get_value_cansleep(GPIO_SDC_WP);
			pr_info("%s: WP Status for Slot %d = %d\n",
				 __func__, pdev->id, status);
		}
		gpio_free(GPIO_SDC_WP);
	}
	return status;
}
#endif

#ifdef CONFIG_MMC_MSM_SDC5_SUPPORT
int sdc5_register_status_notify(void (*callback)(int, void *),
	void *dev_id)
{
	sdc5_status_notify_cb = callback;
	sdc5_status_notify_cb_devid = dev_id;
	return 0;
}
#endif

#ifdef CONFIG_MMC_MSM_SDC2_SUPPORT
int sdc2_register_status_notify(void (*callback)(int, void *),
	void *dev_id)
{
	sdc2_status_notify_cb = callback;
	sdc2_status_notify_cb_devid = dev_id;
	return 0;
}
#endif

/* Interrupt handler for SDC2 and SDC5 detection
 * This function uses dual-edge interrputs settings in order
 * to get SDIO detection when the GPIO is rising and SDIO removal
 * when the GPIO is falling */
static irqreturn_t msm8x60_multi_sdio_slot_status_irq(int irq, void *dev_id)
{
	int status;

	if (!machine_is_lge_i_board() && !machine_is_msm8x60_fusion() && !machine_is_lge_325_board() &&
	    !machine_is_msm8x60_fusn_ffa())
		return IRQ_NONE;

	status = gpio_get_value(MDM2AP_SYNC);
	pr_info("%s: MDM2AP_SYNC Status = %d\n",
		 __func__, status);

#ifdef CONFIG_MMC_MSM_SDC2_SUPPORT
	if (sdc2_status_notify_cb) {
		pr_info("%s: calling sdc2_status_notify_cb\n", __func__);
		sdc2_status_notify_cb(status,
			sdc2_status_notify_cb_devid);
	}
#endif

#ifdef CONFIG_MMC_MSM_SDC5_SUPPORT
	if (sdc5_status_notify_cb) {
		pr_info("%s: calling sdc5_status_notify_cb\n", __func__);
		sdc5_status_notify_cb(status,
			sdc5_status_notify_cb_devid);
	}
#endif
	return IRQ_HANDLED;
}

static int msm8x60_multi_sdio_init(void)
{
	int ret, irq_num;

	if (!machine_is_lge_i_board() && !machine_is_msm8x60_fusion() && !machine_is_lge_325_board() &&
	    !machine_is_msm8x60_fusn_ffa())
		return 0;

	ret = msm_gpiomux_get(MDM2AP_SYNC);
	if (ret) {
		pr_err("%s:Failed to request GPIO %d, ret=%d\n",
					__func__, MDM2AP_SYNC, ret);
		return ret;
	}

	irq_num = gpio_to_irq(MDM2AP_SYNC);

	ret = request_irq(irq_num,
		msm8x60_multi_sdio_slot_status_irq,
		IRQ_TYPE_EDGE_BOTH,
		"sdio_multidetection", NULL);

	if (ret) {
		pr_err("%s:Failed to request irq, ret=%d\n",
					__func__, ret);
		return ret;
	}

	return ret;
}

#ifdef CONFIG_MMC_MSM_SDC3_SUPPORT
#ifdef CONFIG_MMC_MSM_CARD_HW_DETECTION
static unsigned int msm8x60_sdcc_slot_status(struct device *dev)
{
	int status;

	status = gpio_request(PM8058_GPIO_PM_TO_SYS(PMIC_GPIO_SDC3_DET - 1)
				, "SD_HW_Detect");
	if (status) {
		pr_err("%s:Failed to request GPIO %d\n", __func__,
				PM8058_GPIO_PM_TO_SYS(PMIC_GPIO_SDC3_DET - 1));
	} else {
		status = gpio_direction_input(
				PM8058_GPIO_PM_TO_SYS(PMIC_GPIO_SDC3_DET - 1));
		if (!status){
			status = !(gpio_get_value_cansleep(
				PM8058_GPIO_PM_TO_SYS(PMIC_GPIO_SDC3_DET - 1)));
#ifdef CONFIG_LGE_PMIC8058_REGULATOR //platform-bsp@lge.com : turn off sd-card Vdd while SD-card absent.
			if(status)
				sdcc_vreg_data[2].vdd_data->always_on = 1;
			else
				sdcc_vreg_data[2].vdd_data->always_on = 0;
#endif
		}
		gpio_free(PM8058_GPIO_PM_TO_SYS(PMIC_GPIO_SDC3_DET - 1));
	}
	return (unsigned int) status;
}
#endif
#endif

#ifdef	CONFIG_MMC_MSM_SDC4_SUPPORT
#if defined(CONFIG_LGE_BCM432X_PATCH)
#else
static int msm_sdcc_cfg_mpm_sdiowakeup(struct device *dev, unsigned mode)
{
	struct platform_device *pdev;
	enum msm_mpm_pin pin;
	int ret = 0;

	pdev = container_of(dev, struct platform_device, dev);

	/* Only SDCC4 slot connected to WLAN chip has wakeup capability */
	if (pdev->id == 4)
		pin = MSM_MPM_PIN_SDC4_DAT1;
	else
		return -EINVAL;

	switch (mode) {
	case SDC_DAT1_DISABLE:
		ret = msm_mpm_enable_pin(pin, 0);
		break;
	case SDC_DAT1_ENABLE:
		ret = msm_mpm_set_pin_type(pin, IRQ_TYPE_LEVEL_LOW);
		ret = msm_mpm_enable_pin(pin, 1);
		break;
	case SDC_DAT1_ENWAKE:
		ret = msm_mpm_set_pin_wake(pin, 1);
		break;
	case SDC_DAT1_DISWAKE:
		ret = msm_mpm_set_pin_wake(pin, 0);
		break;
	default:
		ret = -EINVAL;
		break;
	}
	return ret;
}
#endif
#endif
#endif

#ifdef CONFIG_MMC_MSM_SDC1_SUPPORT
static struct mmc_platform_data msm8x60_sdc1_data = {
	.ocr_mask       = MMC_VDD_27_28 | MMC_VDD_28_29,
	.translate_vdd  = msm_sdcc_setup_power,
#ifdef CONFIG_MMC_MSM_SDC1_8_BIT_SUPPORT
	.mmc_bus_width  = MMC_CAP_8_BIT_DATA,
#else
	.mmc_bus_width  = MMC_CAP_4_BIT_DATA,
#endif
	.msmsdcc_fmin	= 400000,
	.msmsdcc_fmid	= 24000000,
	.msmsdcc_fmax	= 48000000,
	.nonremovable	= 1,
	.pclk_src_dfab	= 1,
};
#endif

#ifdef CONFIG_MMC_MSM_SDC2_SUPPORT
static struct mmc_platform_data msm8x60_sdc2_data = {
	.ocr_mask       = MMC_VDD_27_28 | MMC_VDD_28_29 | MMC_VDD_165_195,
	.translate_vdd  = msm_sdcc_setup_power,
	.sdio_lpm_gpio_setup = msm_sdcc_sdio_lpm_gpio,
	.mmc_bus_width  = MMC_CAP_8_BIT_DATA,
	.msmsdcc_fmin	= 400000,
	.msmsdcc_fmid	= 24000000,
	.msmsdcc_fmax	= 48000000,
	.nonremovable	= 0,
	.pclk_src_dfab  = 1,
	.register_status_notify = sdc2_register_status_notify,
#ifdef CONFIG_MSM_SDIO_AL
	.is_sdio_al_client = 1,
#endif
};
#endif

#ifdef CONFIG_MMC_MSM_SDC3_SUPPORT
static struct mmc_platform_data msm8x60_sdc3_data = {
	.ocr_mask       = MMC_VDD_27_28 | MMC_VDD_28_29,
	.translate_vdd  = msm_sdcc_setup_power,
	.mmc_bus_width  = MMC_CAP_4_BIT_DATA,
#ifdef CONFIG_LGE_MMC_MSM_SDC3_WP_SUPPORT
	.wpswitch  	= msm_sdc3_get_wpswitch,
#endif
#ifdef CONFIG_MMC_MSM_CARD_HW_DETECTION
	.status      = msm8x60_sdcc_slot_status,
	.status_irq  = PM8058_GPIO_IRQ(PM8058_IRQ_BASE,
				       PMIC_GPIO_SDC3_DET - 1),
	.irq_flags   = IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING,
#endif
	.msmsdcc_fmin	= 400000,
	.msmsdcc_fmid	= 24000000,
	.msmsdcc_fmax	= 48000000,
	.nonremovable	= 0,
	.pclk_src_dfab  = 1,
};
#endif

#ifdef CONFIG_MMC_MSM_SDC4_SUPPORT
#if defined(CONFIG_LGE_BCM432X_PATCH)
/* LGE_CHANGE_S, [yhcha @ 110303], <Add BCM4330> */
static unsigned int bcm432x_sdcc_wlan_slot_status(struct device *dev)
{
	printk(KERN_ERR "%s: %d %d\n", __func__, CONFIG_BCMDHD_GPIO_WL_RESET, gpio_get_value(CONFIG_BCMDHD_GPIO_WL_RESET));
    return gpio_get_value(CONFIG_BCMDHD_GPIO_WL_RESET);
}

static struct mmc_platform_data bcm432x_sdcc_wlan_data = {
    .ocr_mask   	= MMC_VDD_30_31,
    .translate_vdd	= msm_sdcc_setup_power,
    .mmc_bus_width	= MMC_CAP_4_BIT_DATA,
    .status     	= bcm432x_sdcc_wlan_slot_status,
    .status_irq		= MSM_GPIO_TO_INT(CONFIG_BCMDHD_GPIO_WL_RESET),
    .irq_flags		= IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING,
#ifdef CONFIG_MMC_MSM_SDC4_DUMMY52_REQUIRED
    .dummy52_required   = 1,
#endif
    .msmsdcc_fmin	= 400000,//144000,
    .msmsdcc_fmid	= 24000000,//24576000,
    .msmsdcc_fmax	= 48000000,//24576000, // 49152000,
//    .nonremovable	= 0,
	.nonremovable	= 1,
	.pclk_src_dfab  = 1,
};
/* LGE_CHANGE_E, [yhcha @ 110303] <Add BCM4330> */
#else /* qualcomm or google */
static struct mmc_platform_data msm8x60_sdc4_data = {
	.ocr_mask       = MMC_VDD_27_28 | MMC_VDD_28_29,
	.translate_vdd  = msm_sdcc_setup_power,
	.mmc_bus_width  = MMC_CAP_4_BIT_DATA,
	.msmsdcc_fmin	= 400000,
	.msmsdcc_fmid	= 24000000,
	.msmsdcc_fmax	= 48000000,
	.nonremovable	= 0,
	.pclk_src_dfab  = 1,
	.cfg_mpm_sdiowakeup = msm_sdcc_cfg_mpm_sdiowakeup,
#ifdef CONFIG_MMC_MSM_SDC4_DUMMY52_REQUIRED
	.dummy52_required = 1,
#endif
};
#endif /* CONFIG_LGE_BCM432X_PATCH */
#endif

#ifdef CONFIG_MMC_MSM_SDC5_SUPPORT
static struct mmc_platform_data msm8x60_sdc5_data = {
	.ocr_mask       = MMC_VDD_27_28 | MMC_VDD_28_29 | MMC_VDD_165_195,
	.translate_vdd  = msm_sdcc_setup_power,
	.sdio_lpm_gpio_setup = msm_sdcc_sdio_lpm_gpio,
	.mmc_bus_width  = MMC_CAP_4_BIT_DATA,
	.msmsdcc_fmin	= 400000,
	.msmsdcc_fmid	= 24000000,
	.msmsdcc_fmax	= 48000000,
	.nonremovable	= 0,
	.pclk_src_dfab  = 1,
	.register_status_notify = sdc5_register_status_notify,
#ifdef CONFIG_MSM_SDIO_AL
	.is_sdio_al_client = 1,
#endif
};
#endif

static void __init msm8x60_init_mmc(void)
{
#ifdef CONFIG_MMC_MSM_SDC1_SUPPORT
	/* SDCC1 : eMMC card connected */
	sdcc_vreg_data[0].vdd_data = &sdcc_vdd_reg_data[0];
	sdcc_vreg_data[0].vdd_data->reg_name = "8901_l5";
	sdcc_vreg_data[0].vdd_data->set_voltage_sup = 1;
	sdcc_vreg_data[0].vdd_data->level = 2850000;
	sdcc_vreg_data[0].vdd_data->always_on = 1;
	sdcc_vreg_data[0].vdd_data->op_pwr_mode_sup = 1;
	sdcc_vreg_data[0].vdd_data->lpm_uA = 9000;
	sdcc_vreg_data[0].vdd_data->hpm_uA = 200000;

	sdcc_vreg_data[0].vccq_data = &sdcc_vccq_reg_data[0];
	sdcc_vreg_data[0].vccq_data->reg_name = "8901_lvs0";
	sdcc_vreg_data[0].vccq_data->set_voltage_sup = 0;
	sdcc_vreg_data[0].vccq_data->always_on = 1;

	msm_add_sdcc(1, &msm8x60_sdc1_data);
#endif
#ifdef CONFIG_MMC_MSM_SDC2_SUPPORT
	/*
	 * MDM SDIO client is connected to SDC2 on charm SURF/FFA
	 * and no card is connected on 8660 SURF/FFA/FLUID.
	 */
	sdcc_vreg_data[1].vdd_data = &sdcc_vdd_reg_data[1];
	sdcc_vreg_data[1].vdd_data->reg_name = "8058_s3";
	sdcc_vreg_data[1].vdd_data->set_voltage_sup = 1;
	sdcc_vreg_data[1].vdd_data->level = 1800000;

	sdcc_vreg_data[1].vccq_data = NULL;

	if (machine_is_msm8x60_fusion())
		msm8x60_sdc2_data.msmsdcc_fmax = 24000000;
	if (machine_is_lge_i_board() || machine_is_msm8x60_fusion() || machine_is_msm8x60_fusn_ffa() || machine_is_lge_325_board()) {
#ifdef CONFIG_MMC_MSM_SDIO_SUPPORT
		msm8x60_sdc2_data.sdiowakeup_irq = gpio_to_irq(144);
		msm_sdcc_setup_gpio(2, 1);
#endif
		msm_add_sdcc(2, &msm8x60_sdc2_data);
	}
#endif
#ifdef CONFIG_MMC_MSM_SDC3_SUPPORT
	/* SDCC3 : External card slot connected */
	sdcc_vreg_data[2].vdd_data = &sdcc_vdd_reg_data[2];
	sdcc_vreg_data[2].vdd_data->reg_name = "8058_l14";
	sdcc_vreg_data[2].vdd_data->set_voltage_sup = 1;
#ifdef CONFIG_LGE_PMIC8058_REGULATOR //platform@lge.com : for SD-card VDD margin
	sdcc_vreg_data[2].vdd_data->level = 2950000;
#else
	sdcc_vreg_data[2].vdd_data->level = 2850000;
#endif
#ifdef CONFIG_LGE_PM_CURRENT_CONSUMPTION_FIX
/* kiwone.seo@lge.com sdcard error we always set  1*/
	sdcc_vreg_data[2].vdd_data->always_on = 1;
#else
	sdcc_vreg_data[2].vdd_data->always_on = 1;
#endif
	sdcc_vreg_data[2].vdd_data->op_pwr_mode_sup = 1;
	sdcc_vreg_data[2].vdd_data->lpm_uA = 9000;
	sdcc_vreg_data[2].vdd_data->hpm_uA = 200000;

	sdcc_vreg_data[2].vccq_data = NULL;

	sdcc_vreg_data[2].vddp_data = &sdcc_vddp_reg_data[2];
	sdcc_vreg_data[2].vddp_data->reg_name = "8058_l5";
	sdcc_vreg_data[2].vddp_data->set_voltage_sup = 1;
#ifdef CONFIG_LGE_PMIC8058_REGULATOR //platform@lge.com : for SD-card VDD margin
	sdcc_vreg_data[2].vddp_data->level = 2850000; //2950000;
#else
	sdcc_vreg_data[2].vddp_data->level = 2850000;
#endif
	sdcc_vreg_data[2].vddp_data->always_on = 1;
	sdcc_vreg_data[2].vddp_data->op_pwr_mode_sup = 1;
	/* Sleep current required is ~300 uA. But min. RPM
	 * vote can be in terms of mA (min. 1 mA).
	 * So let's vote for 2 mA during sleep.
	 */
	sdcc_vreg_data[2].vddp_data->lpm_uA = 2000;
	/* Max. Active current required is 16 mA */
	sdcc_vreg_data[2].vddp_data->hpm_uA = 16000;

	if (machine_is_msm8x60_fluid())
		msm8x60_sdc3_data.wpswitch = NULL;
	msm_add_sdcc(3, &msm8x60_sdc3_data);
#endif
#ifdef CONFIG_MMC_MSM_SDC4_SUPPORT
	/* SDCC4 : WLAN WCN1314 chip is connected */
	sdcc_vreg_data[3].vdd_data = &sdcc_vdd_reg_data[3];
	sdcc_vreg_data[3].vdd_data->reg_name = "8058_s3";
	sdcc_vreg_data[3].vdd_data->set_voltage_sup = 1;
	sdcc_vreg_data[3].vdd_data->level = 1800000;

	sdcc_vreg_data[3].vccq_data = NULL;

/* LGE_CHANGE_S, [gsm-wifi@lge.com], 2011-02-16, <Add BCM4330> */
#if defined(CONFIG_LGE_BCM432X_PATCH)
	/* GPIO config */
	gpio_tlmm_config(GPIO_CFG(CONFIG_BCMDHD_GPIO_WL_RESET, 0, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_2MA), GPIO_CFG_ENABLE);
	gpio_set_value(CONFIG_BCMDHD_GPIO_WL_RESET, 0);

	gpio_tlmm_config(GPIO_CFG(CONFIG_BCMDHD_GPIO_WL_HOSTWAKEUP, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_UP, GPIO_CFG_2MA), GPIO_CFG_ENABLE);

	/* Register platform device */
	msm_add_sdcc(4, &bcm432x_sdcc_wlan_data);

	msleep(200);
	/* Enable RESET IRQ for wlan card detect */
	enable_irq(gpio_to_irq(CONFIG_BCMDHD_GPIO_WL_RESET));

#else /* qualcomm or google */
	msm_add_sdcc(4, &msm8x60_sdc4_data);
#endif /* CONFIG_LGE_BCM432X_PATCH */
/* LGE_CHANGE_E, [gsm-wifi@lge.com], 2011-02-16, <Add BCM4330> */
#endif

#ifdef CONFIG_MMC_MSM_SDC5_SUPPORT
	/*
	 * MDM SDIO client is connected to SDC5 on charm SURF/FFA
	 * and no card is connected on 8660 SURF/FFA/FLUID.
	 */
	sdcc_vreg_data[4].vdd_data = &sdcc_vdd_reg_data[4];
	sdcc_vreg_data[4].vdd_data->reg_name = "8058_s3";
	sdcc_vreg_data[4].vdd_data->set_voltage_sup = 1;
	sdcc_vreg_data[4].vdd_data->level = 1800000;

	sdcc_vreg_data[4].vccq_data = NULL;

	if (machine_is_msm8x60_fusion())
		msm8x60_sdc5_data.msmsdcc_fmax = 24000000;
	if (machine_is_lge_i_board() || machine_is_msm8x60_fusion() || machine_is_msm8x60_fusn_ffa() ||  machine_is_lge_325_board()) {
#ifdef CONFIG_MMC_MSM_SDIO_SUPPORT
		msm8x60_sdc5_data.sdiowakeup_irq = gpio_to_irq(99);
		msm_sdcc_setup_gpio(5, 1);
#endif
		msm_add_sdcc(5, &msm8x60_sdc5_data);
	}
#endif
}
#ifdef CONFIG_MSM_RPM
static struct msm_rpm_platform_data msm_rpm_data = {
	.reg_base_addrs = {
		[MSM_RPM_PAGE_STATUS] = MSM_RPM_BASE,
		[MSM_RPM_PAGE_CTRL] = MSM_RPM_BASE + 0x400,
		[MSM_RPM_PAGE_REQ] = MSM_RPM_BASE + 0x600,
		[MSM_RPM_PAGE_ACK] = MSM_RPM_BASE + 0xa00,
	},

	.irq_ack = RPM_SCSS_CPU0_GP_HIGH_IRQ,
	.irq_err = RPM_SCSS_CPU0_GP_LOW_IRQ,
	.irq_vmpm = RPM_SCSS_CPU0_GP_MEDIUM_IRQ,
	.msm_apps_ipc_rpm_reg = MSM_GCC_BASE + 0x008,
	.msm_apps_ipc_rpm_val = 4,
};
#endif

void msm_fusion_setup_pinctrl(void)
{
	struct msm_xo_voter *a1;

	// byongdoo.oh@lge.com
	//	if (socinfo_get_platform_subtype() == 0x3) {
		/*
		 * Vote for the A1 clock to be in pin control mode before
		* the external images are loaded.
		*/
		a1 = msm_xo_get(MSM_XO_TCXO_A1, "mdm");
		BUG_ON(!a1);
		msm_xo_mode_vote(a1, MSM_XO_MODE_PIN_CTRL);
	//}
}

struct msm_board_data {
	struct msm_gpiomux_configs *gpiomux_cfgs;
};

static struct msm_board_data msm8x60_lge_325_board_data __initdata = {
	.gpiomux_cfgs = msm8x60_lge_325_gpiomux_cfgs,
};

//mipi shutdown
void *mipi_base;
void *mdp_video_base;
static void __init msm8x60_init(struct msm_board_data *board_data)
{
	uint32_t soc_platform_version;
#ifdef CONFIG_USB_EHCI_MSM_72K
	struct pm8xxx_mpp_config_data hsusb_phy_mpp = {
		.type		= PM8XXX_MPP_TYPE_D_OUTPUT,
		.level		= PM8901_MPP_DIG_LEVEL_L5,
		.control	= PM8XXX_MPP_DOUT_CTRL_HIGH,
	};
#endif
	pmic_reset_irq = PM8058_IRQ_BASE + PM8058_RESOUT_IRQ;

	/*
	 * Initialize RPM first as other drivers and devices may need
	 * it for their initialization.
	 */
#ifdef CONFIG_MSM_RPM
	BUG_ON(msm_rpm_init(&msm_rpm_data));
#endif
	BUG_ON(msm_rpmrs_levels_init(msm_rpmrs_levels,
				ARRAY_SIZE(msm_rpmrs_levels)));
	if (msm_xo_init())
		pr_err("Failed to initialize XO votes\n");

	msm8x60_check_2d_hardware();

	/* Change SPM handling of core 1 if PMM 8160 is present. */
	soc_platform_version = socinfo_get_platform_version();
	if (SOCINFO_VERSION_MAJOR(soc_platform_version) == 1 &&
			SOCINFO_VERSION_MINOR(soc_platform_version) >= 2) {
		struct msm_spm_platform_data *spm_data;

		spm_data = &msm_spm_data_v1[1];
		spm_data->reg_init_values[MSM_SPM_REG_SAW_CFG] &= ~0x0F00UL;
		spm_data->reg_init_values[MSM_SPM_REG_SAW_CFG] |= 0x0100UL;

		spm_data = &msm_spm_data[1];
		spm_data->reg_init_values[MSM_SPM_REG_SAW_CFG] &= ~0x0F00UL;
		spm_data->reg_init_values[MSM_SPM_REG_SAW_CFG] |= 0x0100UL;
	}

	/*
	 * Initialize SPM before acpuclock as the latter calls into SPM
	 * driver to set ACPU voltages.
	 */
	if (SOCINFO_VERSION_MAJOR(socinfo_get_version()) != 1)
		msm_spm_init(msm_spm_data, ARRAY_SIZE(msm_spm_data));
	else
		msm_spm_init(msm_spm_data_v1, ARRAY_SIZE(msm_spm_data_v1));

	/*
	 * Set regulators 8901_l4 and 8901_l6 to be always on in HPM for SURF
	 * devices so that the RPM doesn't drop into a low power mode that an
	 * un-reworked SURF cannot resume from.
	 */
	if (machine_is_msm8x60_surf()) {
		int i;

		for (i = 0; i < ARRAY_SIZE(rpm_regulator_init_data); i++)
			if (rpm_regulator_init_data[i].id
				== RPM_VREG_ID_PM8901_L4
			    || rpm_regulator_init_data[i].id
				== RPM_VREG_ID_PM8901_L6)
				rpm_regulator_init_data[i]
					.init_data.constraints.always_on = 1;
	}

	/*
	 * Disable regulator info printing so that regulator registration
	 * messages do not enter the kmsg log.
	 */
	regulator_suppress_info_printing();

	/* Initialize regulators needed for clock_init. */
	platform_add_devices(early_regulators, ARRAY_SIZE(early_regulators));

	//backlight off before mipi shutdown
	gpio_tlmm_config(GPIO_CFG(49, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),GPIO_CFG_ENABLE);
	gpio_set_value(49, 0);
	//mipi shutdown
	mipi_base = ioremap(0x4700000, 0x1000);
	mdp_video_base = ioremap(0x51e0000, 0x1000);

	writel(0x00000000, mdp_video_base);
	mdelay(60);
	writel(0x01010101, (mipi_base+0x10C));
	writel(0x13FF3BFF, (mipi_base+0x108));
	writel(0, (mipi_base+0x200));
	writel(0, (mipi_base+0x118));
	writel(0, (mipi_base+0x000));
	iounmap(mipi_base);
	iounmap(mdp_video_base);
	msm_clock_init(&msm8x60_clock_init_data);

	/* Buses need to be initialized before early-device registration
	 * to get the platform data for fabrics.
	 */
	msm8x60_init_buses();
	platform_add_devices(early_devices, ARRAY_SIZE(early_devices));
	/* CPU frequency control is not supported on simulated targets. */
	if (!machine_is_msm8x60_rumi3() && !machine_is_msm8x60_sim())
		acpuclk_init(&acpuclk_8x60_soc_data);

	/*
	 * Enable EBI2 only for boards which make use of it. Leave
	 * it disabled for all others for additional power savings.
	 */
	if (machine_is_msm8x60_surf() || machine_is_msm8x60_ffa() ||
			machine_is_msm8x60_rumi3() ||
			machine_is_msm8x60_sim() ||
			machine_is_msm8x60_fluid() ||
			machine_is_msm8x60_dragon())
		msm8x60_init_ebi2();
	msm8x60_init_tlmm();
	msm8x60_init_gpiomux(board_data->gpiomux_cfgs);
	msm8x60_init_uart12dm();
#ifdef CONFIG_MSM_CAMERA_V4L2
	msm8x60_init_cam();
#endif
	msm8x60_init_mmc();

	//[LG_BTUI] To seperate bt power driver module form All in One board configuration
   printk(KERN_ERR "[LG_BTUI] BTINIT------------->%s: \n", __func__);
   lge_add_btpower_devices();
	//[LG_BTUI] minwoo2.kim@lge.com

#ifdef CONFIG_LGE_SENSOR
	sensor_power_init();
#endif

#if defined(CONFIG_PMIC8058_OTHC) || defined(CONFIG_PMIC8058_OTHC_MODULE)
#ifndef CONFIG_LGE_HEADSET_DETECTION_FSA8008
	msm8x60_init_pm8058_othc();
#endif
#endif

#ifdef CONFIG_LGE_PMIC8058_KEYPAD
	pm8058_platform_data.keypad_pdata = &batman_keypad_data;
#endif
#if defined(CONFIG_MACH_LGE_325_BOARD_SKT) || defined(CONFIG_MACH_LGE_325_BOARD_LGU)
	if(lge_bd_rev > LGE_REV_D)
		pm8058_platform_data.leds_pdata = &pm8058_flash_leds_data;
	else
		pm8058_platform_data.leds_pdata =&pm8058_flash_leds_data_for_old_HW;
#else
	/* configure pmic leds */
	pm8058_platform_data.leds_pdata = &pm8058_flash_leds_data;
#endif

	if (machine_is_msm8x60_surf() || machine_is_msm8x60_ffa() ||
	    machine_is_msm8x60_fluid() || machine_is_lge_i_board() || machine_is_msm8x60_fusion() ||
	    machine_is_msm8x60_fusn_ffa() || machine_is_msm8x60_dragon() || machine_is_lge_325_board()) {
		if (SOCINFO_VERSION_MAJOR(socinfo_get_version()) != 1)
			platform_add_devices(msm_footswitch_devices,
					     msm_num_footswitch_devices);
		platform_add_devices(surf_devices,
				     ARRAY_SIZE(surf_devices));

#ifdef CONFIG_LGE_PMIC8901_REGULATOR
/* we don't use */
#else
	pm8901_vreg_mpp0_init();

	platform_device_register(&msm8x60_8901_mpp_vreg);
#endif

#ifdef CONFIG_ANDROID_RAM_CONSOLE
    lge_add_ramconsole_devices();
#endif

#ifdef CONFIG_LGE_HANDLE_PANIC
    lge_add_panic_handler_devices();
#endif
#ifdef CONFIG_LGE_BROADCAST_DCM
	spi_register_board_info(lge_bcast_msm_spi_board_info, ARRAY_SIZE(lge_bcast_msm_spi_board_info));
#endif
//2011-05-05 ella.hwang added for 1Seg Driver SPI porting - [Ends]

#ifdef CONFIG_LGE_BROADCAST_TDMB
	spi_register_board_info(tdmb_spi_board_info, ARRAY_SIZE(tdmb_spi_board_info));
#endif

#ifdef CONFIG_USB_EHCI_MSM_72K
	/*
	 * Drive MPP2 pin HIGH for PHY to generate ID interrupts on 8660
	 * fluid
	 */
	if (machine_is_msm8x60_fluid())
		pm8xxx_mpp_config(PM8901_MPP_PM_TO_SYS(1), &hsusb_phy_mpp);
	msm_add_host(0, &msm_usb_host_pdata);
#endif

		platform_add_devices(asoc_devices,
				ARRAY_SIZE(asoc_devices));
	}

	/* neo.kang@lge.com */
	msm_panel_init();

	/* jisun.shin@lge.com separate camera devices */
#ifdef CONFIG_LGE_CAMERA
	lge_camera_init();
#endif

#ifdef CONFIG_LGE_MHL_SII9244
   platform_device_register(&mhl_sii9244_device);
#endif

#ifdef CONFIG_BATTERY_325
	platform_device_register(&b325_battery_device);
#endif


#ifdef CONFIG_BATTERY_MSM8X60
	if (machine_is_msm8x60_surf() || machine_is_msm8x60_ffa() ||
		machine_is_lge_i_board() || machine_is_msm8x60_fusion() || machine_is_msm8x60_dragon() ||
		machine_is_msm8x60_fusn_ffa() || machine_is_msm8x60_fluid() || machine_is_lge_325_board())
		platform_device_register(&msm_charger_device);
#endif

	if (machine_is_lge_i_board() || machine_is_msm8x60_fusion()|| machine_is_msm8x60_fusn_ffa() || machine_is_lge_325_board())
		platform_add_devices(charm_devices, ARRAY_SIZE(charm_devices));

#if 1 /* platform-bsp@lge.com : pmic8058 charger */
	if (!machine_is_msm8x60_fluid())
                pm8058_platform_data.charger_pdata
                        = &pm8058_charger_pdata;
#endif

#if defined(CONFIG_SPI_QUP) || defined(CONFIG_SPI_QUP_MODULE)
	platform_device_register(&msm_gsbi1_qup_spi_device);
#endif

	if (!machine_is_msm8x60_sim())
		msm_fb_add_devices();

	fixup_i2c_configs();
	register_i2c_devices();

	msm_pm_set_platform_data(msm_pm_data, ARRAY_SIZE(msm_pm_data));
	msm_pm_set_rpm_wakeup_irq(RPM_SCSS_CPU0_WAKE_UP_IRQ);
	msm_cpuidle_set_states(msm_cstates, ARRAY_SIZE(msm_cstates),
				msm_pm_data);
	BUG_ON(msm_pm_boot_init(&msm_pm_boot_pdata));

	pm8058_gpios_init();

#ifdef CONFIG_SENSORS_MSM_ADC
	msm_adc_pdata.target_hw = MSM_8x60;
#endif
#ifdef CONFIG_MSM8X60_AUDIO
	msm_snddev_init();
#endif

	lge_add_misc_devices();

	msm8x60_multi_sdio_init();

	if (machine_is_msm8x60_fusion() || machine_is_msm8x60_fusn_ffa() || machine_is_lge_i_board() || machine_is_lge_325_board())
		msm_fusion_setup_pinctrl();
}

static void __init msm8x60_lge_325_init(void)
{
	msm8x60_init(&msm8x60_lge_325_board_data);
}

static void __init msm8x60_lge_325_init_early(void)
{
	msm8x60_allocate_memory_regions();
}

MACHINE_START(LGE_325_BOARD, "LGE LGE BOARD MSM8X60")
	.map_io = msm8x60_map_io,
	.reserve = msm8x60_reserve,
	.init_irq = msm8x60_init_irq,
	.handle_irq = gic_handle_irq,
	.init_machine = msm8x60_lge_325_init,
	.timer = &msm_timer,
	.init_early = msm8x60_lge_325_init_early,
MACHINE_END

