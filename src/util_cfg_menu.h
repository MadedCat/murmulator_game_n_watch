#pragma once
#include <pico/stdlib.h>
#include "util_cfg.h"

#define CONFIG_MENU_ITEMS 20

const char __in_flash() *config_menu[CONFIG_MENU_ITEMS]={
	"Boot go to  :[            ]",	//0
	"Head Up Display:     [    ]",	//1
	"Sound out mode:       [   ]",	//2
	"Sound Vol:     [          ]",	//3
	"Video output:  [          ]",	//4
	"Video framerate:   [      ]",	//5
	"Mobile Murmulator:    [   ]",	//6
	"LCD BrightLev: [          ]",  //7
	"LCD Rotate:    [          ]",  //8
	"LCD inversion: [          ]",  //9
	"LCD Pixel format:   [     ]",  //10
	"           [LOAD]          ",  //11
	"          [DEFAULT]        ",  //12
	"           [SAVE]          ",  //13
	"     [UPDATE FIRMWARE]     ",  //14
	"       [!!!REBOOT!!!]      ",  //15
	"\0",	//26
};

const char __in_flash() *boot_scr_config[3]={
	"    LOGO    ",
	"FILE MANAGER",
	" EMULATION  ",
};

const char __in_flash() *HUD_config[3]={
	"OFF ",
	"ON  ",
	"TIME",
};

const char __in_flash() *yes_no[2]={
	"No ",
	"Yes"
};

const char __in_flash() *sound_out_config[2]={
	"PWM",
	"I2S",
};


#ifdef VGA_HDMI
const char __in_flash() *video_out_config[8]={
	"   AUTO   ",
	"   VGA    ",
	"   HDMI   ",
	"  ST7789  ",
	"  ST7789V ",
	"  ILI9341 ",
	" ILI9341V ",
	"  GC9A01  "
};
const char __in_flash() *video_out_rotate[4]={
	"Horizont R",
	"Vertical R",
	"Horizont L",
	"Vertical L",
};
const char __in_flash() *video_out_inversion[2]={
	"  NORMAL  ",
	" INVERTED ",
};
const char __in_flash() *video_out_pixels[2]={
	" BGR ",
	" RGB ",
};

#endif

#ifdef COMPOSITE_TV
const char __in_flash() *video_out_config[3]={
	"   AUTO   ",
	"   NTSC   ",
	"   PAL    ",
};
#endif

#ifdef SOFT_COMPOSITE_TV
const char __in_flash() *video_out_config[3]={
	"   AUTO   ",
	"   NTSC   ",
	"   PAL    ",
};
#endif

const char __in_flash() *video_freq_config[4]={
	" 60Hz ",
	" 72Hz ",
	" 75Hz ",
	" 85Hz ",
};

const char __in_flash() *gaudge[11]={
	"          ",
	"*         ",
	"**        ",
	"***       ",
	"****      ",
	"*****     ",
	"******    ",
	"*******   ",
	"********  ",
	"********* ",
	"**********",
};