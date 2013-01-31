/* Copyright (c) 2011, Code Aurora Forum. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 */

#include "sensor_imx105.h"

struct imx105_i2c_reg_conf const init_tbl[] = {
	{0x0305, 0x0001},
	{0x0307, 0x001C},
	{0x303C, 0x004B},
	{0x3031, 0x0010},
	{0x3064, 0x0012},
	{0x3087, 0x0057},
	{0x308A, 0x0035},
	{0x3091, 0x0041},
	{0x3098, 0x0003},
	{0x3099, 0x00C0},
	{0x309A, 0x00A3},
	{0x309C, 0x0034},
	{0x30AB, 0x0001},
	{0x30AD, 0x0008},
	{0x30F3, 0x0003},
	{0x3116, 0x0031},
	{0x3117, 0x0038},
	{0x3138, 0x0028},
	{0x3137, 0x0014},
	{0x3139, 0x002E},
	{0x314D, 0x002A},
	{0x3343, 0x0004},
	{0x3032, 0x0040}
	//{0x0101, 0x0003}
};

/* Preview  register settings	*/
struct imx105_i2c_reg_conf const mode_preview_tbl[] = {
	{0x0101, 0x0003},//read out type : Flip & mirror
	{0x0340, 0x0005},	//Start : shchang@qualcomm.com 09-19 : to make it more stable and get frequent flicker, increasing Y blanks
	{0x0341, 0x000A},		
	{0x0342, 0x000D},
	{0x0343, 0x00D0},
	{0x0346, 0x0000},
	{0x0347, 0x0024},
	{0x034A, 0x0009},
	{0x034B, 0x00C3},
	{0x034C, 0x0006},
	{0x034D, 0x0068},
	{0x034E, 0x0004},
	{0x034F, 0x00D0},
	{0x0381, 0x0001},
	{0x0383, 0x0003},
	{0x0385, 0x0001},
	{0x0387, 0x0003},
	{0x3033, 0x0000},
	{0x3048, 0x0001}, 
	{0x304C, 0x006F},
	{0x304D, 0x0003},
	{0x306A, 0x00D2},
	{0x309B, 0x0028},
	{0x309E, 0x0000},
//	{0x30AA, 0x0002},
	{0x30D5, 0x0009},
	{0x30D6, 0x0001},
	{0x30D7, 0x0001},
	{0x30DE, 0x0002},
	{0x3102, 0x0008},
	{0x3103, 0x0022},
	{0x3104, 0x0020},
	{0x3105, 0x0000},
	{0x3106, 0x0087},
	{0x3107, 0x0000},
	{0x315C, 0x00A5},
	{0x315D, 0x00A4},
	{0x316E, 0x00A6},
	{0x316F, 0x00A5},
	{0x3318, 0x0072},
	{0x0202, 0x0004},
	{0x0203, 0x00ED}
};

/* Snapshot register settings */
struct imx105_i2c_reg_conf const mode_snapshot_tbl[] = {
	{0x0101, 0x0003},//read out type : Flip & mirror
	{0x0340, 0x0009},
	{0x0341, 0x00CE},	//0x00E6 LGE_BSP_CAMERA::shchang@qualcom.com_0707 for flicker. From REG datasheet of IMX105
	{0x0342, 0x000D},
	{0x0343, 0x00D0},
	{0x0346, 0x0000},
	{0x0347, 0x0024},
	{0x034A, 0x0009},
	{0x034B, 0x00C3},
	{0x034C, 0x000C},
	{0x034D, 0x00D0},
	{0x034E, 0x0009},
	{0x034F, 0x00A0},
	{0x0381, 0x0001},
	{0x0383, 0x0001},
	{0x0385, 0x0001},
	{0x0387, 0x0001},
	{0x3033, 0x0000},
	{0x3048, 0x0000},
	{0x304C, 0x006F},
	{0x304D, 0x0003},
	{0x306A, 0x00D2},
	{0x309B, 0x0020},
	{0x309E, 0x0000},
//	{0x30AA, 0x0002},
	{0x30D5, 0x0000},
	{0x30D6, 0x0085},
	{0x30D7, 0x002A},
	{0x30DE, 0x0000},
	{0x3102, 0x0008},
	{0x3103, 0x0022},
	{0x3104, 0x0020},
	{0x3105, 0x0000},
	{0x3106, 0x0087},
	{0x3107, 0x0000},
	{0x315C, 0x00A5},
	{0x315D, 0x00A4},
	{0x316E, 0x00A6},
	{0x316F, 0x00A5},
	{0x3318, 0x0062},
	{0x0202, 0x0009},
	{0x0203, 0x00E1}
};

// LGE_BSP_CAMERA start 20110516 jisun.shin@lge.com - 1080P
/* 1080p Preview&Recording  register settings */
struct imx105_i2c_reg_conf const mode_preview_1080_tbl[]=
{
	{0x0340, 0x0005},	//Start : shchang@qualcomm.com 09-19 : to make it more stable and get frequent flicker, increasing Y blanks
	{0x0341, 0x000A},
	{0x0342, 0x000D},
	{0x0343, 0x00D0},
	{0x0344, 0x0000},
	{0x0345, 0x0004},
	{0x0346, 0x0001},
	{0x0347, 0x0058},
	{0x0348, 0x000C},
	{0x0349, 0x00D3},
	{0x034A, 0x0008},
	{0x034B, 0x008F},
	{0x034C, 0x000C},
	{0x034D, 0x00D0},
	{0x034E, 0x0004},
	{0x034F, 0x00D0},
	{0x0381, 0x0001},
	{0x0383, 0x0001},
	{0x0385, 0x0002},
	{0x0387, 0x0003},
	{0x303D, 0x0070},
	{0x303E, 0x0040},
	{0x3048, 0x0000}, 
	{0x304C, 0x006F},
	{0x304D, 0x0003},
	{0x306A, 0x00F2},
	{0x309B, 0x0020},
	{0x309C, 0x0034},
//	{0x30AA, 0x0002},
	{0x30D5, 0x0000},
	{0x30D6, 0x0085},
	{0x30D7, 0x002A},
	{0x30D8, 0x0064},
	{0x30D9, 0x0089},
	{0x30DE, 0x0000},
	{0x3318, 0x0062}
};
// LGE_BSP_CAMERA end 20110516 jisun.shin@lge.com - 1080P
// Start LGE_BSP_CAMERA : shutter time reg table - jonghwan.ko@lge.com 
struct imx105_i2c_reg_conf const mode_shutterlag_reg1_tbl[]=
{
	{0x303D, 0x0070},
	{0x0104, 0x0000},
	{0x3035, 0x0010},
	{0x303B, 0x0014},
	{0x3312, 0x0045},
	{0x3313, 0x00C0},
	{0x3310, 0x0020},
	{0x3310, 0x0000},
	{0x303B, 0x0004},
	{0x3035, 0x0000}
};
struct imx105_i2c_reg_conf const mode_shutterlag_reg2_tbl[]=
{
	{0x303D, 0x0060},
	{0x0100, 0x0010},
	{0x3035, 0x0010},
	{0x3035, 0x0000},
	//Mode Setting
//	{0x0340, 0x0009},		//shchang@qualcomm 09-19 : to prevent overwriting for exposure
//	{0x0341, 0x00CE}, 	//shchang@qualcomm 09-19 : to prevent overwriting for exposure
	{0x034C, 0x000C},
	{0x034D, 0x00D0},
	{0x0343, 0x00D0},
	{0x034E, 0x0009},
	{0x034F, 0x00A0},
	{0x0383, 0x0001},
	{0x0387, 0x0001},
	{0x303D, 0x0060},
	{0x3041, 0x0097},
	{0x3048, 0x0000},
	{0x309B, 0x0020},
	{0x30D5, 0x0000},
	{0x30D6, 0x0085},
	{0x30D7, 0x002A},
	{0x30DE, 0x0000},
	{0x3318, 0x0062},
	//Clamp Setting
	{0x30B1, 0x0043},
	//Scanner reset
//	{0x3031, 0x0050},
//	{0x3031, 0x0010},
	//MIPI reset
//	{0x3011, 0x0080},
//	{0x3011, 0x0000},
	//Software standby
	{0x0100, 0x0001},	
};
struct imx105_i2c_reg_conf const mode_shutterlag_reg2_1080p_tbl[]=
{
	{0x303D, 0x0060},
	{0x0100, 0x0010},
	{0x3035, 0x0010},
	{0x3035, 0x0000},
	//Mode Setting
//	{0x0340, 0x0009},		//shchang@qualcomm 09-19 : to prevent overwriting for exposure
//	{0x0341, 0x00CE}, 	//shchang@qualcomm 09-19 : to prevent overwriting for exposure
	{0x0346, 0x0000},
	{0x0347, 0x0024},
	{0x034A, 0x0009},
	{0x034B, 0x00C3},	
	{0x034C, 0x000C},
	{0x034D, 0x00D0},
	{0x0343, 0x00D0},
	{0x034E, 0x0009},
	{0x034F, 0x00A0},
	{0x0383, 0x0001},	
	{0x0385, 0x0001},
	{0x0387, 0x0001},
	{0x303D, 0x0060},
	{0x3041, 0x0097},
	{0x3048, 0x0000},	
	{0x309B, 0x0020},
	{0x30D5, 0x0000},
	{0x30D6, 0x0085},
	{0x30D7, 0x002A},
	{0x30DE, 0x0000},
	{0x306A, 0x00D2},
	{0x3318, 0x0062},
	//Clamp Setting
	{0x30B1, 0x0043},
	//Scanner reset
//	{0x3031, 0x0050},
//	{0x3031, 0x0010},
	//MIPI reset
//	{0x3011, 0x0080},
//	{0x3011, 0x0000},
	//Software standby
	{0x0100, 0x0001},	
};
// End LGE_BSP_CAMERA : shutter time reg table - jonghwan.ko@lge.com 

struct imx105_reg imx105_regs = {
	.init_tbl = &init_tbl[0],
	.inittbl_size = ARRAY_SIZE(init_tbl),

	.prev_tbl = &mode_preview_tbl[0],
	.prevtbl_size = ARRAY_SIZE(mode_preview_tbl),

	.snap_tbl = &mode_snapshot_tbl[0],
	.snaptbl_size = ARRAY_SIZE(mode_snapshot_tbl),

// LGE_BSP_CAMERA start 20110516 jisun.shin@lge.com - 1080P
	.prev_1080_tbl = &mode_preview_1080_tbl[0],
	.prevtbl_1080_size = ARRAY_SIZE(mode_preview_1080_tbl),
// LGE_BSP_CAMERA end 20110516 jisun.shin@lge.com - 1080P

// Start LGE_BSP_CAMERA : shutter time reg table - jonghwan.ko@lge.com 
	.shutter_tbl = &mode_shutterlag_reg1_tbl[0],
	.shuttertbl_size = ARRAY_SIZE(mode_shutterlag_reg1_tbl),
	.shutter2_tbl = &mode_shutterlag_reg2_tbl[0],
	.shutter2tbl_size = ARRAY_SIZE(mode_shutterlag_reg2_tbl),	
	.shutter2_1080p_tbl = &mode_shutterlag_reg2_1080p_tbl[0],
	.shutter2_1080ptbl_size = ARRAY_SIZE(mode_shutterlag_reg2_1080p_tbl),		
//End LGE_BSP_CAMERA : shutter time reg table - jonghwan.ko@lge.com 	

};

