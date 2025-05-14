#pragma once
#include "inttypes.h"
#include "stdbool.h"

#define CONFIG_FILENAME "/murmulator_gw.cfg"
#define CONFIG_LINES 65

extern const uint8_t load_pins[2];

/*
typedef enum config{
    cfg_boot_scr		= 0,
    cfg_hud_enable		= 1,
    cfg_tap_load_mode	= 2,
    cfg_def_joy_mode	= 3,
	cfg_res_before_mode	= 4,
	cfg_sound_mode		= 5,
	cfg_tschip_order	= 6,
	cfg_video_out		= 7,
	cfg_frame_rate		= 8,
	cfg_brightness		= 9,
	cfg_volume			= 10
}config;


uint8_t config_values[11];

typedef struct{
    uint8_t min_value;
    uint8_t max_value;
    uint8_t def_value;
} settings_lim_t;

const settings_lim_t settings_limits [11] = {
	{	.min_value = 0,		.max_value = 2,		.def_value=0},		//cfg_boot_scr
	{	.min_value = 0,		.max_value = 1,		.def_value=1},		//cfg_hud_enable
	{	.min_value = 0,		.max_value = 2,		.def_value=0},		//cfg_tap_load_mode
	{	.min_value = 0,		.max_value = 9,		.def_value=0},		//cfg_def_joy_mode
	{	.min_value = 0,		.max_value = 1,		.def_value=0},		//cfg_res_before_mode
	{	.min_value = 0,		.max_value = 4,		.def_value=0},		//cfg_sound_mode
	{	.min_value = 0,		.max_value = 1,		.def_value=0},		//cfg_tschip_order
	{	.min_value = 0,		.max_value = 6,		.def_value=0},		//cfg_video_out
	{	.min_value = 0,		.max_value = 3,		.def_value=0},		//cfg_frame_rate
	{	.min_value = 0,		.max_value = 10,	.def_value=5},		//cfg_brightness
	{	.min_value = 0,		.max_value = 255,	.def_value=104},	//cfg_volume
};
*/

#define BOOT_SCR_LOGO       0
#define BOOT_SCR_FILE_MAN   1
#define BOOT_SCR_EMULATION  2
#define MAX_CFG_BOOT_SCR_MODE 2
#define DEF_CFG_BOOT_SCR_MODE 0
extern uint8_t cfg_boot_scr;

#define HUD_OFF 0
#define HUD_ON 1
#define HUD_TIME 2
#define MAX_CFG_HUD_MODE 2
#define DEF_CFG_HUD_MODE 1
extern uint8_t cfg_hud_enable;

#define OUT_PWM 0
#define OUT_PCM 1
#define MAX_CFG_OUT_MODE 1
#define DEF_CFG_OUT_MODE 0
extern uint8_t cfg_sound_out_mode;

#ifdef SOFT_COMPOSITE_TV
	#define MAX_CFG_VOLUME_MODE 248
#else
	#define MAX_CFG_VOLUME_MODE 256
#endif
#define DEF_CFG_VOLUME_MODE 104
#define CFG_VOLUME_STEP 8
extern short int cfg_volume;

#define MAX_CFG_VIDEO_MODE 7 
#define DEF_CFG_VIDEO_MODE 0
extern uint8_t cfg_video_out;

#define MAX_CFG_VIDEO_FREQ_MODE 3
#define DEF_CFG_VIDEO_FREQ_MODE 0
extern uint8_t cfg_frame_rate;

#define MIN_CFG_LCD_VIDEO_MODE 3 
#define MAX_CFG_LCD_VIDEO_MODE 7 
#define DEF_CFG_LCD_VIDEO_MODE 3
extern uint8_t cfg_lcd_video_out;

#define MAX_CFG_ROTATE 3
#define DEF_CFG_ROTATE 0
extern uint8_t cfg_rotate;

#define MAX_CFG_INVERSION 1
#define DEF_CFG_INVERSION 0
extern uint8_t cfg_inversion;

#define MAX_CFG_PIXELS 1
#define DEF_CFG_PIXELS 0
extern uint8_t cfg_pixels;

#define MAX_CFG_BRIGHT_MODE 10
#define DEF_CFG_BRIGHT_MODE 5
extern uint8_t cfg_brightness;

#define MOBILE_MURM_OFF 0
#define MOBILE_MURM_ON 1
#define MAX_CFG_MOBILE_MODE 1
#define DEF_CFG_MOBILE_MODE 0
extern uint8_t cfg_mobile_mode;

void config_defaults();
bool config_read();
bool config_save();
bool config_write_defaults();
