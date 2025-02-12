/* Copyright (c) 2011, Code Aurora Forum. All rights reserved.
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
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 *
 */


#include "sensor_mt9e013.h"

static struct mt9e013_i2c_reg_conf mipi_settings[] = {
	{0x3064, 0x7800},	//embedded_data_enable
	{0x31AE, 0x0202},	//2-lane MIPI SERIAL_FORMAT
//	{0x31B8, 0x0E3F},	//MIPI_timing
	/*set data to RAW10 format*/
	{0x0112, 0x0A0A},	/*CCP_DATA_FORMAT*/
	{0x30F0, 0x800D},	/*VCM CONTROL : Enable Power collapse, vcm_slew =5*/
};

/*PLL Configuration
(Ext=24MHz, vt_pix_clk=174MHz, op_pix_clk=69.6MHz)*/
static struct mt9e013_i2c_reg_conf pll_settings[] = {
	{0x0300, 0x0004},	//VT_PIX_CLK_DIV=4
	{0x0302, 0x0001},	//VT_SYS_CLK_DIV=1
	{0x0304, 0x0002},	//PRE_PLL_CLK_DIV=2 //Note: 24MHz/2=12MHz
	{0x0306, 0x0040},	//PLL_MULTIPLIER=64 //Note: Running at 768MHz
	{0x0308, 0x000A},	//OP_PIX_CLK_DIV=10
	{0x030A, 0x0001},	//OP_SYS_CLK_DIV=1
};

static struct mt9e013_i2c_reg_conf prev_settings[] = {
	/*Output Size (1640x1232)*/
// LGE_DOM_UPDATE_S sungsik.kim 2011/10/05 {
	{0x0344, 0x0000},/*X_ADDR_START*/
	{0x0348, 0x0CCF},/*X_ADDR_END*/	//3279				//jungki.kim@lge.com  2011-12-12  Remove Black Line on the Left-side		//{0x0348, 0x0CD1},/*X_ADDR_END*/
	{0x0346, 0x0000},/*Y_ADDR_START*/
	{0x034A, 0x09A1},/*Y_ADDR_END*/
	{0x034C, 0x0668},/*X_OUTPUT_SIZE*/
	{0x034E, 0x04D0},/*Y_OUTPUT_SIZE*/
// LGE_DOM_UPDATE_E sungsik.kim 2011/10/05 }

	{0x306E, 0xFCB0},/*DATAPATH_SELECT*/
	{0x3040, 0xC4C3},/*READ_MODE*/
	{0x3178, 0x0000},/*ANALOG_CONTROL5*/
	{0x3ED0, 0x1E24},/*DAC_LD_4_5*/
	{0x0400, 0x0002},/*SCALING_MODE*/
	{0x0404, 0x0010},/*SCALE_M*/
	/*Timing configuration*/
	{0x0342, 0x1280},/*LINE_LENGTH_PCK*/
	{0x0340, 0x0563},/*FRAME_LENGTH_LINES*/
	{0x0202, 0x055F},/*COARSE_INTEGRATION_TIME*/
	{0x3014, 0x0846},/*FINE_INTEGRATION_TIME_*/
	{0x3010, 0x0130},/*FINE_CORRECTION*/
	
};

static struct mt9e013_i2c_reg_conf snap_settings[] = {
	/*Output Size (3280x2464)*/
	//[2-lane MIPI 3280x2464 14.8FPS 67.6ms RAW10 Ext=24MHz Vt_pix_clk=192MHz Op_pix_clk=76.8MHz FOV=3280x2464] 
	{0x0344, 0x0000},		//X_ADDR_START 0
	{0x0348, 0x0CCF},		//X_ADDR_END 3279
	{0x0346, 0x0000},		//Y_ADDR_START 0
	{0x034A, 0x099F},		//Y_ADDR_END 2463
	{0x034C, 0x0CD0},		//X_OUTPUT_SIZE 3280
	{0x034E, 0x09A0},		//Y_OUTPUT_SIZE 2464

	{0x306E, 0xFC80},/*DATAPATH_SELECT*/
	{0x3040, 0xC041},/*READ_MODE*/
	{0x3178, 0x0000},/*ANALOG_CONTROL5*/
	{0x3ED0, 0x1E24},/*DAC_LD_4_5*/
	{0x0400, 0x0000},/*SCALING_MODE*/
	{0x0404, 0x0010},/*SCALE_M*/

	/*Timing configuration*/
	{0x0342, 0x1370},/*LINE_LENGTH_PCK*/
	{0x0340, 0x0A2F},/*FRAME_LENGTH_LINES*/
// LGE_DOM_UPDATE_S sungsik.kim 2011/11/01 {
// shutter lag
//	{0x0202, 0x0A2F},/*COARSE_INTEGRATION_TIME*/
// LGE_DOM_UPDATE_E sungsik.kim 2011/11/01 }
	{0x3014, 0x03F6},/*FINE_INTEGRATION_TIME_ */
	{0x3010, 0x0078},/*FINE_CORRECTION*/

};

static struct mt9e013_i2c_reg_conf FHD_settings[] = {
	/*Output Size (2640x1486)*/
	{0x0344, 0x0140},	//X_ADDR_START 320
	{0x0348, 0x0B8F},	//X_ADDR_END 2959
	{0x0346, 0x01EA},	//Y_ADDR_START 490
	{0x034A, 0x07B7},	//Y_ADDR_END 1975
	{0x034C, 0x0A50},	//X_OUTPUT_SIZE 2640
	{0x034E, 0x05CE},	//Y_OUTPUT_SIZE 1486

	{0x306E, 0xFC80},/*DATAPATH_SELECT*/
	{0x3040, 0xC041},/*READ_MODE*/
	{0x3178, 0x0000},/*ANALOG_CONTROL5*/
	{0x3ED0, 0x1E24},/*DAC_LD_4_5*/
	{0x0400, 0x0000},/*SCALING_MODE*/
	{0x0404, 0x0010},/*SCALE_M*/
	/*Timing configuration*/
	{0x0342, 0x0FD8},/*LINE_LENGTH_PCK*/		//4056
	{0x0340, 0x065D},/*FRAME_LENGTH_LINES*/		//1629
	{0x0202, 0x0629},/*COARSE_INTEGRATION_TIME*///1577
	{0x3014, 0x0C82},/*FINE_INTEGRATION_TIME_ *///3202
	{0x3010, 0x0078},/*FINE_CORRECTION*///120
};


static struct mt9e013_i2c_reg_conf recommend_settings[] = {
	//mipi timing setting
	{0x31B0, 0x0083},
	{0x31B2, 0x004D},
	{0x31B4, 0x0E67},
	{0x31B6, 0x0D24},
	{0x31B8, 0x020E},
	{0x31BA, 0x0710},
	{0x31BC, 0x2A0D},
	{0x31BE, 0xC007},	// 0xC007 : continuous, 0xC003 : noncontinuous

	//Recommended Settings
	{0x3044, 0x0590},
	{0x306E, 0xFC80},
	{0x30B2, 0xC000},
	{0x30D6, 0x0800},
	{0x316C, 0xB42A},
	{0x316E, 0x869C},	//{0x316E, 0x869B},	// jungki.kim@lge.com  2011-10-24  not to appear the sun to be black
	{0x3170, 0x210E},
	{0x317A, 0x010E},
	{0x31E0, 0x1FB9},
	{0x31E6, 0x07FC},
	{0x37C0, 0x0000},
	{0x37C2, 0x0000},
	{0x37C4, 0x0000},
	{0x37C6, 0x0000},
	{0x3E00, 0x0011},
	{0x3E02, 0x8801},
	{0x3E04, 0x2801},
	{0x3E06, 0x8449},
	{0x3E08, 0x6841},
	{0x3E0A, 0x400C},
	{0x3E0C, 0x1001},
	{0x3E0E, 0x2603},
	{0x3E10, 0x4B41},
	{0x3E12, 0x4B24},
	{0x3E14, 0xA3CF},
	{0x3E16, 0x8802},
	{0x3E18, 0x8401},
	{0x3E1A, 0x8601},
	{0x3E1C, 0x8401},
	{0x3E1E, 0x840A},
	{0x3E20, 0xFF00},
	{0x3E22, 0x8401},
	{0x3E24, 0x00FF},
	{0x3E26, 0x0088},
	{0x3E28, 0x2E8A},
	{0x3E30, 0x0000},
	{0x3E32, 0x00FF},
	{0x3E34, 0x4029},
	{0x3E36, 0x00FF},
	{0x3E38, 0x8469},
	{0x3E3A, 0x00FF},
	{0x3E3C, 0x2801},
	{0x3E3E, 0x3E2A},
	{0x3E40, 0x1C01},
	{0x3E42, 0xFF84},
	{0x3E44, 0x8401},
	{0x3E46, 0x0C01},
	{0x3E48, 0x8401},
	{0x3E4A, 0x00FF},
	{0x3E4C, 0x8402},
	{0x3E4E, 0x8984},
	{0x3E50, 0x6628},
	{0x3E52, 0x8340},
	{0x3E54, 0x00FF},
	{0x3E56, 0x4A42},
	{0x3E58, 0x2703},
	{0x3E5A, 0x6752},
	{0x3E5C, 0x3F2A},
	{0x3E5E, 0x846A},
	{0x3E60, 0x4C01},
	{0x3E62, 0x8401},
	{0x3E66, 0x3901},
	{0x3E90, 0x2C01},
	{0x3E98, 0x2B02},
	{0x3E92, 0x2A04},
	{0x3E94, 0x2509},
	{0x3E96, 0xF000},
	{0x3E9A, 0x2905},
	{0x3E9C, 0x00FF},
	{0x3ECC, 0x00D8},	//{0x3ECC, 0x00E4},	//{0x3ECC, 0x00EB},	// jungki.kim@lge.com  2011-12-19  not to appear the sun to be black and vertical lines
	{0x3ED0, 0x1E24},
	{0x3ED4, 0xFAA4},
	{0x3ED6, 0x909B},
	{0x3EE0, 0x2424},
	{0x3EE4, 0xC100},
	{0x3EE6, 0x0540},
	{0x3174, 0x8000},

};


struct mt9e013_reg mt9e013_regs = {
	.reg_mipi = &mipi_settings[0],
	.reg_mipi_size = ARRAY_SIZE(mipi_settings),
	.rec_settings = &recommend_settings[0],
	.rec_size = ARRAY_SIZE(recommend_settings),
	.reg_pll = &pll_settings[0],
	.reg_pll_size = ARRAY_SIZE(pll_settings),

	.reg_prev = &prev_settings[0],
	.reg_prev_size = ARRAY_SIZE(prev_settings),

	.reg_snap = &snap_settings[0],
	.reg_snap_size = ARRAY_SIZE(snap_settings),

	.reg_FHD = &FHD_settings[0],
	.reg_FHD_size = ARRAY_SIZE(FHD_settings),	
};
