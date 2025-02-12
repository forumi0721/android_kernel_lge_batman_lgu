/* lge/lge_board/batman/platform_lge_325_panel.c
 *
 * Copyright (C) 2010 LGE, Inc.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 *
 */

#include <linux/kernel.h>
#include <linux/platform_device.h>
#include <linux/gpio.h>
#include <linux/delay.h>
#include <linux/debugfs.h>
#include <linux/mfd/pmic8901.h>
#include <linux/regulator/pmic8901-regulator.h>
#include <linux/regulator/consumer.h>
#include <linux/regulator/machine.h>
#include <linux/bootmem.h>
#include <asm/io.h>
#include <mach/msm_memtypes.h>
#include <mach/board.h>
#include <mach/msm_bus_board.h>

#include <linux/i2c.h>
#include <linux/ion.h>
#include <mach/ion.h>
#include "devices_lge_325.h"
#include "board_lge_325.h"

#ifdef CONFIG_FB_MSM_LCDC_DSUB
/* VGA = 1440 x 900 x 4(bpp) x 2(pages)
   prim = 1024 x 600 x 4(bpp) x 2(pages)
   This is the difference. */
#define MSM_FB_DSUB_PMEM_ADDER (0xA32000-0x4B0000)
#else
#define MSM_FB_DSUB_PMEM_ADDER (0)
#endif

#ifdef CONFIG_FB_MSM_TRIPLE_BUFFER
/* prim = 1024 x 600 x 4(bpp) x 3(pages) */
//#define MSM_FB_PRIM_BUF_SIZE 0x708000
/* prim = 1280 x 736 x 4(bpp) x 3(pages) */
#define MSM_FB_PRIM_BUF_SIZE (1024 * 768 * 4 * 3)
#else
/* prim = 1024 x 600 x 4(bpp) x 2(pages) */
//#define MSM_FB_PRIM_BUF_SIZE 0x4B0000
/* prim = 1280 x 736 x 4(bpp) x 2(pages) */
#define MSM_FB_PRIM_BUF_SIZE (1024 * 768 * 4 * 2)
#endif

#ifdef CONFIG_FB_MSM_HDMI_MSM_PANEL
#define MSM_FB_EXT_BUF_SIZE  (1920 * 1080 * 2 * 1) /* 2 bpp x 1 page */
#elif defined(CONFIG_FB_MSM_TVOUT)
#define MSM_FB_EXT_BUF_SIZE  (720 * 576 * 2 * 2) /* 2 bpp x 2 pages */
#else
#define MSM_FB_EXT_BUFT_SIZE	0
#endif

#define MSM_FB_EXT_BUF_CAPTION_SIZE (1920 * 1080 * 4 * 2)

/* Note: must be multiple of 4096 */
// add fb1 size for blending caption(msm8660)
#define MSM_FB_SIZE roundup(MSM_FB_PRIM_BUF_SIZE + MSM_FB_EXT_BUF_SIZE + \
				MSM_FB_EXT_BUF_CAPTION_SIZE + MSM_FB_DSUB_PMEM_ADDER, 4096)

#define MSM_PMEM_SF_SIZE 0x4000000 /* 64 Mbytes */
#define MSM_HDMI_PRIM_PMEM_SF_SIZE 0x4000000 /* 64 Mbytes */

extern unsigned char hdmi_is_primary;
#if 0
#ifdef CONFIG_FB_MSM_HDMI_AS_PRIMARY
unsigned char hdmi_is_primary = 1;
#else
unsigned char hdmi_is_primary;
#endif
#endif

#ifdef CONFIG_FB_MSM_OVERLAY0_WRITEBACK
#define MSM_FB_OVERLAY0_WRITEBACK_SIZE roundup((1024 * 768 * 3 * 2), 4096)
#else
#define MSM_FB_OVERLAY0_WRITEBACK_SIZE (0)
#endif  /* CONFIG_FB_MSM_OVERLAY0_WRITEBACK */

#ifdef CONFIG_FB_MSM_OVERLAY1_WRITEBACK
#define MSM_FB_OVERLAY1_WRITEBACK_SIZE roundup((1920 * 1088 * 3 * 2), 4096)
#else
#define MSM_FB_OVERLAY1_WRITEBACK_SIZE (0)
#endif  /* CONFIG_FB_MSM_OVERLAY1_WRITEBACK */

#define MIPI_VIDEO_HITACHI_PANEL_NAME		"mipi_video_hitachi_wvga"
#define HDMI_PANEL_NAME	"hdmi_msm"
#define TVOUT_PANEL_NAME	"tvout_msm"


static struct resource msm_fb_resources[] = {
	{
		.flags  = IORESOURCE_DMA,
	}
};

static int msm_fb_detect_panel(const char *name)
{
	if (!strncmp(name, MIPI_VIDEO_HITACHI_PANEL_NAME,
			strnlen(MIPI_VIDEO_HITACHI_PANEL_NAME,
				PANEL_NAME_MAX_LEN)))
		return 0;

	if (!strncmp(name, HDMI_PANEL_NAME,
			strnlen(HDMI_PANEL_NAME,
				PANEL_NAME_MAX_LEN)))
		return 0;

	if (!strncmp(name, TVOUT_PANEL_NAME,
			strnlen(TVOUT_PANEL_NAME,
				PANEL_NAME_MAX_LEN)))
		return 0;

	pr_warning("%s: not supported '%s'", __func__, name);
	return -ENODEV;
}

static struct msm_fb_platform_data msm_fb_pdata = {
	.detect_client = msm_fb_detect_panel,
};

static struct platform_device msm_fb_device = {
	.name   = "msm_fb",
	.id     = 0,
	.num_resources     = ARRAY_SIZE(msm_fb_resources),
	.resource          = msm_fb_resources,
	.dev.platform_data = &msm_fb_pdata,  
};

#ifdef CONFIG_FB_MSM_HDMI_MSM_PANEL
static struct resource hdmi_msm_resources[] = {
	{
		.name  = "hdmi_msm_qfprom_addr",
		.start = 0x00700000,
		.end   = 0x007060FF,
		.flags = IORESOURCE_MEM,
	},
	{
		.name  = "hdmi_msm_hdmi_addr",
		.start = 0x04A00000,
		.end   = 0x04A00FFF,
		.flags = IORESOURCE_MEM,
	},
	{
		.name  = "hdmi_msm_irq",
		.start = HDMI_IRQ,
		.end   = HDMI_IRQ,
		.flags = IORESOURCE_IRQ,
	},
};

static int hdmi_enable_5v(int on);
static int hdmi_core_power(int on, int show);
static int hdmi_cec_power(int on);

static struct msm_hdmi_platform_data hdmi_msm_data = {
	.irq = HDMI_IRQ,
	.enable_5v = hdmi_enable_5v,
	.core_power = hdmi_core_power,
	.cec_power = hdmi_cec_power,
};

static struct platform_device hdmi_msm_device = {
	.name = "hdmi_msm",
	.id = 0,
	.num_resources = ARRAY_SIZE(hdmi_msm_resources),
	.resource = hdmi_msm_resources,
	.dev.platform_data = &hdmi_msm_data,
};
#endif /* CONFIG_FB_MSM_HDMI_MSM_PANEL */

#ifdef CONFIG_FB_MSM_MIPI_DSI

/* minjong.gong@lge.com, 2011.01.31 - lgit lcd for MWC board*/
#define LCD_RESET_N		50
/* minjong.gong@lge.com, 2011.01.31 - lgit lcd for MWC board*/

static void mipi_config_gpio(int on)
{
	if(on)
	{
		/* Sub lcd  reset */
		gpio_tlmm_config(LCD_RESET_N,0);
		gpio_direction_output(LCD_RESET_N,1);
	}
}

extern void lm3533_lcd_backlight_set_level( int level);
static int mipi_hitachi_backlight_level(int level, int max, int min)
{
	lm3533_lcd_backlight_set_level(level);

	return 0;
}

static struct msm_panel_common_pdata mipi_hitachi_pdata = {
	.backlight_level = mipi_hitachi_backlight_level,
	.panel_config_gpio = mipi_config_gpio,
};

static struct platform_device mipi_dsi_hitachi_panel_device = {
	.name = "mipi_hitachi",
	.id = 0,
	.dev = {
		.platform_data = &mipi_hitachi_pdata,
	}
};

#ifndef CONFIG_LGE_DISPLAY_MIPI_HITACHI_VIDEO_HD_PT
/* minjong.gong@lge.com, 2011.01.31 - lgit lcd for MWC board*/
#define MDP_VSYNC_GPIO 28
#endif

/*
 * MIPI_DSI only use 8058_LDO0 which need always on
 * therefore it need to be put at low power mode if
 * it was not used instead of turn it off.
 */
static struct regulator* reg_8901_l2 = NULL;
static struct regulator* reg_8901_l3 = NULL;
static struct regulator* reg_8901_mvs = NULL;
static int mipi_dsi_panel_power(int on)
{
	int flag_on = !!on;
	static int mipi_dsi_power_save_on;
	int rc = 0;

	if (mipi_dsi_power_save_on == flag_on)
		return 0;

	mipi_dsi_power_save_on = flag_on;

	if (reg_8901_l2 == NULL) {
		reg_8901_l2 = regulator_get(NULL, "8901_l2");
		if (IS_ERR(reg_8901_l2)) {
			reg_8901_l2 = NULL;
		}
	}	
	if (reg_8901_mvs == NULL) {
		reg_8901_mvs = regulator_get(NULL, "8901_mvs0");
		if (IS_ERR(reg_8901_mvs)) {
			reg_8901_mvs = NULL;
		}
	}
	if (reg_8901_l3 == NULL) {
		reg_8901_l3 = regulator_get(NULL, "8901_l3");
		if (IS_ERR(reg_8901_l3)) {
			reg_8901_l3 = NULL;
		}
	}

	if(on){
		rc = regulator_enable(reg_8901_mvs); // +1V8_LCD_IO
		if (rc) {
			pr_err("'%s' regulator enable failed, rc=%d\n",
					"8901_mvs", rc);
			return rc;
		}
		mdelay(2);

		rc = regulator_set_voltage(reg_8901_l2, 3000000, 3000000);
		if (!rc)
			rc = regulator_enable(reg_8901_l2);
		if (rc) {
			pr_err("'%s' regulator enable failed, rc=%d\n",
					"8901_l2", rc);
			return rc;
		}
		udelay(100); // 100us

		rc = regulator_set_voltage(reg_8901_l3, 3000000, 3000000); // +3V0_LCD_VCI
		if (!rc)
			rc = regulator_enable(reg_8901_l3);
		if (rc) {
			pr_err("'%s' regulator enable failed, rc=%d\n",
					"8901_l3", rc);
			return rc;
		}
		udelay(100); // 100us
	}
	else{

		rc = regulator_disable(reg_8901_l3);
		if (rc)
			pr_warning("'%s' regulator disable failed, rc=%d\n",
					"8901_l3", rc);
		udelay(100); // 100us		
		rc = regulator_disable(reg_8901_l2);
		if (rc)
			pr_warning("'%s' regulator disable failed, rc=%d\n",
					"8901_12", rc);
		mdelay(3);
		rc = regulator_disable(reg_8901_mvs);
		if (rc)
			pr_warning("'%s' regulator disable failed, rc=%d\n",
					"8901_mvs", rc);
		udelay(100); // 100us
		pr_info("%s(off): success\n", __func__);

	}
	return 0;
}
#endif
struct mipi_dsi_platform_data mipi_dsi_pdata = {
    //.vsync_gpio = MDP_VSYNC_GPIO,
	.dsi_power_save   = mipi_dsi_panel_power,
};



void __init msm8x60_allocate_memory_regions(void)
{
	void *addr;
	unsigned long size;

	if (hdmi_is_primary)
		size = roundup((1920 * 1088 * 4 * 2), 4096);
	else
		size = MSM_FB_SIZE;

	addr = alloc_bootmem_align(size, 0x1000);
	msm_fb_resources[0].start = __pa(addr);
	msm_fb_resources[0].end = msm_fb_resources[0].start + size - 1;
	pr_info("allocating %lu bytes at %p (%lx physical) for fb\n",
		size, addr, __pa(addr));

}

void __init msm8x60_set_display_params(char *prim_panel, char *ext_panel)
{
	if (strnlen(prim_panel, PANEL_NAME_MAX_LEN)) {
		strlcpy(msm_fb_pdata.prim_panel_name, prim_panel,
			PANEL_NAME_MAX_LEN);
		pr_debug("msm_fb_pdata.prim_panel_name %s\n",
			msm_fb_pdata.prim_panel_name);

		if (!strncmp((char *)msm_fb_pdata.prim_panel_name,
			HDMI_PANEL_NAME, strnlen(HDMI_PANEL_NAME,
				PANEL_NAME_MAX_LEN))) {
			pr_debug("HDMI is the primary display by"
				" boot parameter\n");
			hdmi_is_primary = 1;
		}
	}
	if (strnlen(ext_panel, PANEL_NAME_MAX_LEN)) {
		strlcpy(msm_fb_pdata.ext_panel_name, ext_panel,
			PANEL_NAME_MAX_LEN);
		pr_debug("msm_fb_pdata.ext_panel_name %s\n",
			msm_fb_pdata.ext_panel_name);
	}
}

static struct platform_device *panel_devices[] __initdata = {
	&msm_fb_device,

#ifdef CONFIG_FB_MSM_MIPI_DSI

/* jaeseong.gim@lge.com. 2011-01-16 */
#if CONFIG_LGE_DISPLAY_MIPI_HITACHI_VIDEO_HD_PT
	&mipi_dsi_hitachi_panel_device,
#endif /*CONFIG_LGE_DISPLAY_MIPI_HITACHI_VIDEO_HD_PT */
#endif /*CONFIG_FB_MSM_MIPI_DSI */
#ifdef CONFIG_FB_MSM_HDMI_MSM_PANEL
	&hdmi_msm_device,
#endif /* CONFIG_FB_MSM_HDMI_MSM_PANEL */

};

void __init msm_panel_init(void){
	platform_add_devices(panel_devices, ARRAY_SIZE(panel_devices));
}
//start, linear mode, shoogi.lee@lge.com, 2011_04_20
struct backlight_platform_data {
   void (*platform_init)(void);
   int gpio;
   unsigned int mode;
   int max_current;
   int init_on_boot;
   int min_brightness;
   int default_brightness;
   int max_brightness;   
   int dimming_brightness;
};

static struct backlight_platform_data lm3533_data = {
	.gpio = 49,
	.max_current = 0x17,

	.min_brightness = 0x14,
	.default_brightness = 0x71,
	.max_brightness = 0xFF,//0x71,
	.dimming_brightness = 0x07, //?? this value should be changed by shoogi.
};
	
struct i2c_registry {
	u8                     machs;
	int                    bus;
	struct i2c_board_info *info;
	int                    len;
};

#define LM3533_BACKLIGHT_ADDRESS 0x36
static struct i2c_board_info msm_i2c_backlight_info[] = {
	{
		I2C_BOARD_INFO("lm3533", LM3533_BACKLIGHT_ADDRESS),
		.platform_data = &lm3533_data,
	}
};

static struct i2c_registry backlight_device __initdata = {
	0,
	MSM_GSBI3_QUP_I2C_BUS_ID,
	msm_i2c_backlight_info,
	ARRAY_SIZE(msm_i2c_backlight_info),
};

void __init i2c_register_backlight_info(void){
	i2c_register_board_info(backlight_device.bus,
							backlight_device.info,
							backlight_device.len);
}


#if !defined(CONFIG_GPIO_SX150X) && !defined(CONFIG_GPIO_SX150X_MODULE)
static inline void display_common_power(int on) {}
#endif

static int mipi_dsi_panel_power(int on);

#ifdef CONFIG_FB_MSM_HDMI_MSM_PANEL
#define _GET_REGULATOR(var, name) do {				\
	var = regulator_get(NULL, name);			\
	if (IS_ERR(var)) {					\
		pr_err("'%s' regulator not found, rc=%ld\n",	\
			name, IS_ERR(var));			\
		var = NULL;					\
		return -ENODEV;					\
	}							\
} while (0)

static int hdmi_enable_5v(int on)
{
#ifndef CONFIG_MACH_LGE_325_BOARD
	static struct regulator *reg_8901_hdmi_mvs;	/* HDMI_5V */
	static struct regulator *reg_8901_mpp0;		/* External 5V */
	static int prev_on;
	int rc;

	if (on == prev_on)
		return 0;

	if (!reg_8901_hdmi_mvs)
		_GET_REGULATOR(reg_8901_hdmi_mvs, "8901_hdmi_mvs");
	if (!reg_8901_mpp0)
		_GET_REGULATOR(reg_8901_mpp0, "8901_mpp0");

	if (on) {
		rc = regulator_enable(reg_8901_mpp0);
		if (rc) {
			pr_err("'%s' regulator enable failed, rc=%d\n",
				"reg_8901_mpp0", rc);
			return rc;
		}
		rc = regulator_enable(reg_8901_hdmi_mvs);
		if (rc) {
			pr_err("'%s' regulator enable failed, rc=%d\n",
				"8901_hdmi_mvs", rc);
			return rc;
		}
		pr_info("%s(on): success\n", __func__);
	} else {
		rc = regulator_disable(reg_8901_hdmi_mvs);
		if (rc)
			pr_warning("'%s' regulator disable failed, rc=%d\n",
				"8901_hdmi_mvs", rc);
		rc = regulator_disable(reg_8901_mpp0);
		if (rc)
			pr_warning("'%s' regulator disable failed, rc=%d\n",
				"reg_8901_mpp0", rc);
		pr_info("%s(off): success\n", __func__);
	}
	prev_on = on;
#endif //CONFIG_MACH_LGE_325_BOARD
	return 0;
}

static int hdmi_core_power(int on, int show)
{
#ifndef CONFIG_MACH_LGE_325_BOARD
	static struct regulator *reg_8058_l16;		/* VDD_HDMI */
#endif //CONFIG_MACH_LGE_325_BOARD
	static int prev_on;
	int rc;

	if (on == prev_on)
		return 0;
	
#ifndef CONFIG_MACH_LGE_325_BOARD
	if (!reg_8058_l16)
		_GET_REGULATOR(reg_8058_l16, "8058_l16");
#endif //CONFIG_MACH_LGE_325_BOARD

	if (on) {
#ifndef CONFIG_MACH_LGE_325_BOARD
		rc = regulator_set_voltage(reg_8058_l16, 1800000, 1800000);
		if (!rc)
			rc = regulator_enable(reg_8058_l16);
		if (rc) {
			pr_err("'%s' regulator enable failed, rc=%d\n",
				"8058_l16", rc);
			return rc;
		}
#endif //CONFIG_MACH_LGE_325_BOARD
		rc = gpio_request(170, "HDMI_DDC_CLK");
		if (rc) {
			pr_err("'%s'(%d) gpio_request failed, rc=%d\n",
				"HDMI_DDC_CLK", 170, rc);
			goto error1;
		}
		rc = gpio_request(171, "HDMI_DDC_DATA");
		if (rc) {
			pr_err("'%s'(%d) gpio_request failed, rc=%d\n",
				"HDMI_DDC_DATA", 171, rc);
			goto error2;
		}
		rc = gpio_request(172, "HDMI_HPD");
		if (rc) {
			pr_err("'%s'(%d) gpio_request failed, rc=%d\n",
				"HDMI_HPD", 172, rc);
			goto error3;
		}
		pr_info("%s(on): success\n", __func__);
	} else {
		gpio_free(170);
		gpio_free(171);
		gpio_free(172);

#ifndef CONFIG_MACH_LGE_325_BOARD		
		rc = regulator_disable(reg_8058_l16);
		if (rc)
			pr_warning("'%s' regulator disable failed, rc=%d\n",
				"8058_l16", rc);
#endif //CONFIG_MACH_LGE_325_BOARD
		pr_info("%s(off): success\n", __func__);
	}

	prev_on = on;

	return 0;

error3:
	gpio_free(171);
error2:
	gpio_free(170);
error1:
#ifndef CONFIG_MACH_LGE_325_BOARD	
	regulator_disable(reg_8058_l16);
#endif //CONFIG_MACH_LGE_325_BOARD
	return rc;
}

static int hdmi_cec_power(int on)
{	
#ifndef CONFIG_MACH_LGE_325_BOARD
	static struct regulator *reg_8901_l3;		/* HDMI_CEC */
	static int prev_on;
	int rc;

	if (on == prev_on)
		return 0;

	if (!reg_8901_l3)
		_GET_REGULATOR(reg_8901_l3, "8901_l3");

	if (on) {
		rc = regulator_set_voltage(reg_8901_l3, 3300000, 3300000);
		if (!rc)
			rc = regulator_enable(reg_8901_l3);
		if (rc) {
			pr_err("'%s' regulator enable failed, rc=%d\n",
				"8901_l3", rc);
			return rc;
		}
		rc = gpio_request(169, "HDMI_CEC_VAR");
		if (rc) {
			pr_err("'%s'(%d) gpio_request failed, rc=%d\n",
				"HDMI_CEC_VAR", 169, rc);
			goto error;
		}
		pr_info("%s(on): success\n", __func__);
	} else {
		gpio_free(169);
		rc = regulator_disable(reg_8901_l3);
		if (rc)
			pr_warning("'%s' regulator disable failed, rc=%d\n",
				"8901_l3", rc);
		pr_info("%s(off): success\n", __func__);
	}

	prev_on = on;
	
	return 0;
error:
	regulator_disable(reg_8901_l3);
	return rc;
#endif //CONFIG_MACH_LGE_325_BOARD

	return 0;
}

#undef _GET_REGULATOR

#endif /* CONFIG_FB_MSM_HDMI_MSM_PANEL */

#ifdef CONFIG_MSM_BUS_SCALING

static struct msm_bus_vectors rotator_init_vectors[] = {
	{
		.src = MSM_BUS_MASTER_ROTATOR,
		.dst = MSM_BUS_SLAVE_SMI,
		.ab = 0,
		.ib = 0,
	},
	{
		.src = MSM_BUS_MASTER_ROTATOR,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab = 0,
		.ib = 0,
	},
};

static struct msm_bus_vectors rotator_ui_vectors[] = {
	{
		.src = MSM_BUS_MASTER_ROTATOR,
		.dst = MSM_BUS_SLAVE_SMI,
		.ab  = 0,
		.ib  = 0,
	},
	{
		.src = MSM_BUS_MASTER_ROTATOR,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab  = (1024 * 600 * 4 * 2 * 60),
		.ib  = (1024 * 600 * 4 * 2 * 60 * 1.5),
	},
};

static struct msm_bus_vectors rotator_vga_vectors[] = {
	{
		.src = MSM_BUS_MASTER_ROTATOR,
		.dst = MSM_BUS_SLAVE_SMI,
		.ab  = (640 * 480 * 2 * 2 * 30),
		.ib  = (640 * 480 * 2 * 2 * 30 * 1.5),
	},
	{
		.src = MSM_BUS_MASTER_ROTATOR,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab  = (640 * 480 * 2 * 2 * 30),
		.ib  = (640 * 480 * 2 * 2 * 30 * 1.5),
	},
};

static struct msm_bus_vectors rotator_720p_vectors[] = {
	{
		.src = MSM_BUS_MASTER_ROTATOR,
		.dst = MSM_BUS_SLAVE_SMI,
		.ab  = (1280 * 736 * 2 * 2 * 30),
		.ib  = (1280 * 736 * 2 * 2 * 30 * 1.5),
	},
	{
		.src = MSM_BUS_MASTER_ROTATOR,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab  = (1280 * 736 * 2 * 2 * 30),
		.ib  = (1280 * 736 * 2 * 2 * 30 * 1.5),
	},
};

static struct msm_bus_vectors rotator_1080p_vectors[] = {
	{
		.src = MSM_BUS_MASTER_ROTATOR,
		.dst = MSM_BUS_SLAVE_SMI,
		.ab  = (1920 * 1088 * 2 * 2 * 30),
		.ib  = (1920 * 1088 * 2 * 2 * 30 * 1.5),
	},
	{
		.src = MSM_BUS_MASTER_ROTATOR,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab  = (1920 * 1088 * 2 * 2 * 30),
		.ib  = (1920 * 1088 * 2 * 2 * 30 * 1.5),
	},
};

static struct msm_bus_paths rotator_bus_scale_usecases[] = {
	{
		ARRAY_SIZE(rotator_init_vectors),
		rotator_init_vectors,
	},
	{
		ARRAY_SIZE(rotator_ui_vectors),
		rotator_ui_vectors,
	},
	{
		ARRAY_SIZE(rotator_vga_vectors),
		rotator_vga_vectors,
	},
	{
		ARRAY_SIZE(rotator_720p_vectors),
		rotator_720p_vectors,
	},
	{
		ARRAY_SIZE(rotator_1080p_vectors),
		rotator_1080p_vectors,
	},
};

struct msm_bus_scale_pdata rotator_bus_scale_pdata = {
	rotator_bus_scale_usecases,
	ARRAY_SIZE(rotator_bus_scale_usecases),
	.name = "rotator",
};

static struct msm_bus_vectors mdp_init_vectors[] = {
	/* For now, 0th array entry is reserved.
	 * Please leave 0 as is and don't use it
	 */
	{
		.src = MSM_BUS_MASTER_MDP_PORT0,
		.dst = MSM_BUS_SLAVE_SMI,
		.ab = 0,
		.ib = 0,
	},
	/* Master and slaves can be from different fabrics */
	{
		.src = MSM_BUS_MASTER_MDP_PORT0,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab = 0,
		.ib = 0,
	},
};

static struct msm_bus_vectors mdp_sd_smi_vectors[] = {
	/* Default case static display/UI/2d/3d if FB SMI */
	{
		.src = MSM_BUS_MASTER_MDP_PORT0,
		.dst = MSM_BUS_SLAVE_SMI,
		.ab = 375367680,    //(1024x768x60x4)+(1920x1080x60x1.5)
		.ib = 469209600,    //.ab x 1.25
	},
	/* Master and slaves can be from different fabrics */
	{
		.src = MSM_BUS_MASTER_MDP_PORT0,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab = 0,
		.ib = 0,
	},
};

static struct msm_bus_vectors mdp_sd_ebi_vectors[] = {
	/* Default case static display/UI/2d/3d if FB SMI */
	{
		.src = MSM_BUS_MASTER_MDP_PORT0,
		.dst = MSM_BUS_SLAVE_SMI,
		.ab = 375367680,    //(1024x768x60x4)+(1920x1080x60x1.5)
		.ib = 469209600,    //.ab x 1.25
	},
	/* Master and slaves can be from different fabrics */
	{
		.src = MSM_BUS_MASTER_MDP_PORT0,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab = 375367680,    //(1024x768x60x4)+(1920x1080x60x1.5)
		.ib = 938419200,    //.ab x 1.25 x 2
	},
};
static struct msm_bus_vectors mdp_vga_vectors[] = {
	/* VGA and less video */
	{
		.src = MSM_BUS_MASTER_MDP_PORT0,
		.dst = MSM_BUS_SLAVE_SMI,
		.ab = 216391680,    //(1024x768x60x4)+(640x480x60x1.5)
		.ib = 270489600,    //.ab x 1.25
	},
	{
		.src = MSM_BUS_MASTER_MDP_PORT0,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab = 216391680,    //(1024x768x60x4)+(640x480x60x1.5)
		.ib = 540979200,    //.ab x 1.25 x 2
	},
};

static struct msm_bus_vectors mdp_720p_vectors[] = {
	/* 720p and less video */
	{
		.src = MSM_BUS_MASTER_MDP_PORT0,
		.dst = MSM_BUS_SLAVE_SMI,
		.ab = 271687680,    //(1024x768x60x4)+(1280x720x60x1.5)
		.ib = 339609600,    //.ab x 1.25
	},
	/* Master and slaves can be from different fabrics */
	{
		.src = MSM_BUS_MASTER_MDP_PORT0,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab = 271687680,    //(1024x768x60x4)+(1280x720x60x1.5)
		.ib = 679219200,    //.ab x 1.25 x 2
	},
};

static struct msm_bus_vectors mdp_1080p_vectors[] = {
	/* 1080p and less video */
	{
		.src = MSM_BUS_MASTER_MDP_PORT0,
		.dst = MSM_BUS_SLAVE_SMI,
		.ab = 941598720,    //(1024x768x60x4x4)+(1920x1080x60x1.5)
		.ib = 1176998400,    //.ab x 1.25
	},
	/* Master and slaves can be from different fabrics */
	{
		.src = MSM_BUS_MASTER_MDP_PORT0,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab = 941598720,    //(1024x768x60x4x4)+(1920x1080x60x1.5)
		.ib = 2000000000,    //.ab x 1.25 x 2 (or MAX)
	},
};

static struct msm_bus_paths mdp_bus_scale_usecases[] = {
	{
		ARRAY_SIZE(mdp_init_vectors),
		mdp_init_vectors,
	},
	{
		ARRAY_SIZE(mdp_sd_smi_vectors),
		mdp_sd_smi_vectors,
	},
	{
		ARRAY_SIZE(mdp_sd_ebi_vectors),
		mdp_sd_ebi_vectors,
	},
	{
		ARRAY_SIZE(mdp_vga_vectors),
		mdp_vga_vectors,
	},
	{
		ARRAY_SIZE(mdp_720p_vectors),
		mdp_720p_vectors,
	},
	{
		ARRAY_SIZE(mdp_1080p_vectors),
		mdp_1080p_vectors,
	},
};
static struct msm_bus_scale_pdata mdp_bus_scale_pdata = {
	mdp_bus_scale_usecases,
	ARRAY_SIZE(mdp_bus_scale_usecases),
	.name = "mdp",
};
#endif

#ifdef CONFIG_MSM_BUS_SCALING
static struct msm_bus_vectors dtv_bus_init_vectors[] = {
	/* For now, 0th array entry is reserved.
	 * Please leave 0 as is and don't use it
	 */
	{
		.src = MSM_BUS_MASTER_MDP_PORT0,
		.dst = MSM_BUS_SLAVE_SMI,
		.ab = 0,
		.ib = 0,
	},
	/* Master and slaves can be from different fabrics */
	{
		.src = MSM_BUS_MASTER_MDP_PORT0,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab = 0,
		.ib = 0,
	},
};
static struct msm_bus_vectors dtv_bus_def_vectors[] = {
	/* For now, 0th array entry is reserved.
	 * Please leave 0 as is and don't use it
	 */
	{
		.src = MSM_BUS_MASTER_MDP_PORT0,
		.dst = MSM_BUS_SLAVE_SMI,
		.ab = 566092800,
		.ib = 707616000,
	},
	/* Master and slaves can be from different fabrics */
	{
		.src = MSM_BUS_MASTER_MDP_PORT0,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab = 566092800,
		.ib = 707616000,
	},
};

static struct msm_bus_vectors dtv_bus_hdmi_prim_vectors[] = {
        /* For now, 0th array entry is reserved.
         * Please leave 0 as is and don't use it
         */
        {
                .src = MSM_BUS_MASTER_MDP_PORT0,
                .dst = MSM_BUS_SLAVE_SMI,
                .ab = 2000000000,
                .ib = 2000000000,
        },
        /* Master and slaves can be from different fabrics */
        {
                .src = MSM_BUS_MASTER_MDP_PORT0,
                .dst = MSM_BUS_SLAVE_EBI_CH0,
                .ab = 2000000000,
                .ib = 2000000000,
        },
};

static struct msm_bus_paths dtv_bus_scale_usecases[] = {
	{
		ARRAY_SIZE(dtv_bus_init_vectors),
		dtv_bus_init_vectors,
	},
	{
		ARRAY_SIZE(dtv_bus_def_vectors),
		dtv_bus_def_vectors,
	},
};

static struct msm_bus_scale_pdata dtv_bus_scale_pdata = {
	dtv_bus_scale_usecases,
	ARRAY_SIZE(dtv_bus_scale_usecases),
	.name = "dtv",
};

static struct lcdc_platform_data dtv_pdata = {
	.bus_scale_table = &dtv_bus_scale_pdata,
};

static struct msm_bus_paths dtv_hdmi_prim_bus_scale_usecases[] = {
        {
                ARRAY_SIZE(dtv_bus_init_vectors),
                dtv_bus_init_vectors,
        },
        {
                ARRAY_SIZE(dtv_bus_hdmi_prim_vectors),
                dtv_bus_hdmi_prim_vectors,
        },
};

static struct msm_bus_scale_pdata dtv_hdmi_prim_bus_scale_pdata = {
        dtv_hdmi_prim_bus_scale_usecases,
        ARRAY_SIZE(dtv_hdmi_prim_bus_scale_usecases),
        .name = "dtv",
};

static struct lcdc_platform_data dtv_hdmi_prim_pdata = {
        .bus_scale_table = &dtv_hdmi_prim_bus_scale_pdata,
};
#endif


#ifdef CONFIG_FB_MSM_MIPI_DSI
int mdp_core_clk_rate_table[] = {
	200000000,
	200000000,
	200000000,
	200000000,
};
#else
int mdp_core_clk_rate_table[] = {
	59080000,
	85330000,
	128000000,
	200000000,
};
#endif

static struct msm_panel_common_pdata mdp_pdata = {
	.mdp_core_clk_rate = 160000000,//59080000,
	.mdp_core_clk_table = mdp_core_clk_rate_table,
	.num_mdp_clk = ARRAY_SIZE(mdp_core_clk_rate_table),
#ifdef CONFIG_MSM_BUS_SCALING
	.mdp_bus_scale_table = &mdp_bus_scale_pdata,
#endif
	.mdp_rev = MDP_REV_41,
#ifdef CONFIG_MSM_MULTIMEDIA_USE_ION
	.mem_hid = ION_CP_WB_HEAP_ID,
#else
	.mem_hid = MEMTYPE_EBI1,
#endif
};

void __init msm8x60_mdp_writeback(struct memtype_reserve* reserve_table)
{
	mdp_pdata.ov0_wb_size = MSM_FB_OVERLAY0_WRITEBACK_SIZE;
	mdp_pdata.ov1_wb_size = MSM_FB_OVERLAY1_WRITEBACK_SIZE;
#if defined(CONFIG_ANDROID_PMEM) && !defined(CONFIG_MSM_MULTIMEDIA_USE_ION)
    reserve_table[mdp_pdata.mem_hid].size +=
        mdp_pdata.ov0_wb_size;
    reserve_table[mdp_pdata.mem_hid].size +=
        mdp_pdata.ov1_wb_size;
#endif
}

void __init msm_fb_add_devices(void)
{
	msm_fb_register_device("mdp", &mdp_pdata);

	msm_fb_register_device("mipi_dsi", &mipi_dsi_pdata);
#ifdef CONFIG_MSM_BUS_SCALING
        if (hdmi_is_primary)
                msm_fb_register_device("dtv", &dtv_hdmi_prim_pdata);
        else
                msm_fb_register_device("dtv", &dtv_pdata);
#endif
}

