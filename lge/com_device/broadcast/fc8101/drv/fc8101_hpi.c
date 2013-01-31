/*****************************************************************************
 Copyright(c) 2009 FCI Inc. All Rights Reserved

 File name : fc8101_hpi.c

 Description : fc8110 host interface

 History :
 ----------------------------------------------------------------------
 2009/09/14 	jason		initial
*******************************************************************************/

#include "fci_types.h"
#include "fc8101_regs.h"

#define HPIC_READ			0x01	// read command
#define HPIC_WRITE			0x02	// write command
#define HPIC_AINC			0x04	// address increment
#define HPIC_BMODE			0x00	// byte mode
#define HPIC_WMODE          0x10	// word mode
#define HPIC_LMODE          0x20	// long mode
#define HPIC_ENDIAN			0x00	// little endian
#define HPIC_CLEAR			0x80	// currently not used

#define BBM_BASE_ADDR       0
#define BBM_BASE_OFFSET     0

#define FC8101_CMD_REG			(*(volatile u8 *)(BBM_BASE_ADDR + (BBM_COMMAND_REG << BBM_BASE_OFFSET)))
#define FC8101_ADDR_REG			(*(volatile u8 *)(BBM_BASE_ADDR + (BBM_ADDRESS_REG << BBM_BASE_OFFSET)))
#define FC8101_DATA_REG			(*(volatile u8 *)(BBM_BASE_ADDR + (BBM_DATA_REG << BBM_BASE_OFFSET)))

int fc8101_hpi_init(HANDLE hDevice, u16 param1, u16 param2)
{

	return BBM_OK;
}

int fc8101_hpi_byteread(HANDLE hDevice, u16 addr, u8 *data)
{
	FC8101_CMD_REG = HPIC_READ | HPIC_BMODE | HPIC_ENDIAN;

	FC8101_ADDR_REG = (addr & 0xff);

	*data = FC8101_DATA_REG;
	
	return BBM_OK;
}

int fc8101_hpi_wordread(HANDLE hDevice, u16 addr, u16 *data)
{
	u8 command = HPIC_READ | HPIC_AINC | HPIC_BMODE | HPIC_ENDIAN;

	FC8101_CMD_REG = command;

	FC8101_ADDR_REG = (addr & 0xff);

	*data = FC8101_DATA_REG;
	*data |= FC8101_DATA_REG << 8;
	
	return BBM_OK;
}

int fc8101_hpi_longread(HANDLE hDevice, u16 addr, u32 *data)
{
	FC8101_CMD_REG = HPIC_READ | HPIC_AINC | HPIC_BMODE | HPIC_ENDIAN;

	FC8101_ADDR_REG = (addr & 0xff);

	*data = FC8101_DATA_REG;
	*data |= FC8101_DATA_REG << 8;
	*data |= FC8101_DATA_REG << 16;
	*data |= FC8101_DATA_REG << 24;

	return BBM_OK;
}

int fc8101_hpi_bulkread(HANDLE hDevice, u16 addr, u8 *data, u16 length)
{
	s32 i;
	
	FC8101_CMD_REG = HPIC_READ | HPIC_AINC | HPIC_BMODE | HPIC_ENDIAN;

	FC8101_ADDR_REG = (addr & 0xff);

	for(i=0; i<length; i++) {
		data[i] = FC8101_DATA_REG;
	}

	return BBM_OK;
}

int fc8101_hpi_bytewrite(HANDLE hDevice, u16 addr, u8 data)
{
	FC8101_CMD_REG = HPIC_WRITE | HPIC_BMODE | HPIC_ENDIAN;

	FC8101_ADDR_REG = (addr & 0xff);

	FC8101_DATA_REG = data;

	return BBM_OK;
}

int fc8101_hpi_wordwrite(HANDLE hDevice, u16 addr, u16 data)
{
	u8 command = HPIC_WRITE | HPIC_BMODE | HPIC_ENDIAN | HPIC_AINC;
	
	FC8101_CMD_REG = command;

	FC8101_ADDR_REG = (addr & 0xff);

	FC8101_DATA_REG = (data & 0xff);
	FC8101_DATA_REG = (data & 0xff00) >> 8;

	return BBM_OK;
}

int fc8101_hpi_longwrite(HANDLE hDevice, u16 addr, u32 data)
{
	FC8101_CMD_REG = HPIC_WRITE | HPIC_AINC | HPIC_BMODE | HPIC_ENDIAN;

	FC8101_ADDR_REG = (addr & 0xff);

	FC8101_DATA_REG = (data & 0xff);
	FC8101_DATA_REG = (data & 0xff00) >> 8;
	FC8101_DATA_REG = (data & 0xff0000) >> 16;
	FC8101_DATA_REG = (data & 0xff000000) >> 24;

	return BBM_OK;
}

int fc8101_hpi_bulkwrite(HANDLE hDevice, u16 addr, u8* data, u16 length)
{
	s32 i;

	FC8101_CMD_REG = HPIC_WRITE | HPIC_AINC | HPIC_BMODE | HPIC_ENDIAN;


	FC8101_ADDR_REG = (addr & 0xff);

	for(i = 0; i < length; i++) {
		FC8101_DATA_REG = data[i];
	}

	return BBM_OK;
}

int fc8101_hpi_dataread(HANDLE hDevice, u16 addr, u8* data, u16 length)
{
	s32 i;

	FC8101_CMD_REG = HPIC_READ | HPIC_BMODE | HPIC_ENDIAN;

	FC8101_ADDR_REG = (addr & 0xff);

	for(i = 0; i < length; i++) {
		data[i] = FC8101_DATA_REG;
	}

	return BBM_OK;
}

int fc8101_hpi_deinit(HANDLE hDevice)
{
	
	return BBM_OK;
}
