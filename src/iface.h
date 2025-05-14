#pragma once
#include "inttypes.h"

#define DLG_RES_NONE	0x00
#define DLG_RES_CANCEL	0xFF
#define DLG_RES_OK		0x01
#define DLG_RES_RET		0x02
#define DLG_RES_ABR		0x03

#define DIALOG_OK			0x00
#define DIALOG_OK_CAN		0x01
#define DIALOG_OK_RET_CAN	0x02
#define DIALOG_ABR_RET_CAN	0x03
#define DIALOG_CAN			0x04

extern const char *iface_btn[5][3];
extern const uint8_t iface_res[5][4];