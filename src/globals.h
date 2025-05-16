#pragma once
#include <pico.h>
#include "stdio.h"
#include "screen_util.h"
#include "config.h"

#define TRDOS_COMPILE

#define FW_VERSION "v"SOFT_VERSION
#define FW_AUTHOR "TecnoCat"

#define SCREEN_H (240)
#define SCREEN_W (320)
#define V_BUF_SZ (SCREEN_H*SCREEN_W)+128

#define DIRS_DEPTH (10)
#define MAX_FILES  (400)
#define SD_BUFFER_SIZE 0x0C00  //Размер буфера для работы с файлами
#define FILE_NAME_LEN (13) //name=8+ext=3+dot=1+ftype=1

#define BOOT_LOGO		0x00
#define MENU_MAIN		0x01
#define MENU_JOY_MAIN	0x02
#define MENU_HELP		0x03
#define MENU_SETTINGS	0x04
#define EMULATION		0xFF

#define TEMP_BUFF_SIZE_X 0x0400
#define TEMP_BUFF_SIZE_Y 0x0A00

#define GAME_RAM_SIZE 0x21000

#define PREVIEW_PIX_SIZE 0x6400
#define PREVIEW_PAL_SIZE 0x0320

#define BYTE_TO_BINARY_PATTERN "%c%c%c%c%c%c%c%c"
#define BYTE_TO_BINARY(byte)  \
(byte & 0x80 ? '1' : '0'), \
(byte & 0x40 ? '1' : '0'), \
(byte & 0x20 ? '1' : '0'), \
(byte & 0x10 ? '1' : '0'), \
(byte & 0x08 ? '1' : '0'), \
(byte & 0x04 ? '1' : '0'), \
(byte & 0x02 ? '1' : '0'), \
(byte & 0x01 ? '1' : '0') 

extern uint8_t game_buff[GAME_RAM_SIZE];

extern char buf[10];			//временный буфер
extern char temp_msg[60]; // Буфер для вывода строк


extern bool kbd_lock;

//#define NUM_V_BUF (3)
#ifdef NUM_V_BUF
	extern  bool is_show_frame[NUM_V_BUF];
	extern int draw_vbuf_inx;
	extern int show_vbuf_inx;
#endif


//extern bool int_en;
extern uint8_t graph_buf[];
extern uint8_t hud_line[];
extern uint8_t color_zx[16];

extern uint8_t temp_buffer_x[TEMP_BUFF_SIZE_X];
extern uint8_t temp_buffer_y[TEMP_BUFF_SIZE_Y];


extern uint8_t sd_buffer[SD_BUFFER_SIZE];

int	null_printf(const char *str, ...);

#define FAST_FUNC __not_in_flash_func

uint32_t my_millis();
