#include "iface.h"
#include "inttypes.h"

const char *iface_btn[5][3] ={
	{"[Ok]\0",		"*\0",			"*\0"},
	{"[Ok]\0",		"[Cancel]\0",	"*\0"},
	{"[Ok]\0",		"[Retry]\0",	"[Cancel]\0"},
	{"[Abort]\0",	"[Retry]\0",	"[Cancel]\0"},
	{"[Cancel]\0",	"*\0",			"*\0"},
};


const uint8_t iface_res[5][4] ={
	{DLG_RES_OK,		DLG_RES_NONE,		DLG_RES_NONE	,DLG_RES_NONE},
	{DLG_RES_OK,		DLG_RES_CANCEL,		DLG_RES_NONE	,DLG_RES_NONE},
	{DLG_RES_OK,		DLG_RES_RET,		DLG_RES_CANCEL	,DLG_RES_NONE},
	{DLG_RES_ABR,		DLG_RES_RET,		DLG_RES_CANCEL	,DLG_RES_NONE},
	{DLG_RES_CANCEL,	DLG_RES_NONE,		DLG_RES_NONE	,DLG_RES_NONE},
};