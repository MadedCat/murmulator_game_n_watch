#define PICO_FLASH_SPI_CLKDIV 4

#define VGA_HDMI

// #define PICO_FLASH_SIZE_BYTES (4 * 1024 * 1024)

//#define DEBUG_DELAY

//#define DEBUG_BLINK

//#define DEBUG_DISABLE_LOADERS

#define SOUND_SAMPLE_RATE 32768//GW_AUDIO_FREQ //44100

#define TIMER_PERIOD 500 //ms


#define ZX_AY_PWM_PIN0 (26)
#define ZX_AY_PWM_PIN1 (27)
#define ZX_BEEP_PIN (28)

#define WORK_LED_PIN (25)

//#define TST_PIN (29)

#define SHOW_SCREEN_DELAY 1000 //в милисекундах

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


#include <pico.h>
#include <stdio.h>
#include <pico/stdlib.h>
#include <pico/multicore.h>
#include <pico/bootrom.h>
#include <pico/rand.h>
#include <pico/stdio_usb.h>
#include <hardware/sync.h>
#include <hardware/irq.h>
#include <hardware/watchdog.h>
#include <hardware/clocks.h>
#include <hardware/structs/systick.h>
#include <hardware/pwm.h>
#include <hardware/vreg.h>
#include <hardware/flash.h>
#include <math.h>
#include <string.h>
#include "hardware/flash.h"
#include <pico/flash.h>

/*
#define FLASH_TARGET_OFFSET (1024 * 1024)
const char* rom_filename = (const char *)(XIP_BASE + FLASH_TARGET_OFFSET);
const uint8_t* rom = (const uint8_t *)(XIP_BASE + FLASH_TARGET_OFFSET) + 4096;
size_t get_rom4prog_size() { return FLASH_TARGET_OFFSET - 4096; } 

extern char __flash_binary_end;
#define FLASH_TARGET_OFFSET (((((uintptr_t)&__flash_binary_end - XIP_BASE) / FLASH_SECTOR_SIZE) + 4) * FLASH_SECTOR_SIZE)
static const uintptr_t rom = XIP_BASE + FLASH_TARGET_OFFSET;
char __uninitialized_ram(filename[256]);
*/

#define FLASH_TARGET_OFFSET (1ul << 20)
const uint8_t* rom = (const uint8_t *)(XIP_BASE + FLASH_TARGET_OFFSET) + 4096;

#include "globals.h"

#include "../lib/joysticks/Joystics.h"





#ifdef VGA_HDMI
	#include "../lib/video/video.h"
#endif
#ifdef COMPOSITE_TV
	#include "../lib/tv_out/tv_out.h"
#endif
#ifdef SOFT_COMPOSITE_TV
	#include "../lib/tv_out_soft/tv_out.h"
#endif

extern bool graphics_draw_screen;
extern uint32_t graphics_frame_count;

#include "screen_util.h"
#include "iface.h"
#include "util_sd.h"

#include "util_i2c_kbd.h" // i2c keyboard
#include "../lib/sound/i2s.h"

extern void i2s_out(int16_t l_out,int16_t r_out);

#include "hud_func.h"



#include "mur_logo.h"
#include "mur_logo_small.h"
#include "mur_logo_cat.h"
#include "utf_handle.h"
#include "help.h"
#include "fast_menu.h"
#include "ps2.h"
#include "util_cfg.h"
#include "util_cfg_menu.h"
#include "util_power.h"

//#include "image.h"
//#include "palette.h"
//#include "image1.h"

/*-------Emulation--------*/
#include "def_game.h"
#include "system/gw_type_defs.h"
#include "system/gw_romloader.h"
#include "system/gw_system.h"
#include "system/gw_graphic.h"



extern gwromheader_t gw_head;

extern uint8_t *GW_ROM;

/* ROM in RAM : objects pointers */
extern uint8_t  *gw_rom_base;
extern uint16_t *gw_prewiew_pal;
extern uint16_t *gw_prewiew;
extern uint16_t *gw_background_pal;
extern uint16_t *gw_background;
extern uint8_t  *gw_segments;
extern uint16_t *gw_segments_x;
extern uint16_t *gw_segments_y;
extern uint16_t *gw_segments_width;
extern uint16_t *gw_segments_height;
extern uint32_t *gw_segments_offset;
extern uint8_t  *gw_program;
extern uint8_t  *gw_melody;
extern uint32_t *gw_keyboard;

static bool softkey_time_pressed = 0;
static bool softkey_alarm_pressed = 0;
static bool softkey_A_pressed = 0;
static bool softkey_B_pressed = 0;
static bool softkey_Game_A_pressed = 0;
static bool softkey_Game_B_pressed = 0;

static bool softkey_only = 0;
static unsigned int softkey_duration = 0;

volatile bool gw_emulate;
volatile bool emulation=false;
repeating_timer_t emulation_timer;
bool emulation_timer_enabled=false;
volatile bool reload_last_rom=false;

char romfilename[400];
/*-------Emulation--------*/

extern uint8_t cfg_boot_scr;
extern uint8_t cfg_hud_enable;
extern uint8_t cfg_sound_out_mode;
extern short int cfg_volume;
extern uint8_t cfg_video_out;
extern uint8_t cfg_frame_rate;
extern uint8_t cfg_lcd_video_out;
extern uint8_t cfg_rotate;
extern uint8_t cfg_inversion;
extern uint8_t cfg_pixels;
extern uint8_t cfg_brightness;
extern uint8_t cfg_mobile_mode;


/*Forward declarations*/
void process_input(void);
void clear_input(void);
/*Forward declarations*/

bool ack_input = false;

bool flip_led = false;

uint32_t last_action = 0;
bool scroll_lfn = false;
uint32_t scroll_action = 0;
uint16_t scroll_pos =0;


extern uint32_t free_time_ticks;
uint32_t freetime_action = 0;
#define FREE_TIME_DELAY 500 //в милисекундах

bool init_fs=false;
short int settings_index=0;
short int settings_lines=0;
short int menu_inc_dec=0;

bool is_pause_mode=false;
bool i2cKbdMode;
bool keyPressed;
uint8_t i2cBtnPressed = 0;

bool emu_paused = true;

bool read_dir = true;
short int shift_file_index=0;
short int cur_dir_index=0;
short int cur_file_index=0;
short int cursor_index_old[DIRS_DEPTH];
short int display_file_index=0;
short int N_files=0;
short int sel_files=0;
short int del_files=0;
short int err_files=0;
char current_lfn[200];
//uint64_t current_time = 0;
char icon[2];
uint8_t sound_reg_pause[36];
	
short int lineStart=0;	
short int fast_menu_index=0;
short int old_menu_index=0;


uint8_t current_settings=0;
uint8_t current_drive=0;
char save_file_name_image[25];

short int fr =-1;

bool need_reset_after_menu=false;

uint8_t fast_mode[5]={0,0,0,0,0};
uint8_t fast_mode_ptr=0;

uint8_t menu_mode[5]={0,0,0,0,0};
uint8_t menu_ptr=0;

uint8_t current_video_out=0;
uint8_t current_frame_rate=0;
uint8_t current_rotate=0;
uint8_t current_inversion=0;
uint8_t current_pixels=0;
uint8_t current_pin=0;
uint8_t current_sound_out=0;
uint8_t current_mobile_mode=0;

#define NUM_SHOW_FILES 27

bool is_new_screen=false;
bool need_redraw=false;
bool show_screen=true;

uint8_t old_volume = 0;
bool lock_display_off=false;
//char vol_ind[12];

/*battery monitor*/
#define BATT_TEXT_LINE_LEN 5
char batt_text[BATT_TEXT_LINE_LEN+2];
extern uint8_t battery_power_percent;
extern bool battery_power_charge;
/*battery monitor*/

void software_reset(){
	watchdog_enable(1, 1);
	while(1);
}



void inInit(uint gpio){
	gpio_init(gpio);
	gpio_set_dir(gpio,GPIO_IN);
	gpio_pull_up(gpio);
}


#define D_JOY_HAT_UNLOCK	(D_JOY_SELECT|D_JOY_START)

#define HAT_UP		(1<<0)
#define HAT_DOWN	(1<<1)
#define HAT_LEFT	(1<<2)
#define HAT_RIGHT	(1<<3)
#define HAT_A		(1<<4)
#define HAT_B		(1<<5)
#define HAT_SELECT	(1<<6)
#define HAT_START	(1<<7)

volatile uint8_t hat_switch=0;

#define data_joy_2 (data_joy>>16)

bool joy_pressed 	= false;
bool joy_connected	= false;

uint8_t data_joy1=0;							//данные первого джойстика
uint8_t data_joy2=0;							// данные второго джойстика
uint8_t data_ext_joy1=0;						// добавочные кнопки первого джойстика
uint8_t data_ext_joy2=0;						// добавочные кнопки второго джойстика
uint8_t active_type_joystick = NES_joy1_2;				//  тип активного подключенного джойстика

uint32_t data_joy=0;
uint32_t old_data_joy=0;
uint32_t rel_data_joy=0;


void process_input(){
	if(i2cKbdMode){
		keyPressed = i2c_decode_kbd();
	} else {
		keyPressed = decode_PS2();
	}
		
	data_joy_input(active_type_joystick);
								
	uint32_t temp_data_joy;
	temp_data_joy = active_joystick_data(active_type_joystick);		// приведение данных с джойстиков к формату мурмулятора

	data_joy1 = (uint8_t)((temp_data_joy>>8)&0x000000ff);
	data_joy2 = (uint8_t)((temp_data_joy>>24)&0x000000ff);
	data_ext_joy1 = (uint8_t)((temp_data_joy)&0x000000ff);
	data_ext_joy2 = (uint8_t)((temp_data_joy>>16)&0x000000ff);

	data_joy1=(data_joy1&0x0f)|((data_joy1>>2)&0x30)|((data_joy1<<3)&0x80)|((data_joy1<<1)&0x40); 
	data_joy2=(data_joy2&0x0f)|((data_joy2>>2)&0x30)|((data_joy2<<3)&0x80)|((data_joy2<<1)&0x40);

	data_joy = data_joy1|(data_ext_joy1<<8)|(data_joy2<<16)|(data_ext_joy2<<24);

	//printf("JR[%08X]\tJ1[%02X]>[%02X]\tJ2[%02X]>[%02X]\tCJ[%08X]\n",temp_data_joy,data_joy1,data_ext_joy1,data_joy2,data_ext_joy2,data_joy);

	if(Joystics.joy_pressed) {joy_pressed = true;} else{joy_pressed = false;}
}

void clear_input(void){
	//printf("Clear input\n");
	kb_st_ps2.u[0]=0;
	kb_st_ps2.u[1]&=(KB_U1_L_SHIFT|KB_U1_L_CTRL|KB_U1_L_ALT|KB_U1_R_SHIFT|KB_U1_R_CTRL|KB_U1_R_ALT);
	kb_st_ps2.u[2]=0;
	kb_st_ps2.u[3]=0;
	//memset(kb_st_ps2.u,0,sizeof(kb_st_ps2.u));
	data_joy=0;
	old_data_joy=0;
	rel_data_joy=0;
	joy_pressed=false;
	// if(WII_Init){
	// 	Wii_clear_old();
	// }
}

bool wait_kbdjoy_emu(void){
	process_input();
	if((joy_pressed)||(KBD_PRESS)){
		//printf("key pressed %d "BYTE_TO_BINARY_PATTERN"\n", joy_pressed, BYTE_TO_BINARY(data_joy));
		return true;
	}
	clear_input();
	//printf("no key pressed\n");
	return false;
}

bool wait_kbdjoy_menu(void){
	process_input();
	if((joy_pressed)||(KBD_PRESS)){
		////printf("key pressed\n");
		return true;
	}
	////printf("no key pressed\n");
	return false;
}
/*process input*/

/*-------Graphics--------*/
#define PALETTE_IDX_INC (50)
#define PALETTE_COL_DEC (48)

void set_def_palette(g_out video_mode){
	if(video_mode<g_out_TFT_ST7789){
		for(int i=0;i<16;i++){
			int I=(i>>3)&1;
			int G=(i>>2)&1;
			int R=(i>>1)&1;
			int B=(i>>0)&1;
			uint32_t RGB=((R?I?255:170:0)<<16)|((G?I?255:170:0)<<8)|((B?I?255:170:0)<<0);
			graphics_set_palette(i+PALETTE_SHIFT,RGB);
		}
	}
	if(video_mode>g_out_HDMI){
		for(int i=0;i<16;i++){
			int I=(i>>3)&1;
			int G=(i>>2)&1;
			int R=(i>>1)&1;
			int B=(i>>0)&1;
			//uint32_t RGB=((R?I?31:17:0)<<11)|((G?I?63:33:0)<<5)|((B?I?31:17:0)<<0);
			uint32_t RGB=((R?I?255:170:0)<<16)|((G?I?255:170:0)<<8)|((B?I?255:170:0)<<0);
			graphics_set_palette(i+PALETTE_SHIFT,RGB);
		}
	}
}

void updatePalette(const uint8_t *palette,uint16_t length) {
	graphics_set_palette(0, RGB888(0, 0, 0));
    for (uint8_t i = 0; i < (length/3); i++) {
        uint8_t R = palette[0];
        uint8_t G = palette[1];
        uint8_t B = palette[2];
		//printf("Update Pal: [%d]>[%d][%d][%d]\n",i,R,G,B);
        graphics_set_palette(i, RGB888(R, G, B));
		palette+=3;
    }
	//graphics_set_palette(255, 0);
}

#define MIN_VAL(X, Y) (((X) < (Y)) ? (X) : (Y))
#define MAX_VAL(X, Y) (((X) > (Y)) ? (X) : (Y))

void updatePaletteGame(const uint8_t *palette,uint16_t length) {
	uint8_t R_out = 0;
	uint8_t G_out = 0;
	uint8_t B_out = 0;

	graphics_set_palette(0, RGB888(0, 0, 0));
    for (uint8_t i = 0; i < (length/3); i++) {
        int R = palette[0];
        int G = palette[1];
        int B = palette[2];
		for (uint8_t j = 0; j < 4; j++) {
			if(j==1){
				R -= (12);
				G -= (12);
				B -= (12);
			} else {
				R -= (PALETTE_COL_DEC*j);
				G -= (PALETTE_COL_DEC*j);
				B -= (PALETTE_COL_DEC*j);
			}
			//if((R>=0)&&(R<256)){R_out=(uint8_t)R;} else {R_out=0;};
			//if((G>=0)&&(G<256)){G_out=(uint8_t)G;} else {G_out=0;};
			//if((B>=0)&&(B<256)){B_out=(uint8_t)B;} else {B_out=0;};
			R_out=(uint8_t)MAX_VAL(R,0);
			G_out=(uint8_t)MAX_VAL(G,0);
			B_out=(uint8_t)MAX_VAL(B,0);
			//printf("Update Pal: [%d]>[%d][%d][%d]>[%d][%d][%d]\n",(i+(PALETTE_IDX_INC*j)),R,G,B,R_out, G_out, B_out);
        	graphics_set_palette(i+(PALETTE_IDX_INC*j), RGB888(R_out, G_out, B_out));
		}
		palette+=3;
    }
	//graphics_set_palette(255, 0);
}


void draw_mur_logo(){
	//return;
	draw_text_len((PREVIEW_POS_X+(PREVIEW_WIDTH/2)-((9*FONT_W)/2)),70,"NO SIGNAL",COLOR_ITEXT,COLOR_TEXT,9);
	for(uint8_t y=0;y<83;y++){
		for(uint8_t x=0;x<84;x++){
			uint8_t pixel = mur_logo_cat[x+(y*84)];
			if(pixel<0xFF)
				draw_pixel((PREVIEW_POS_X+(PREVIEW_WIDTH/2)-(84/2))+x,90+y,PALETTE_SHIFT+pixel);
		}
	}
}

void draw_mur_logo_big(short int xPos,short int yPos,uint8_t help){ //x-155 y-60
	//return;
	//for(uint8_t y=0;y<110;y++){
	//draw_mur_logo_anim(xPos,yPos,0,0,0);
	//memcpy(graph_buf+LOGO_PIX_OFFSET,mur_logo,LOGO_PIX_SIZE);
	if(help==1){
	
		updatePalette(mur_logo_small_pal,LOGO_SMALL_PAL_SIZE);

		for(uint16_t y=0;y<=LOGO_SMALL_PIX_HEIGHT;y++){
			for(uint16_t x=0;x<=LOGO_SMALL_PIX_WIDTH;x++){
				uint8_t pixel = (uint8_t)mur_logo_small[x+(y*LOGO_SMALL_PIX_WIDTH)];
				draw_pixel(xPos+x,yPos+y,pixel);
			}
		}	
		draw_text_len((xPos+(LOGO_SMALL_PIX_WIDTH/2)-(FONT_W*7)),yPos+109+FONT_H*2,"   F1 - HELP   ",CL_BLUE,CL_WHITE,15);
		draw_text_len((xPos+(LOGO_SMALL_PIX_WIDTH/2)-(FONT_W*7)),yPos+109+FONT_H*3,"WIN,HOME-RETURN",CL_BLUE,CL_WHITE,15);	
		if(joy_connected){
			draw_text_len((xPos+(LOGO_SMALL_PIX_WIDTH/2)-(FONT_W*7)),yPos+109+FONT_H*4,"JOY[START]-EXIT",CL_BLUE,CL_WHITE,15);
		}

	} else 
	if(help==2){
		updatePalette(mur_logo_pal,LOGO_PAL_SIZE);

		for(uint16_t y=0;y<=LOGO_PIX_HEIGHT;y++){
			for(uint16_t x=0;x<=LOGO_PIX_WIDTH;x++){
				uint8_t pixel = (uint8_t)mur_logo[x+(y*LOGO_PIX_WIDTH)];
				draw_pixel(xPos+x,yPos+y,pixel); 
				//draw_pixel(x,y,pixel);
			}
		}			
		memset(temp_msg,0,sizeof(temp_msg));
		sprintf(temp_msg,"%-s %s",FW_VERSION,FW_AUTHOR);
		/*
		draw_text_len(xPos+FONT_W+2,yPos+102+FONT_H*2,temp_msg,CL_BLUE,CL_WHITE,15);
		memset(temp_msg, 0, sizeof(temp_msg));
		draw_text_len(xPos+FONT_W+2,yPos+102+FONT_H*4," WIN,HOME-MENU ",CL_BLUE,CL_WHITE,15);	
		draw_text_len(xPos+FONT_W+2,yPos+102+FONT_H*5,"   F1 - HELP   ",CL_BLUE,CL_WHITE,15);
		if(joy_connected){
			draw_text_len(xPos+FONT_W+2,yPos+102+FONT_H*6,"JOY[START]-MENU",CL_BLUE,CL_WHITE,15);
		}
		*/
		//(SCREEN_W-LOGO_PIX_WIDTH)/2)//((SCREEN_H-LOGO_PIX_HEIGHT)/2)
		draw_text_len(xPos+((LOGO_PIX_WIDTH/2)-((17*FONT_W)/2)),(yPos+LOGO_PIX_HEIGHT)-(FONT_H+1),temp_msg,CL_BLUE,CL_WHITE,17);
		memset(temp_msg, 0, sizeof(temp_msg));
		draw_text_len(xPos+((LOGO_PIX_WIDTH/2)-((15*FONT_W)/2)),yPos+LOGO_PIX_HEIGHT+FONT_H*1," WIN,HOME-MENU ",CL_BLUE,CL_WHITE,15);	
		draw_text_len(xPos+((LOGO_PIX_WIDTH/2)-((15*FONT_W)/2)),yPos+LOGO_PIX_HEIGHT+FONT_H*2,"   F1 - HELP   ",CL_BLUE,CL_WHITE,15);
		if(joy_connected){
			draw_text_len(xPos+((LOGO_PIX_WIDTH/2)-((15*FONT_W)/2)),yPos+LOGO_PIX_HEIGHT+FONT_H*3,"JOY[START]-MENU",CL_BLUE,CL_WHITE,15);
		}
		
	}
	
}

void draw_help_text(short int startLine,uint8_t lang){
	//printf("[%08X][%08X][%08X]\n",help_text,help_text_rus,&help_text[0]);
	for(uint8_t y=0;y<SCREEN_HELP_LINES;y++){
		memset(temp_msg,0,sizeof(temp_msg));
		////printf("%s\n",help_text[y]);
		if((startLine+y)>HELP_LINES){
			//draw_text_len(8,20+(y*FONT_H),"									 ",CL_BLACK,CL_WHITE,37);
			draw_text5x7_len(9,20+(y*FONT_5x7_H),"														   ",CL_BLACK,CL_WHITE,59);
			continue;
		} else {
			if (conv_utf_cp866(help_text[lang][startLine+y], temp_msg, strlen(help_text[lang][startLine+y]))>0){
				draw_text5x7_len(9,20+(y*FONT_5x7_H),temp_msg,CL_BLACK,CL_WHITE,59);
			}
		}
		////printf("%s\n",temp_msg);
		//memset(temp_msg, 0, sizeof(temp_msg));
	}
}

void MessageBox(char *header,char *message,uint8_t colorFG,uint8_t colorBG,uint8_t delay){
	if(menu_mode[menu_ptr]==EMULATION){
		//zx_machine_enable_vbuf(false);
		busy_wait_ms(20);
	}	
	uint8_t max_len= strlen(header)>strlen(message)?strlen(header):strlen(message);
	if(max_len<10){max_len=10;}
	uint8_t left_x =(SCREEN_W/2)-((max_len/2)*FONT_W);
	uint8_t left_y = strlen(message)==0 ? (SCREEN_H/2)-(FONT_H/2):(SCREEN_H/2)-FONT_H;
	uint8_t height = FONT_H*2; //uint8_t height = strlen(message)>0 ? FONT_H*2+5:FONT_H+5;

	draw_rect(left_x-1,left_y,(max_len*FONT_W)+2,height+1,COLOR_MAIN_RAMK,true);//Основная рамка
	draw_rect(left_x,left_y+FONT_H,(max_len*FONT_W),FONT_H,colorBG,true);//Фон главного окна
	//draw_stripes(left_x+((max_len-5)*FONT_W),left_y);

	if (strlen(message)>0){
		draw_text(left_x,left_y,header,colorFG,CL_EMPTY);
		draw_text(left_x,left_y+FONT_H,message,colorFG,CL_EMPTY);
	} else {
		draw_text(left_x,left_y+FONT_H,header,colorFG,CL_EMPTY);
	}
	////printf("X:%d,Y:%d\n",left_x,left_y);
	switch (delay){
		case 1:
			busy_wait_ms(3000);
			break;
		case 2:
			busy_wait_ms(750);
			break;
		case 3:
			busy_wait_ms(250);
			break;
		case 4:
			busy_wait_ms(1000);
			break;
		case 0xFF:
			while(!wait_kbdjoy_menu()){
				busy_wait_ms(30);	
			}
			break;			
		default:
		break;
	}
	//if(menu_mode[menu_ptr]==EMULATION) zx_machine_enable_vbuf(true);
}

uint8_t DialogBox(char *header,char *message,uint8_t colorFG,uint8_t colorBG,uint8_t di_id){
	volatile uint8_t max_len = strlen(header)>strlen(message)?strlen(header):strlen(message);
	if(max_len<10){max_len=10;}
	volatile uint8_t left_x =(SCREEN_W/2)-((max_len/2)*FONT_W);
	volatile uint8_t left_y = strlen(message)==0 ? (SCREEN_H/2)-(FONT_H/2):(SCREEN_H/2)-FONT_H;
	volatile uint8_t height = FONT_H*3; //uint8_t height = strlen(message)>0 ? FONT_H*2+5:FONT_H+5;

	draw_rect(left_x-1,left_y,(max_len*FONT_W)+2,height+1,COLOR_MAIN_RAMK,true);//Основная рамка
	draw_rect(left_x,left_y+FONT_H,(max_len*FONT_W),FONT_H*2,colorBG,true);//Фон главного окна
	//draw_stripes(left_x+((max_len-5)*FONT_W),left_y);

	if (strlen(message)>0){
		draw_text(left_x,left_y,header,colorFG,CL_EMPTY);
		draw_text(left_x,left_y+FONT_H,message,colorFG,CL_EMPTY);
	} else {
		draw_text(left_x,left_y+FONT_H,header,colorFG,CL_EMPTY);
	}
	////printf("X:%d,Y:%d\n",left_x,left_y);
	if(menu_mode[menu_ptr]==EMULATION){
		//zx_machine_enable_vbuf(false);
		busy_wait_ms(20);
	}
	/*
#define DLG_RES_NONE	0x00
#define DLG_RES_CANCEL	0xFF
#define DLG_RES_OK		0x01
#define DLG_RES_RET		0x02
#define DLG_RES_ABR		0x03	
	*/
	short int dia_pos=0;
	uint8_t dia_res=0;
	need_redraw=true;
	while(true){
		process_input();
		busy_wait_us(500);
		if((KBD_LEFT)||(data_joy==D_JOY_LEFT)){
			dia_pos--;
			if(dia_pos<0){dia_pos=2;}
			while(strlen(iface_btn[di_id][dia_pos])<2){
				if(dia_pos<0){dia_pos=2;break;}
				dia_pos--;				
			}
			busy_wait_ms(150);
			need_redraw=true;
		}
		if((KBD_RIGHT)||(data_joy==D_JOY_RIGHT)){
			dia_pos++;
			while(strlen(iface_btn[di_id][dia_pos])<2){
				if(dia_pos>2){dia_pos=0;break;};
				dia_pos++;
			}
			if(dia_pos>2){dia_pos=0;};
			busy_wait_ms(150);
			need_redraw=true;
		}
		
		if((KBD_ENTER)||(data_joy==D_JOY_A)){
			dia_res = iface_res[di_id][dia_pos];
			busy_wait_ms(150);
			break;
		}
		if((KBD_ESC)||(data_joy&D_JOY_START)){
			dia_res=DLG_RES_NONE;
			need_redraw=true;
			busy_wait_ms(150);
			break;
		}
		if(need_redraw){
			short int pos = left_x+(max_len*FONT_W);
			short int max_btn = 2;
			do{
				if(strlen(iface_btn[di_id][max_btn])>1){
					pos-=strlen(iface_btn[di_id][max_btn])*FONT_W;
					memset(temp_msg,0,sizeof(temp_msg));
					strcpy(temp_msg,iface_btn[di_id][max_btn]);
					if(dia_pos==max_btn){
						temp_msg[0]=0x10;
						temp_msg[strlen(iface_btn[di_id][max_btn])-1]=0x11;
					}					
					draw_text(pos,left_y+(FONT_H*2),temp_msg,COLOR_TEXT,dia_pos==max_btn?COLOR_CURRENT_BG:COLOR_BACKGOUND);
				}
				max_btn--;
			} while(max_btn>=0);
			need_redraw=false;
		}
	}
	//if(menu_mode[menu_ptr]==EMULATION) zx_machine_enable_vbuf(true);
	clear_input();
	return dia_res;
}

uint8_t EditDialogBox(char *header,char *message,char *value,uint8_t colorFG,uint8_t colorBG,uint8_t di_id,bool in_type){
	volatile uint8_t max_len = strlen(header)>strlen(message)?strlen(header):strlen(message);
	if(max_len<10){max_len=10;}
	volatile uint8_t left_x =(SCREEN_W/2)-((max_len/2)*FONT_W);
	volatile uint8_t left_y = strlen(message)==0 ? (SCREEN_H/2)-(FONT_H/2):(SCREEN_H/2)-FONT_H;
	volatile uint8_t height = FONT_H*4; //uint8_t height = strlen(message)>0 ? FONT_H*2+5:FONT_H+5;

	draw_rect(left_x-1,left_y,(max_len*FONT_W)+2,height+1,COLOR_MAIN_RAMK,true);//Основная рамка
	draw_rect(left_x,left_y+FONT_H,(max_len*FONT_W),FONT_H*2,colorBG,true);//Фон главного окна
	//draw_stripes(left_x+((max_len-5)*FONT_W),left_y);

	volatile uint8_t value_len = strlen(value);
	volatile short int max_pos = strlen(value)+3;
	if (strlen(message)>0){
		draw_text(left_x,left_y,header,colorFG^0x0F,CL_EMPTY);
		draw_text(left_x,left_y+FONT_H,message,colorFG,CL_EMPTY);
		draw_text_len(left_x,left_y+(FONT_H*2),value,colorFG,CL_GRAY,value_len);
	} else {
		draw_text(left_x,left_y+FONT_H,header,colorFG^0x0F,CL_EMPTY);
		draw_text_len(left_x,left_y+(FONT_H*2),value,colorFG,CL_GRAY,value_len);
	}
	////printf("X:%d,Y:%d\n",left_x,left_y);
	if(menu_mode[menu_ptr]==EMULATION){
		//zx_machine_enable_vbuf(false);
		busy_wait_ms(20);
	}
	/*
#define DLG_RES_NONE	0x00
#define DLG_RES_CANCEL	0xFF
#define DLG_RES_OK		0x01
#define DLG_RES_RET		0x02
#define DLG_RES_ABR		0x03	
	*/
	short int dia_pos=0;
	uint8_t dia_res=0;
	uint8_t temp=0;
	need_redraw=true;
	while(true){
		process_input();
		busy_wait_us(500);
		if((KBD_LEFT)||(data_joy==D_JOY_LEFT)){
			dia_pos--;
			if(dia_pos<0){dia_pos=max_pos;}
			if(dia_pos>value_len){
				while(iface_res[di_id][dia_pos-value_len]<DLG_RES_OK){
					dia_pos--;
					//printf("dia_pos1:%04d\n",dia_pos);
					if(dia_pos<0){dia_pos=max_pos;break;}
				}
			}
			busy_wait_ms(150);
			need_redraw=true;
		}
		if((KBD_RIGHT)||(data_joy==D_JOY_RIGHT)){
			dia_pos++;
			if(dia_pos>value_len){
				while(iface_res[di_id][dia_pos-value_len]<DLG_RES_OK){
					dia_pos++;
					//printf("dia_pos2:%04d\n",dia_pos);
					if(dia_pos>max_pos){dia_pos=0;break;};
				}
			}
			if(dia_pos>max_pos){dia_pos=0;};
			busy_wait_ms(150);
			need_redraw=true;
		}
		if(dia_pos<value_len){
			if((KBD_UP)||(data_joy==D_JOY_UP)){
				value[dia_pos]++;
				if(value[dia_pos]>0x39) value[dia_pos]=0x30;
				busy_wait_ms(150);
				need_redraw=true;
			}
			if((KBD_DOWN)||(data_joy==D_JOY_DOWN)){
				value[dia_pos]--;
				if(value[dia_pos]<0x30) value[dia_pos]=0x39;
				busy_wait_ms(150);
				need_redraw=true;
			}
			if(KBD_PRESS){
				uint8_t chr = convert_kb_u_to_char(kb_st_ps2,in_type);
				if(chr>0){				
					value[dia_pos]=chr;
					dia_pos++;
					busy_wait_ms(150);
					need_redraw=true;
				}
			}

		}
		
		if((KBD_ENTER)||(data_joy==D_JOY_A)){
			if(dia_pos>=value_len){
				dia_res = iface_res[di_id][dia_pos-value_len];
				//printf("dia_res:%02X\n",dia_res);
				busy_wait_ms(150);
				break;
			}
		}
		if((KBD_ESC)||(data_joy&D_JOY_START)){
			dia_res=DLG_RES_NONE;
			need_redraw=true;
			busy_wait_ms(150);
			break;
		}
		if(need_redraw){
			short int pos = left_x+(max_len*FONT_W);
			short int max_btn = 2;
			do{
				if(strlen(iface_btn[di_id][max_btn])>1){
					pos-=strlen(iface_btn[di_id][max_btn])*FONT_W;
					draw_text(pos,left_y+(FONT_H*3),iface_btn[di_id][max_btn],COLOR_TEXT,(dia_pos-value_len)==max_btn?COLOR_CURRENT_BG:COLOR_BACKGOUND);
				}
				max_btn--;
			} while(max_btn>=0);
			draw_text_len(left_x,left_y+(FONT_H*2),value,colorFG,CL_GRAY,value_len);
			if(dia_pos<=value_len){
				draw_text_len(left_x+(FONT_W*dia_pos),left_y+(FONT_H*2),&value[dia_pos],colorFG,COLOR_CURRENT_BG,1);
			}
			need_redraw=false;
			//printf("dia_pos:%03d\n",dia_pos);
		}
	}
	//if(menu_mode[menu_ptr]==EMULATION) zx_machine_enable_vbuf(true);
	clear_input();
	return dia_res;
}


void draw_main_window(){
	draw_rect(0,0,SCREEN_W,SCREEN_H,COLOR_FULLSCREEN,true);//Заливаем экран //
	draw_rect(FONT_W-1,FONT_H,(SCREEN_W-(FONT_W*2))+2,(SCREEN_H-(FONT_H*2))+1,COLOR_MAIN_RAMK,true);//Основная рамка
	draw_rect(FONT_H,(FONT_W*2),SCREEN_W-(FONT_W*2),SCREEN_H-(FONT_H*3),COLOR_BACKGOUND,true);//Фон главного окна
	//draw_logo_header(((SCREEN_W-FONT_W)-SPEC_LOGO_W)-(10*FONT_W),FONT_H);
	memset(temp_msg,0,sizeof(temp_msg));
	sprintf(temp_msg,"%s %s",FW_VERSION,FW_AUTHOR);
	draw_text_len(FONT_W,FONT_H,temp_msg,COLOR_BACKGOUND,CL_EMPTY,20);
	//memset(temp_msg, 0, sizeof(temp_msg));
}

void draw_file_window(){
	draw_rect(PREVIEW_POS_X,SCREEN_H-(FONT_H*4),PREVIEW_WIDTH,(FONT_H*3),COLOR_BACKGOUND,true);//Фон отображения информации о файле //COLOR_BACKGOUND
	draw_rect(PREVIEW_POS_X-FONT_W,PREVIEW_POS_Y-1,FONT_W,SCREEN_H-(FONT_H*3)+2,COLOR_MAIN_RAMK,false);  //Рамка полосы прокрутки //COLOR_BORDER
	
	//draw_rect(7,SCREEN_H-FONT_H-8,9+FONT_W*FILE_NAME_LEN,FONT_H+1,COLOR_BORDER,false);//панель подсказок под файлами
	//draw_text_len(8,SCREEN_H-FONT_H-7,"F1-HLP,HOME-RET",COLOR_TEXT,COLOR_BACKGOUND,15);//get_file_from_dir("0:/z80",i)
	
	//*draw_rect(PREVIEW_POS_X,PREVIEW_POS_Y,PREVIEW_WIDTH,SCREEN_H-25,COLOR_BACKGOUND,true);  
	//draw_rect(PREVIEW_POS_X+11,16+11,160,120,0x0,true);
	//*draw_rect(PREVIEW_POS_X,PREVIEW_POS_Y,PREVIEW_WIDTH,192,COLOR_BACKGOUND,true);
	//*draw_line(PREVIEW_POS_X,209,17+FONT_W*37,209,COLOR_PIC_BG);
	
}

void draw_fast_menu(uint8_t xPos,uint8_t yPos,bool drawbg,uint8_t menu,uint8_t active){
	////printf("xPos:%d  yPos:%d  width:%d  height:%d\n",xPos,yPos,width,height);
	//draw_rect(0,0,SCREEN_W,SCREEN_H,COLOR_FULLSCREEN,true);//Заливаем экран 
	uint8_t lines = 0;
	for(uint8_t i=0;i<FAST_MENU_LINES;i++){
		if(*fast_menu[menu][i]==0){
			lines=i;
			break;
		}
	}
	uint8_t width =(16*FONT_W)+2;
	uint8_t height =(lines*FONT_H)+FONT_H+1;
	//printf("xPos:%d  yPos:%d  width:%d  height:%d lines:%d\n",xPos,yPos,width,height,lines);
	if(drawbg){
		draw_rect(xPos-1,yPos,width,height,COLOR_MAIN_RAMK,true);//Основная рамка
		draw_rect(xPos,yPos+FONT_H,width-2,height-9,COLOR_BACKGOUND,true);//Фон главного окна
		draw_logo_header(xPos+2,yPos);
	}
	for(uint8_t y=0;y<lines;y++){
		//memset(temp_msg,0,sizeof(temp_msg));
		////printf("%s\n",help_text[y]);
		/*if((startLine+y)>HELP_LINES){
			draw_text_len(8,20+(y*8),"									 ",CL_BLACK,CL_WHITE,37);
			continue;
		}*/
		/*if (convert_utf8_to_windows1251(help_text[startLine+y], temp_msg, strlen(help_text[startLine+y]))>0){
			
		};*/
		if((menu==0)&&(!init_fs)&&((y==0))){ //||(y==2)||(y==3)||(y==4)
			draw_text_len(xPos,(yPos+FONT_H)+(y*FONT_H),fast_menu[menu][y],COLOR_DTEXT,y==active?COLOR_CURRENT_BG:COLOR_BACKGOUND,16);	
		} else{
			draw_text_len(xPos,(yPos+FONT_H)+(y*FONT_H),fast_menu[menu][y],COLOR_TEXT,y==active?COLOR_CURRENT_BG:COLOR_BACKGOUND,16);
		}
	}
	//memset(temp_msg, 0, sizeof(temp_msg));
}

void draw_config_menu(uint8_t xPos,uint8_t yPos,bool drawbg,uint8_t active){
	uint8_t lines = 0;
	for(uint8_t i=0;i<CONFIG_MENU_ITEMS;i++){
		if(*config_menu[i]==0){
			lines=i;
			break;
		}
	}
	uint8_t width =(27*FONT_W)+2;
	uint8_t height =(lines*FONT_H)+FONT_H+1;
	//printf("xPos:%d  yPos:%d  width:%d  height:%d lines:%d\n",xPos,yPos,width,height,lines);
	if(drawbg){
		draw_rect(xPos-1,yPos,width,height,COLOR_MAIN_RAMK,true);//Основная рамка
		draw_rect(xPos,yPos+FONT_H,width-2,height-9,COLOR_BACKGOUND,true);//Фон главного окна
		draw_logo_header((xPos+width)-(16*FONT_W)+(FONT_W-2),yPos);
		draw_text_len(xPos,yPos,"[SETTINGS]",COLOR_ITEXT,CL_EMPTY,10);
		
	} 		
	for(uint8_t y=0;y<lines;y++){
		#ifdef VGA_HDMI
		if((g_out)cfg_video_out<g_out_TFT_ST7789){
			if((y>16)&&(y<(settings_lines-4))){
				draw_text_len(xPos,(yPos+FONT_H)+(y*FONT_H),config_menu[y],COLOR_DTEXT,y==active?COLOR_CURRENT_BG:COLOR_BACKGOUND,27);
				continue;
			}
		}
		#endif
		if((!init_fs)&&((y==settings_lines-4)||(y==settings_lines-3)||(y==settings_lines-2))){
			draw_text_len(xPos,(yPos+FONT_H)+(y*FONT_H),config_menu[y],COLOR_DTEXT,y==active?COLOR_CURRENT_BG:COLOR_BACKGOUND,27);
		} else {
			draw_text_len(xPos,(yPos+FONT_H)+(y*FONT_H),config_menu[y],COLOR_TEXT,y==active?COLOR_CURRENT_BG:COLOR_BACKGOUND,27);			
		}
		if(y==0){
			draw_text_len(xPos+(14*FONT_W),(yPos+FONT_H)+(y*FONT_H),boot_scr_config[cfg_boot_scr],COLOR_TEXT,y==active?COLOR_CURRENT_BG:COLOR_BACKGOUND,12);
		}	
		if(y==1){
			draw_text_len(xPos+(22*FONT_W),(yPos+FONT_H)+(y*FONT_H),HUD_config[cfg_hud_enable],COLOR_TEXT,y==active?COLOR_CURRENT_BG:COLOR_BACKGOUND,4);
		}		
		if(y==2){
			draw_text_len(xPos+(23*FONT_W),(yPos+FONT_H)+(y*FONT_H),sound_out_config[cfg_sound_out_mode],COLOR_TEXT,y==active?COLOR_CURRENT_BG:COLOR_BACKGOUND,3);
		}
		if(y==3){
			draw_text_len(xPos+(16*FONT_W),(yPos+FONT_H)+(y*FONT_H),gaudge[(cfg_volume/25)],COLOR_TEXT,y==active?COLOR_CURRENT_BG:COLOR_BACKGOUND,10);
		}
		if(y==4){
			draw_text_len(xPos+(16*FONT_W),(yPos+FONT_H)+(y*FONT_H),video_out_config[cfg_video_out],COLOR_TEXT,y==active?COLOR_CURRENT_BG:COLOR_BACKGOUND,10);
		}
		if(y==5){
			if(graphics_try_framerate((g_out)cfg_video_out,(fr_rate)cfg_frame_rate,false)){
				draw_text_len(xPos+(20*FONT_W),(yPos+FONT_H)+(y*FONT_H),video_freq_config[cfg_frame_rate],COLOR_TEXT,y==active?COLOR_CURRENT_BG:COLOR_BACKGOUND,6);
			} else {
				draw_text_len(xPos+(20*FONT_W),(yPos+FONT_H)+(y*FONT_H),video_freq_config[cfg_frame_rate],COLOR_DTEXT,y==active?COLOR_CURRENT_BG:COLOR_BACKGOUND,6);
			}
		}
		if(y==6){
			draw_text_len(xPos+(23*FONT_W),(yPos+FONT_H)+(y*FONT_H),yes_no[cfg_mobile_mode],COLOR_TEXT,y==active?COLOR_CURRENT_BG:COLOR_BACKGOUND,3);
		}		
		#ifdef VGA_HDMI
		if((g_out)cfg_video_out>g_out_HDMI){
			if(y==7){
				draw_text_len(xPos+(16*FONT_W),(yPos+FONT_H)+(y*FONT_H),gaudge[cfg_brightness],COLOR_TEXT,y==active?COLOR_CURRENT_BG:COLOR_BACKGOUND,10);
			}
			if(y==8){
				draw_text_len(xPos+(16*FONT_W),(yPos+FONT_H)+(y*FONT_H),video_out_rotate[cfg_rotate],COLOR_TEXT,y==active?COLOR_CURRENT_BG:COLOR_BACKGOUND,10);
			}
			if(y==9){
				draw_text_len(xPos+(16*FONT_W),(yPos+FONT_H)+(y*FONT_H),video_out_inversion[cfg_inversion],COLOR_TEXT,y==active?COLOR_CURRENT_BG:COLOR_BACKGOUND,10);
			}		
			if(y==10){
				draw_text_len(xPos+(21*FONT_W),(yPos+FONT_H)+(y*FONT_H),video_out_pixels[cfg_pixels],COLOR_TEXT,y==active?COLOR_CURRENT_BG:COLOR_BACKGOUND,5);
			}		
		}
		#endif
	}
	////printf("xPos:%d  yPos:%d  width:%d  height:%d\n",xPos,yPos,width,height);
	//draw_rect(0,0,SCREEN_W,SCREEN_H,COLOR_FULLSCREEN,true);//Заливаем экран 
	//memset(temp_msg, 0, sizeof(temp_msg));
}

bool LoadTxt(char *file_name){
#ifndef DEBUG_DISABLE_LOADERS	
	short int res =0;
	size_t bytesRead;
	size_t bytesToRead;
	size_t FileSize;

	memset(temp_buffer_y, 0, TEMP_BUFF_SIZE_Y);

	res = sd_open_file(&sd_file,file_name,FA_READ);
	////printf("sd_open_file=%d\n",res);
	if (res!=FR_OK){sd_close_file(&sd_file);return false;}
   	FileSize = sd_file_size(&sd_file);
	printf("text Filesize %u bytes\n", FileSize);
	
	/*uint16_t ptr=0;
	do{
		//printf("[%04X]",ptr);
		for (uint8_t col=0;col<16;col++){
			//printf("\t%02X",sd_buffer[ptr]);
			ptr++;
		}
		//printf("\n");
	} while(ptr<sizeof(sd_buffer));
	//printf("\n");*/


	memset(temp_msg, 0, sizeof(temp_msg));
	sprintf(temp_msg,"File size:%dk",(short int)(FileSize/1024));
	draw_text_len(18+FONT_W*FILE_NAME_LEN,216, temp_msg,COLOR_TEXT,COLOR_BACKGOUND,22);

	res = sd_read_file(&sd_file,temp_buffer_y,TEMP_BUFF_SIZE_Y-5,&bytesRead);
	if (res!=FR_OK){sd_close_file(&sd_file);return false;}
	draw_text_len(18+FONT_W*FILE_NAME_LEN,16,"File contents:",COLOR_TEXT,COLOR_BACKGOUND,14);
	uint16_t ptr=0;
	uint8_t len=0;
	for (uint8_t i = 0; i < (PREVIEW_HEIGHT/FONT_5x7_H); i++){
		memset(temp_msg, 0, sizeof(temp_msg));
		for (uint8_t j = 0; j < (PREVIEW_WIDTH/FONT_5x7_W); j++){
			if((temp_buffer_y[ptr+j]==0x0A)){
				len=j+1;
				break;
			}
			if(temp_buffer_y[ptr+j]==0x00){
				len=j;	
				break;
			}
			len=j;
		}
		//printf("ptr:%d len:%d \n",ptr,len);
		memcpy(temp_msg,&temp_buffer_y[ptr],len);
		ptr+=len;
		if(len>0){
			draw_text5x7_len(PREVIEW_POS_X,25+(FONT_5x7_H)*i,temp_msg,COLOR_TEXT,COLOR_BACKGOUND,(PREVIEW_WIDTH/FONT_5x7_W));
		}
		if(len==0){
			uint8_t k=0;
			while((temp_buffer_y[ptr+k]!=0x0A)||(temp_buffer_y[ptr+k]!=0x00)){
				k++;
			}
			ptr+=k;
		}
		if(ptr>=TEMP_BUFF_SIZE_Y){break;}
		if(ptr>=FileSize){break;}
	}
	sd_close_file(&sd_file);
	return true;
#endif
}

/*-------Graphics--------*/
/*-------AY inits--------*/


repeating_timer_t soft_sound_timer;
bool sound_timer_enabled;

static uint8_t beep_data;
static uint8_t beep_data_old;
static bool bepper_out = false;

static bool ldout = false;

static int outL=0;
static int outR=0; 

static int outL_old=0;  
static int outR_old=0; 

static int beeper_signed=0;
static int outL_signed=0;
static int outR_signed=0;

static long mixL=0;
static long mixR=0;

/*
void FAST_FUNC(hw_zx_set_beep_out)(uint8_t val){

	beep_data_old=beep_data;
	if(cfg_sound_mode>0){
		beep_data=val>>3;
		//printf(BYTE_TO_BINARY_PATTERN" - "BYTE_TO_BINARY_PATTERN"\n",BYTE_TO_BINARY(beep_data),BYTE_TO_BINARY(val));
	} else {
		beep_data=0;
	}
	bepper_out^=(beep_data==beep_data_old)?false:true;
	if(cfg_sound_mode == 4){
		AY_to595Beep(bepper_out);
	} else {
		if(cfg_sound_out_mode==OUT_PWM){
			pwm_set_gpio_level(ZX_BEEP_PIN,(((bepper_out*255)*(cfg_volume))/100));
		}
		if(cfg_sound_out_mode==OUT_PCM){
			//if(beep_data!=beep_data_old) gpio_put(WORK_LED_PIN,1);
			if((beep_data!=0)||(beep_data_old!=0)){
				mixL = 1;
				mixR = 1;
				beeper_signed = (bool)bepper_out?128:0;
				mixL = (2*(mixL+beeper_signed))-((mixL*beeper_signed)/128)-128;
				mixR = (2*(mixR+beeper_signed))-((mixR*beeper_signed)/128)-128;
				i2s_out(((int)(mixL)*4)*(cfg_volume/8),((int)(mixR)*4)*(cfg_volume/8));
			}
		}
	};
/
}
*/

volatile uint16_t audio_buffer_idx =0;

//#pragma GCC push_options
//#pragma GCC optimize("-Ofast")
//bool __scratch_y("sound_ayemu") AY_timer_callback(repeating_timer_t *rt)
bool FAST_FUNC(Sound_timer_callback)(repeating_timer_t *rt){
	
	outL_old=outL;
	outR_old=outR;
	outL=(gw_audio_buffer[audio_buffer_idx] << 5);
	outR=(gw_audio_buffer[audio_buffer_idx+1] << 5);
	if(cfg_sound_out_mode==OUT_PWM){
		pwm_set_gpio_level(ZX_AY_PWM_PIN0,(uint8_t)((outR*cfg_volume)/100)); // Право
		pwm_set_gpio_level(ZX_AY_PWM_PIN1,(uint8_t)((outL*cfg_volume)/100)); // Лево
	}
	if(cfg_sound_out_mode==OUT_PCM){
		mixL=1;
		mixR=1;
		if((outL!=outL_old)||(outR!=outR_old)){
			mixL = (2*(mixL+outL))-((mixL*outL)/128)-128;
			mixR = (2*(mixR+outR))-((mixR*outR)/128)-128;
			i2s_out((int)(((int)(mixR)*4)*(cfg_volume/CFG_VOLUME_STEP)),(int)(((int)(mixL)*4)*(cfg_volume/CFG_VOLUME_STEP)));
		}
	}
	if(audio_buffer_idx<(GW_AUDIO_BUFFER_LENGTH)){
		audio_buffer_idx++;
	} else {
		audio_buffer_idx=0;
		gw_audio_buffer_copied = true;
	}

	return true;
}


void PWM_init_pin(uint pinN){
	gpio_set_function(pinN, GPIO_FUNC_PWM);
	uint slice_num = pwm_gpio_to_slice_num(pinN);
	
	pwm_config c_pwm=pwm_get_default_config();
	pwm_config_set_clkdiv(&c_pwm,1.0);
	pwm_config_set_wrap(&c_pwm,255);//MAX PWM value
	pwm_init(slice_num,&c_pwm,true);
}

static void PWM_Deinit_pin(uint pinN){
	if(gpio_get_function(pinN)==GPIO_FUNC_PWM){	
		uint slice_num = pwm_gpio_to_slice_num(pinN);
		////printf("ay deslice_num:%u\n",slice_num);
		pwm_set_enabled (slice_num, false);
		pwm_config c_pwm=pwm_get_default_config();
		pwm_init(slice_num,&c_pwm,false);
		gpio_deinit(pinN);
	}
}


bool Init_Sound(){
	if(!sound_timer_enabled){
		//printf("Init Soft_AY\n");
		sound_timer_enabled=true;
		if(cfg_sound_out_mode==OUT_PWM){
			printf("Set OUT PWM\n");
			PWM_init_pin(ZX_AY_PWM_PIN0);
			PWM_init_pin(ZX_AY_PWM_PIN1);
			PWM_init_pin(ZX_BEEP_PIN);
		}
		if(cfg_sound_out_mode==OUT_PCM){
			printf("Set OUT I2S\n");
			gpio_init(ZX_AY_PWM_PIN0);
			gpio_init(ZX_AY_PWM_PIN1);
			gpio_init(ZX_BEEP_PIN);
			gpio_set_dir(ZX_AY_PWM_PIN0,GPIO_OUT);
			gpio_set_dir(ZX_AY_PWM_PIN1,GPIO_OUT);
			gpio_set_dir(ZX_BEEP_PIN,GPIO_OUT);
			i2s_init();
		}
	
		short int rate = SOUND_SAMPLE_RATE;
		if (!add_repeating_timer_us(-1000000 / rate, Sound_timer_callback, NULL, &soft_sound_timer)) {
			////printf("Failed to add timer\n");
			return false;
		}
		////printf("Init TIMER\n");
	}
	return true;
}

void Deinit_Sound(){
	if(sound_timer_enabled){
		////printf("DeInit Soft_AY\n");
		cancel_repeating_timer(&soft_sound_timer);
		if(cfg_sound_out_mode==OUT_PWM){
			PWM_Deinit_pin(ZX_AY_PWM_PIN0);
			PWM_Deinit_pin(ZX_AY_PWM_PIN1);
			PWM_Deinit_pin(ZX_BEEP_PIN);
		}
		//gpio_deinit(TST_PIN);
		if(cfg_sound_out_mode==OUT_PCM){
			i2s_deinit();
		}		
		sound_timer_enabled=false;
	}
}

uint32_t fire_millis(){
	return us_to_ms(time_us_32());
}

static uint32_t autofireX=0;
static uint32_t autofireY=0;
static uint32_t autofire=0;
static bool autofire_flipX=false;
static bool autofire_flipY=false;

/*
bool FAST_FUNC(emulation_timer_callback)(repeating_timer_t *rt){
	if(emulation) gw_system_run(4);
	return true;
}
*/



/*
void load_z80_file(char* file_name,short int slot){
	emu_paused = true;
	im_ready_loading = false;
	while (emu_paused){ //Quick Save
		busy_wait_ms(10);
		if (im_ready_loading){
			zx_machine_reset(false);
			AY_reset(cfg_sound_mode);// сбросить AY
			memset(temp_msg,0,sizeof(temp_msg));
			if(slot>0){
				sprintf(temp_msg," Loading slot# %d ",slot);
			} else {
				sprintf(temp_msg," Loading QUICKSAVE ");
			}
			if (load_image_z80(file_name)){
				MessageBox("LOAD",temp_msg,CL_WHITE,CL_BLUE,2);
			} else {
				MessageBox("Error!!!",temp_msg,CL_YELLOW,CL_LT_RED,1);
			}
			//AY_reset();// сбросить AY
			clear_input();
			emu_paused = false;
			im_ready_loading = false;
			break;
		}
	}
}
*/

void input_init(){
	printf("Joysticks Init:\n");

	active_type_joystick = joy_start();            // инициализация джойстиков	
	if(Joystics.joy_connected) {
		joy_connected = true;
		if(Joystics.Present_i2c_PCF_NES_joy1){printf("PCF_NES_joy1\t");}
		if(Joystics.Present_i2c_PCF_NES_joy2){printf("PCF_NES_joy2\t");}
		if(Joystics.Present_NES_joy1){printf("NES_joy1\t");}
		if(Joystics.Present_NES_joy2){printf("NES_joy2\t");}
		printf(" Connected \n");
	}else{
		joy_connected = false;
	}
	
	if (i2c_kbd_start()){i2cKbdMode = true;}else{i2cKbdMode = false;}

	short int i2c_state = i2c_kbd_data_in();
	//printf ("i2c_state: %d\n",i2c_state);

	if(!i2cKbdMode) {
		//printf ("i2c Keyboard not connected\n");
		i2c_kbd_deinit();	
		busy_wait_ms(100);
		start_PS2_capture();
		printf ("PS/2 Keyboard Started\n");
	} else {
		printf ("I2C Keyboard Started\n");
	}	
}

uint8_t hat_switch_process(uint16_t data_joy){
	uint8_t result=0;
	if(!kbd_lock){
		if((data_joy&D_JOY_START)&&(data_joy&D_JOY_UP)){result|=HAT_UP;result|=HAT_START;}
		if((data_joy&D_JOY_START)&&(data_joy&D_JOY_DOWN)){result|=HAT_DOWN;result|=HAT_START;}
		if((data_joy&D_JOY_START)&&(data_joy&D_JOY_LEFT)){result|=HAT_LEFT;result|=HAT_START;}
		if((data_joy&D_JOY_START)&&(data_joy&D_JOY_RIGHT)){result|=HAT_RIGHT;result|=HAT_START;}
		if((data_joy&D_JOY_START)&&(data_joy&D_JOY_A)){result|=HAT_A;result|=HAT_START;}
		if((data_joy&D_JOY_START)&&(data_joy&D_JOY_B)){result|=HAT_B;result|=HAT_START;}
		if((data_joy&D_JOY_SELECT)&&(data_joy&D_JOY_UP)){result|=HAT_UP;result|=HAT_SELECT;}
		if((data_joy&D_JOY_SELECT)&&(data_joy&D_JOY_DOWN)){result|=HAT_DOWN;result|=HAT_SELECT;}
		if((data_joy&D_JOY_SELECT)&&(data_joy&D_JOY_LEFT)){result|=HAT_LEFT;result|=HAT_SELECT;}
		if((data_joy&D_JOY_SELECT)&&(data_joy&D_JOY_RIGHT)){result|=HAT_RIGHT;result|=HAT_SELECT;}
	}
	if((data_joy&D_JOY_SELECT)&&(data_joy&D_JOY_A)){result|=HAT_A;result|=HAT_SELECT;}
	if((data_joy&D_JOY_SELECT)&&(data_joy&D_JOY_B)){result|=HAT_B;result|=HAT_SELECT;}
	return result;
}

void get_battery_stats(){
	monitor_battery_voltage();
	battery_status = round(battery_power_percent/10);
	if(battery_status>10)battery_status=10;
	if (battery_power_charge = batt_management_usb_power_detected()){
		battery_status_ico++;
		if(battery_status_ico>3){battery_status_ico=1;};
		battery_status|=battery_status_ico<<4;
	} else {
		battery_status&=~0xF0;
	}
	//printf("bs:[%02X]\tbpp:%d\tbpc:%d\n",battery_status,battery_power_percent,battery_power_charge);

	/*battery_status = battery_power_percent;
	if(battery_status>100)battery_status=100;
	memset(batt_text,0,sizeof(batt_text));
	sprintf(batt_text," %03d%%",battery_status);
	if(batt_management_usb_power_detected()){batt_text[0]=0x18;};
	*/
}



/*-------Emulation--------*/
/*
repeating_timer_t draw_timer;

bool FAST_FUNC(Draw_callback)(repeating_timer_t *rt){
	if(emulation){
		gw_system_blit(&graph_buf[0]);
	}
	return true;
}

void Init_Draw(){
	if (add_repeating_timer_ms(-1000000 / 60, Draw_callback, NULL, &draw_timer)) {
		printf("Draw Time init - OK\n");
	} else {
		printf("Failed to add timer\n");
	}
}
*/

void gw_set_background(){
	if (gw_head.flags & FLAG_RENDERING_LCD_INVERTED){
		memset(graph_buf, CL_BLACK, gw_head.background_pixel_size);
	} else {
		memcpy(graph_buf,gw_background,gw_head.background_pixel_size);
	}	
}

repeating_timer_t emulation_timer;

volatile bool emu_trigger=false;

bool FAST_FUNC(Emu_callback)(repeating_timer_t *rt){
	emu_trigger = true;
	return true;
}


void Render_loop() {
	multicore_lockout_victim_init();
	while (true) {
		if(emulation) {
			gw_system_blit(&graph_buf[0]);
		}
	}
}


void enter_pause(void){
	printf("Enter emu pause\n");
	emulation=false;
	emu_paused=true;
	softkey_only=true;
}

void resume_pause(void){
	printf("Resume emu from pause\n");
	gw_set_background();
	emulation=true;
	emu_paused=false;
	softkey_only=false;
}

volatile uint32_t old_hw_buttons = 0;

uint32_t gw_get_buttons(){
    uint32_t hw_buttons = 0;
	
    if (!softkey_only){
		if ((data_joy&D_JOY_LEFT)||(data_joy_2&D_JOY_LEFT))		hw_buttons |= GW_BUTTON_LEFT;
		if ((data_joy&D_JOY_RIGHT)||(data_joy_2&D_JOY_RIGHT))	hw_buttons |= GW_BUTTON_RIGHT;
		if ((data_joy&D_JOY_UP)||(data_joy_2&D_JOY_UP))			hw_buttons |= GW_BUTTON_UP;
		if ((data_joy&D_JOY_DOWN)||(data_joy_2&D_JOY_DOWN))		hw_buttons |= GW_BUTTON_DOWN;
		if ((data_joy&D_JOY_A)||(data_joy_2&D_JOY_A))			hw_buttons |= GW_BUTTON_A;
		if ((data_joy&D_JOY_B)||(data_joy_2&D_JOY_B))			hw_buttons |= GW_BUTTON_B;
		if ((data_joy&D_JOY_X)||(data_joy_2&D_JOY_X))			hw_buttons |= GW_BUTTON_TIME;
		if ((data_joy&D_JOY_Y)||(data_joy_2&D_JOY_Y))			hw_buttons |= GW_BUTTON_GAME;
		/*if(joy_pressed){
			printf("data_joy:[%08X]\n",data_joy);
		}*/
    }

    // software keys
    hw_buttons |= ((uint32_t)softkey_A_pressed) << 4;
	hw_buttons |= ((uint32_t)softkey_B_pressed) << 5;
    hw_buttons |= ((uint32_t)softkey_Game_B_pressed) << 6;
	hw_buttons |= ((uint32_t)softkey_Game_A_pressed) << 7;
    hw_buttons |= ((uint32_t)softkey_time_pressed) << 10;
    hw_buttons |= ((uint32_t)softkey_alarm_pressed) << 11;

	
	/*if((hw_buttons>0)&&(old_hw_buttons!=hw_buttons)){
		printf("hw_buttons:[%08X][%02X]\n",hw_buttons,(uint8_t)softkey_only);
		old_hw_buttons=hw_buttons;
	}*/
    return hw_buttons;
}

void start_watch_mode(){
	if(softkey_duration>0) return;
	printf("G&W emulate watch mode\n");
	softkey_only = 1;
	//gw_system_reset();
	// From reset state : run
	gw_system_run(GW_SYS_FREQ*2);
	// press TIME to exit TIME settings mode
	softkey_time_pressed = 1;
	gw_system_run(GW_SYS_FREQ/2);
	softkey_time_pressed = 0;
	gw_system_run(GW_SYS_FREQ*2);
	// synchronize G&W with RTC and run
	gw_system_run(GW_SYS_FREQ);
	// press A required by some game
	softkey_A_pressed = 1;
	gw_system_run(GW_SYS_FREQ/2);
	softkey_A_pressed = 0;
	gw_system_run(GW_SYS_FREQ);
	// enable user keys
	softkey_only = 0;
}

/*-------Emulation--------*/



FileRec* file=NULL;

uint32_t timer_update =0;
uint32_t main_loop =0;
uint32_t main_loop1 =0;

uint8_t help_lng;

int main(void){
	//vreg_set_voltage(VREG_VOLTAGE_1_20);//def
	vreg_set_voltage(VREG_VOLTAGE_1_30);
	//vreg_set_voltage(VREG_VOLTAGE_1_25);
	
	busy_wait_ms(100);
	/*
		вот эти пробуй:280, 288, 290400, 296, 297, 300
		дальше такие:302400, 303, 304800, 306, 307, 308, 309600, 312, 314400, 315, 316, 316800
	*/
	
	//set_sys_clock_khz(300000, true);
	//set_sys_clock_khz(284000, false);
	//set_sys_clock_khz(290400, false);
	//set_sys_clock_khz(504000, false);
	//set_sys_clock_khz(252000, false);//def
	//set_sys_clock_khz(297600, false);
	/*
	#if VIDEO_VGA
	//set_sys_clock_khz(378000, true);
	set_sys_clock_khz(315000, false);
	#endif
	*/
	#ifdef VGA_HDMI
		set_sys_clock_khz(315000, false);
	#endif
	#if COMPOSITE_TV||SOFT_COMPOSITE_TV
		set_sys_clock_khz(378000, false);
		//set_sys_clock_khz(360000, false);
	#endif
	
	busy_wait_ms(10);
	
	stdio_init_all();

	busy_wait_ms(100);

	gpio_init(WORK_LED_PIN);
	gpio_set_dir(WORK_LED_PIN,GPIO_OUT);

	#ifdef DEBUG_DELAY
	while (!stdio_usb_connected()) {
		gpio_put(WORK_LED_PIN, 1);
		busy_wait_ms(50);
		gpio_put(WORK_LED_PIN, 0);
		busy_wait_ms(500);
	}
	busy_wait_ms(500);
	#endif

	//busy_wait_ms(1500);
	
	printf("MurMulator %s by %s \n\n",FW_VERSION,FW_AUTHOR);
	printf("Main Program Start!!!\n");
	printf("CPU clock=%ld Hz\n",(long)clock_get_hz(clk_sys));

	printf("init FileSystem\n");
	//int fs=vfs_init();
	init_fs=init_filesystem();
	printf("init FS:%d\n",init_fs);
	
	
	printf("Load config\n");
	settings_index = 0;
	settings_lines = 0;
	for(uint8_t i=0;i<CONFIG_MENU_ITEMS;i++){
		if(*config_menu[i]==0){
			settings_lines=i-1;
			break;
		}
	}	
	if(init_fs){
		if(!config_read()){
			if(!config_write_defaults()){
				printf("Error saving %s\n",CONFIG_FILENAME);
			}
		} /*else {
			cfg_sound_mode=3;
		}*/
		if(cfg_boot_scr>MAX_CFG_BOOT_SCR_MODE){
			cfg_boot_scr=DEF_CFG_BOOT_SCR_MODE;
		}		
		if(cfg_hud_enable>MAX_CFG_HUD_MODE){
			cfg_hud_enable=MAX_CFG_HUD_MODE;
		}		
		if(cfg_sound_out_mode>MAX_CFG_OUT_MODE){
			cfg_sound_out_mode=DEF_CFG_OUT_MODE;
		}
		if(cfg_volume>MAX_CFG_VOLUME_MODE){
			cfg_volume=DEF_CFG_VOLUME_MODE;
		}
		if(cfg_video_out>MAX_CFG_VIDEO_MODE){
			cfg_video_out=DEF_CFG_VIDEO_MODE;
		}
		if(cfg_frame_rate>MAX_CFG_VIDEO_FREQ_MODE){
			cfg_frame_rate=DEF_CFG_VIDEO_FREQ_MODE;
		}
		if(cfg_mobile_mode>MAX_CFG_MOBILE_MODE){
			cfg_mobile_mode=DEF_CFG_MOBILE_MODE;
		}	
		if((cfg_lcd_video_out>MAX_CFG_LCD_VIDEO_MODE)||(cfg_lcd_video_out<MIN_CFG_LCD_VIDEO_MODE)){
			cfg_lcd_video_out=DEF_CFG_LCD_VIDEO_MODE;
		}
		if(cfg_brightness>MAX_CFG_BRIGHT_MODE){
			cfg_brightness=DEF_CFG_BRIGHT_MODE;
		}
		if(cfg_rotate>MAX_CFG_ROTATE){
			cfg_rotate=DEF_CFG_ROTATE;
		}	
		if(cfg_inversion>MAX_CFG_INVERSION){
			cfg_inversion=DEF_CFG_INVERSION;
		}
		if(cfg_pixels>MAX_CFG_PIXELS){
			cfg_pixels=DEF_CFG_PIXELS;
		}
	}  
	if(!init_fs){
		cfg_sound_out_mode=DEF_CFG_OUT_MODE;
		cfg_volume=DEF_CFG_VOLUME_MODE;
		cfg_lcd_video_out=DEF_CFG_LCD_VIDEO_MODE;
		cfg_brightness=DEF_CFG_BRIGHT_MODE;
		cfg_rotate=DEF_CFG_ROTATE;
		cfg_pixels=DEF_CFG_PIXELS;
		cfg_hud_enable=DEF_CFG_HUD_MODE;
		cfg_mobile_mode=DEF_CFG_MOBILE_MODE;
	}

	
	//printf("*Settings read: %s\n",video_out_config[cfg_lcd_video_out]);

	current_frame_rate	= 	cfg_frame_rate;
	current_video_out	= 	cfg_video_out;
	current_rotate		=	cfg_rotate;
	current_inversion	=	cfg_inversion;
	current_pixels		=	cfg_pixels;
	current_sound_out	=	cfg_sound_out_mode;
	current_hud_mode	=	cfg_hud_enable;
	current_mobile_mode	=	cfg_mobile_mode;

	//cfg_sound_mode=0;
	#ifdef DEBUG_DELAY
		printf("Real config:\n");
		printf("  + cfg_sound_out_mode:%d\n", cfg_sound_out_mode);
		printf("  + cfg_hud_enable:%d\n", cfg_hud_enable);
		printf("  + cfg_frame_rate:%d\n", cfg_frame_rate);
	#endif
	
	gw_system_sound_init();

	// ----------Init Sound------------
	sound_timer_enabled=false;
	printf("Init Sound - ");
	if(Init_Sound()){
		printf("Sound - OK\n");
	} else {
		printf("Sound - FAIL\n");
	}

	printf("Read root folder\n");
	if (init_fs){
		N_files = read_select_dir(cur_dir_index);
		if(N_files==0){
			for (uint8_t i = 0; i < 5; i++){
				N_files = read_select_dir(cur_dir_index);
				if(N_files>0) break;
				printf("Read dir fail - retry\n");	
				busy_wait_ms(250);
			}
		}
		printf("Init FS Success:%d files\n",N_files);
		//N_files = read_select_dir(cur_dir_index);
		//printf("DIR:%s cur_dir_index:%d\n",dir_path,cur_dir_index);
		//printf("init_fs>N_files:%d\n",N_files);
	}  else printf("Init FS Fail\n");

	printf("Init Emulation timer\n");
	short int rate = GW_REFRESH_RATE;
	if (!add_repeating_timer_us(-1000000 / rate, Emu_callback, NULL, &emulation_timer)) {
		printf("Failed to add emulation timer\n");
		//return false;
	}

	printf("Init graphics buffer\n");

	init_screen(graph_buf,SCREEN_W,SCREEN_H,mode_8bpp);

	bool (*handler_ptr)(short);

	/*
	for(uint8_t idx;idx<10;idx++){
		hud[idx].active=false;
	}
	*/
	printf("Begin video init: ");
	#ifdef VGA_HDMI
		/*busy_wait_ms(100);
		startVGA(cfg_frame_rate);
		printf("VGA Started\n");*/
		busy_wait_ms(100);
		uint8_t c_video=cfg_video_out;
		if((g_out)c_video==g_out_AUTO){
			printf("Autodetect:");
			c_video = (uint8_t)graphics_test_output();
			if((g_out)c_video>g_out_HDMI){
				c_video=cfg_lcd_video_out;
			}
			printf(" - found %s\n",video_out_config[c_video]);
			printf("Settings:%s@%s\n",video_out_config[c_video],video_freq_config[cfg_frame_rate]);
			//cfg_video_out=c_video;
		} else {
			//c_video=cfg_video_out;
			printf("Settings:%s@%s\n",video_out_config[cfg_video_out],video_freq_config[cfg_frame_rate]);
			
		}

		graphics_set_mode(g_mode_320x240x8bpp);

		if((g_out)c_video>g_out_HDMI){
			printf("LCD Params init:");
			graphics_set_rotate((rotate)cfg_rotate);
			graphics_set_inversion((bool)cfg_inversion);
			graphics_set_pixels(cfg_pixels);
			printf(" %s %s %s\n",video_out_rotate[cfg_rotate],video_out_inversion[cfg_inversion],video_out_pixels[cfg_pixels]);
		}

		graphics_set_buffer(graph_buf);

		graphics_set_hud_buffer(hud_line);
		memset(hud_line,0x11,SCREEN_W);
		graphics_set_hud_handler(NULL);

		//graphics_set_textbuffer(txt_buf,txt_buf_color);

		graphics_init((g_out)c_video,(fr_rate)cfg_frame_rate);

		set_def_palette((g_out)c_video);

		if((g_out)c_video>g_out_HDMI){
			gpio_set_function(TFT_LED_PIN, GPIO_FUNC_PWM);
			uint slice_num = pwm_gpio_to_slice_num(TFT_LED_PIN);		
			pwm_config led_pwm=pwm_get_default_config();
			pwm_config_set_clkdiv(&led_pwm,1.0);
			pwm_config_set_wrap(&led_pwm,256);//MAX PWM value
			pwm_init(slice_num,&led_pwm,true);
			pwm_set_gpio_level(TFT_LED_PIN,(TFT_MIN_BRIGHTNESS+(cfg_brightness*10)));			//уровень подсветки TFT
			printf("LCD Level:%d\n",cfg_brightness);
		}
		current_video_out = c_video;

		//startVGA(cfg_frame_rate);
		printf("Video Out Started\n");
	#endif

	#ifdef COMPOSITE_TV
		graphics_init(g_TV_OUT_NTSC);
		graphics_set_buffer(graph_buf);
		graphics_set_mode(g_mode_320x240x4bpp);
		for(int i=0;i<16;i++){
			int I=(i>>3)&1;
			int G=(i>>2)&1;
			int R=(i>>1)&1;
			int B=(i>>0)&1;
			uint32_t RGB=((R?I?255:170:0)<<16)|((G?I?255:170:0)<<8)|((B?I?255:170:0)<<0);
			graphics_set_palette(i,RGB);
		}
	#endif
	#ifdef SOFT_COMPOSITE_TV
		graphics_set_buffer(graph_buf);
		#ifndef SOFT_CVBS_NO_HUD
		graphics_set_hud_buffer(hud_line);
		memset(hud_line,0x11,SCREEN_W);
		graphics_set_hud_handler(&hud_prepare);
		#endif
		graphics_init();
		tv_out_mode_t g_mode=graphics_get_default_mode((g_out)cfg_video_out);
		printf("Found %s\n",video_out_config[cfg_video_out]);
		graphics_set_mode(g_mode);
		for(int i=0;i<16;i++){
			int I=(i>>3)&1;
			int G=(i>>2)&1;
			int R=(i>>1)&1;
			int B=(i>>0)&1;
			uint32_t RGB=((R?I?255:170:0)<<16)|((G?I?255:170:0)<<8)|((B?I?255:170:0)<<0);
			// uint32_t RGB=((R?I?255:200:0)<<16)|((G?I?255:200:0)<<8)|((B?I?255:200:0)<<0);
			graphics_set_palette(i,RGB);
		}
		printf("Soft Composite Started\n");
	#endif

	updatePalette(mur_logo_pal,LOGO_PAL_SIZE);

	hud_timer=0;
	hud_ptr = NULL;
	current_hud_mode=cfg_hud_enable;
	old_hud_mode=current_hud_mode;

	//-------Init Battery Check--------
	#ifdef VGA_HDMI
		if(cfg_mobile_mode==MOBILE_MURM_ON){
			hud_ptr = &hud_battery;
			current_hud_mode|=HM_SHOW_BATTERY;
			battery_status = 0;
		}	
		if(current_hud_mode&HM_SHOW_BATTERY){
			printf("Battery monitor init\n");
			if(batt_management_init()){
				printf("Battery monitor started\n");
			}
			busy_wait_ms(100);
			get_battery_stats();
			//monitor_battery_voltage();
			//monitor_battery_voltage();
		}
	#endif
	//-------Init Battery Check--------	
	input_init();

	//-------Launch Second core--------	
	printf("Launch render\n");
	void (*Render_ptr)();
	Render_ptr = Render_loop;
	multicore_launch_core1(Render_ptr);
	
	/*
	void (*ZXThread_ptr)();
	ZXThread_ptr = ZXThread;
	multicore_launch_core1(ZXThread_ptr);
	busy_wait_ms(50);
	*/
	//-------Launch Second core--------	
	
	/*-------Emulation--------*/
    /* clear soft keys */
	softkey_A_pressed		= 0;
	softkey_B_pressed		= 0;
	softkey_Game_A_pressed	= 0;
	softkey_Game_B_pressed	= 0;
	softkey_time_pressed	= 0;
	softkey_alarm_pressed	= 0;
    softkey_duration = 0;
	/*-------Emulation--------*/

	cur_dir_index=0;
	
	for(uint8_t i=0;i<DIRS_DEPTH;i++){
		cursor_index_old[i]=-1;
	}
	
	
	menu_ptr=0;
	menu_mode[menu_ptr]=EMULATION;
	menu_ptr++;
	switch (cfg_boot_scr){
		case BOOT_SCR_LOGO: menu_mode[menu_ptr]=BOOT_LOGO; break;
		case BOOT_SCR_FILE_MAN: menu_mode[menu_ptr]=MENU_MAIN; break;
		case BOOT_SCR_EMULATION: menu_mode[menu_ptr]=EMULATION; break;
		default: menu_mode[menu_ptr]=BOOT_LOGO; break;
	}
	
	//TRDOS_disabled=true;
	main_loop = 0;
	int32_t ticker = 0;

	graphics_set_hud_handler(NULL);
	#ifndef SOFT_CVBS_NO_HUD
	graphics_set_hud_handler(&hud_battery);
	graphics_set_hud_handler(&hud_scale);
	graphics_set_hud_handler(&hud_kb_lock);
	#endif
	graphics_set_hud_handler(NULL);
	//printf("[%08X][%08X][%08X]\n",help_ptr,help_text_rus,help_text_eng);
	help_lng=0;
	//printf("[%08X][%08X][%08X]\n",help_ptr,help_text_rus,help_text_eng);
	printf("starting main loop %d>[%02X]\n",menu_ptr,menu_mode[menu_ptr]);
	do{ //BEGIN GLOBAL LOOP
		graphics_set_hud_handler(hud_ptr);
		//BOOT SCREEN
		if(menu_mode[menu_ptr]==BOOT_LOGO){
			
			//boot logo
			//zx_machine_enable_vbuf(false);
			draw_rect(0,0,SCREEN_W,SCREEN_H,CL_BLACK,true);//Заливаем экран 
			//draw_mur_logo_big(0,0,2);
			draw_mur_logo_big((SCREEN_W-LOGO_PIX_WIDTH)/2,((SCREEN_H-LOGO_PIX_HEIGHT)/2),2);
			//(SCREEN_W-LOGO_PIX_WIDTH)/2)//((SCREEN_H-LOGO_PIX_HEIGHT)/2)
			ticker=0;
			uint8_t repeat=0;
			printf("Enter Boot Screen Mode\n");
			while(true){
				while(!wait_kbdjoy_emu()){
					ticker++;
					if ((ticker%8192)==0){
						if(current_hud_mode&HM_SHOW_BATTERY){
							get_battery_stats();
						}
					}
					if(ticker>131070){ticker=0;}
				}
				if((KBD_HOME)||(KBD_L_WIN)||(KBD_R_WIN)){
					menu_mode[menu_ptr]=MENU_MAIN;
					last_action = 0;
					clear_input();
					busy_wait_ms(100);
					break;
				}
				if (KBD_F1) {
					menu_mode[menu_ptr]=MENU_HELP;
					need_redraw=true;
					clear_input();
					busy_wait_ms(100);
					break;
				}
				if(((data_joy&D_JOY_START)||(data_joy_2&D_JOY_START))||((((KBD_L_CTRL)||(KBD_R_CTRL)))&&(KBD_F11))){
					menu_mode[menu_ptr]=MENU_JOY_MAIN;
					clear_input();
					busy_wait_ms(100);
					break;
				}
				if((KBD_PRESS)||(joy_pressed)){
					menu_ptr--;
					clear_input();
					busy_wait_ms(100);
					break;
				}
			}
			//convert_kb_u_to_kb_zx(&kb_st_ps2,zx_write_buffer->kb_data);
			joy_pressed=false;
		}
		//BOOT SCREEN

		//BEGIN MENU LOOP
		//printf("1 menu_ptr:%d menu_mode:%d\n",menu_ptr,menu_mode[menu_ptr]);
		if(menu_mode[menu_ptr]<EMULATION){
			old_hud_mode=current_hud_mode;
			current_hud_mode&=~HM_SHOW_VOLUME;
			current_hud_mode&=~HM_SHOW_BRIGHT;
			if(current_hud_mode&HM_SHOW_BATTERY){
				handler_ptr=&hud_battery;
				graphics_set_hud_handler(handler_ptr);
			} else {
				graphics_set_hud_handler(NULL);
			};
			set_def_palette((g_out)current_video_out);
			//zx_machine_enable_vbuf(false);
			is_new_screen=true;
			printf("Enter Menu Mode\n");
			ticker=0;
			do{// main menu loop
				#ifdef VGA_HDMI
				//graphics_update_screen();
				#endif
				ticker++;
				if ((ticker%32700)==0){
					if(current_hud_mode&HM_SHOW_BATTERY){
						get_battery_stats();
					}
				}				
				if((us_to_ms(time_us_32())-timer_update)>TIMER_PERIOD){
					flip_led^=1;
					if(!flip_led){
						timer_update=us_to_ms(time_us_32())+2500;
						gpio_put(WORK_LED_PIN,1);
					} else {
						timer_update=us_to_ms(time_us_32())+5000;
						gpio_put(WORK_LED_PIN,0);
					}
					// printf("Time is:%d \n",my_millis());
					//sleep_ms(50);
					//gpio_put(WORK_LED_PIN,0);
					//busy_wait_ms(50);
				};			
				if(menu_mode[menu_ptr]==EMULATION){break;}
				process_input();
				if((data_joy>>16)>0){
					data_joy=(data_joy>>16);
				}
				
				//printf("data_joy:[%08X]\n",data_joy);
				/*if(((Joystics.Present_WII_joy)||(Joystics.Present_i2c_PCF_16_buttons))&&(data_joy!=0)){
					busy_wait_ms(150); //WII joystick delay
				}else*/
				if ((KBD_PRESS)||(joy_pressed)){ //like a speccy keypress sound
					/*hw_zx_set_beep_out(0x00);
					busy_wait_us(3500);
					hw_zx_set_beep_out(0x08);
					busy_wait_us(3500);
					hw_zx_set_beep_out(0x00);
					/*
					busy_wait_ms(3);
					hw_zx_set_beep_out(0x08);
					busy_wait_ms(3);
					hw_zx_set_beep_out(0x00);
					*/
				}

				if(data_joy!=0){
					busy_wait_ms(110); //joystick delay
				}						
				
				//busy_wait_us(500);
				/*--HARD reset--*/
				if (((KBD_L_SHIFT)||(KBD_R_SHIFT))&&((KBD_L_ALT)||(KBD_R_ALT))&&(KBD_DELETE)){
					software_reset();
				}
				/*--HARD reset--*/
				/*--DEF reset--*/
				if (((KBD_L_CTRL)||(KBD_R_CTRL))&&((KBD_L_ALT)||(KBD_R_ALT))&&(KBD_BACK_SPACE)){ 
					config_write_defaults();
					busy_wait_ms(100);
					software_reset();
				}
				/*--DEF reset--*/				
				/*--HELP--*/
				if ((KBD_F1)&&(menu_mode[menu_ptr]!=MENU_HELP)){
					menu_ptr++;
					menu_mode[menu_ptr]=MENU_HELP;
					is_new_screen=true;
					need_redraw=true;
					last_action=0;
				}
				/*--HELP--*/				
				//Controllers speed fix, joy is faster than keyboard
				/*if((WII_Init)&&(data_joy!=0)){
					busy_wait_ms(130); //WII joystick delay
				}else
				if(data_joy!=0){
					busy_wait_ms(100); //joystick delay
				} else {
					busy_wait_ms(50);
				}*/
				//Controllers speed fix, joy is faster than keyboard
				////printf("kbd[0][%08X]   kbd[1][%08X]   kbd[2][%08X]   kbd[2][%08X]\n",kb_st_ps2.u[0],kb_st_ps2.u[1],kb_st_ps2.u[2],kb_st_ps2.u[3]);
				/*if((data_joy!=0)||(KBD_PRESS)){
					//printf("menu_mode[menu_ptr]>%d   menu_ptr>%d   fast_mode[fast_mode_ptr]>%d   fast_mode_ptr>%d   fast_menu_index>%d   old_menu_index>%d\n",menu_mode[menu_ptr],menu_ptr,fast_mode[fast_mode_ptr],fast_mode_ptr,fast_menu_index,old_menu_index);
				}*/
				//printf("4 menu_ptr:%d menu_mode:%d\n",menu_ptr,menu_mode[menu_ptr]);
				//printf("5 fast_mode_ptr:%d fast_mode:%d\n",fast_mode_ptr,fast_mode[fast_mode_ptr]);
				/*if ((ticker%8)==0){
					printf("M> joy_pressed:%d   data_joy:[%02X]   old_data_joy:[%02X]   rel_data_joy:[%02X]   hat_switch:[%02X] \n",joy_pressed,data_joy,old_data_joy,rel_data_joy,hat_switch);
				}*/
				if((!joy_pressed)&&(hat_switch>0)){
					hat_switch=0;
					clear_input();
					continue;
				}				
				switch (menu_mode[menu_ptr]){
					case MENU_MAIN:
						if(is_new_screen){
							draw_main_window();					
							if(init_fs){
								////printf("is_new_screen>N_files:%d\n",N_files);
								draw_file_window();
								display_file_index=-1;
								draw_mur_logo_big(PREVIEW_POS_X+((PREVIEW_WIDTH/2)-(LOGO_SMALL_PIX_WIDTH/2)),60,1);
							}
							is_new_screen=false;
							need_redraw=true;
							clear_input();
						}
						/*--Return from Menu--*/
						if((KBD_ESC)||((data_joy&D_JOY_START)&&(hat_switch==0))||(KBD_HOME)||(KBD_L_WIN)||(KBD_R_WIN)){
							fast_menu_index=old_menu_index;
							fast_mode_ptr=0;
							menu_ptr--;
							old_menu_index=0;
							is_new_screen=true;
							busy_wait_ms(150);
							//clear_input();
							hat_switch=0x80;
							continue;
						}	
						/*--Return from Menu--*/
						/*--Settings Menu--*/
						if(KBD_F12){
							menu_ptr++;
							menu_mode[menu_ptr]=MENU_SETTINGS;
							is_new_screen=true;
							last_action=0;
							continue;
						}
						/*--Settings Menu--*/
						if(init_fs==false){
							if(need_redraw){
								MessageBox("SD Card not found!!!","	Please REBOOT   ",CL_LT_YELLOW,CL_RED,0);
								need_redraw=false;
								busy_wait_ms(100);
								clear_input();
								break;
							}
						} else {
							if(KBD_PRT_SCR){
								show_screen^=1;
								last_action = (my_millis()+SHOW_SCREEN_DELAY)-150;
							}
							if((KBD_ENTER)||(data_joy==D_JOY_A)){
								need_redraw=true;
								//printf("file_type:%x\n",files[cur_file_index][(FILE_NAME_LEN-1)]);
								file = (FileRec*)&files[cur_file_index];
								//printf("fname: %s\n",file->filename);
								if (file->attr&AM_DIR){ //выбран каталог
									//printf("cur_file_index:%d  dir_index:%d\n",cur_file_index,cur_dir_index);
									if (cur_file_index==0){//на уровень выше
										if (cur_dir_index>0) {
											cur_dir_index--;
											N_files = read_select_dir(cur_dir_index);
											cur_file_index=0;
											if(cursor_index_old[cur_dir_index]>=0){
												cur_file_index=cursor_index_old[cur_dir_index];
												////printf("restore index:%d\n",cur_file_index);
											}													
											draw_rect(FONT_W,FONT_H,(FONT_W*20),FONT_H,COLOR_MAIN_RAMK,true);
											//busy_wait_ms(200);
											last_action = my_millis();
										};
									} else
									if (cur_dir_index<(DIRS_DEPTH-2)){//выбор каталога
										//printf("store index:%d>%d\n",cur_file_index,cur_dir_index);
										cursor_index_old[cur_dir_index]=cur_file_index;
										cur_dir_index++;
										strncpy(dirs[cur_dir_index],files[cur_file_index],(FILE_NAME_LEN-1));
										N_files = read_select_dir(cur_dir_index);
										cur_file_index=0;
										display_file_index=cur_file_index;
										//shift_file_index=0;
										last_action = my_millis();
										busy_wait_ms(200);
									}
								} else {  // выбран файл
									memset(afilename,0,FILE_NAME_LEN);
									strncpy(afilename,files[cur_file_index],(FILE_NAME_LEN-1));

									strcpy(activefilename,dir_path);
									strcat(activefilename,"/");
									strcat(activefilename,afilename);

									const char* ext = get_file_extension(afilename);
									/*------Load and Run image file------*/
									if(strcasecmp(ext, "gwm") == 0) {
										printf("current file select=%s\n",activefilename); 
										memset(temp_msg,0,sizeof(temp_msg));
										sprintf(temp_msg," Loading file:%s",afilename);
										MessageBox("GWM",temp_msg,CL_WHITE,CL_BLUE,0);
										strcpy(romfilename,activefilename);
										if(gw_romloader(romfilename)){
											//gw_dump_struct();
											if(gw_system_config()){
												printf("G&W configured\n");
											   gw_system_start();
											   printf("G&W start\n");
											   gw_system_reset();
											   printf("G&W reset\n");
											   start_watch_mode();
											}				   
											menu_mode[menu_ptr]=EMULATION;
											printf("gw_romloader - OK\n");
											continue;
										} else {
											MessageBox("Error loading ROM!!!",afilename,CL_YELLOW,CL_LT_RED,1);
											last_action = my_millis();
											draw_file_window();
											emu_paused = true;
											continue;
										}
										continue;
									}
									/*------Load and Run image file------*/
								}
							}
							if(((KBD_INSERT)||(KBD_SPACE)||(data_joy==D_JOY_B))&&(cur_file_index>0)){
								file = (FileRec*)&files[cur_file_index];
								if(file->attr&~TOP_DIR_ATTR){
									file->attr^=SELECTED_FILE_ATTR;	
								}
								cur_file_index++;
								need_redraw=true;
								last_action = my_millis();
							}
							if((((KBD_L_SHIFT)||(KBD_R_SHIFT))&&(KBD_DELETE))&&(cur_file_index>0)){
								sel_files=0;
								for(short int i=0;i<N_files+1;i++){
									file = (FileRec*)&files[i];
									if(file->attr&SELECTED_FILE_ATTR){
										sel_files++;
									}
								}
								if(sel_files>0){
									memset(temp_msg,0,sizeof(temp_msg));
									sprintf(temp_msg,"DELETE [%d] SELECTED FILES?",sel_files);
									if(DialogBox("[DELETE]",temp_msg,CL_RED,COLOR_BACKGOUND,DIALOG_OK_CAN)==DLG_RES_OK){
										del_files=0;
										err_files=0;
										for(short int i=0;i<N_files+1;i++){
											file = (FileRec*)&files[i];
											if(file->attr&SELECTED_FILE_ATTR){
												memset(afilename,0,FILE_NAME_LEN);
												strncpy(afilename,file->filename,(FILE_NAME_LEN-1));
												strcpy(activefilename,dir_path);
												strcat(activefilename,"/");
												strcat(activefilename,afilename);
												if(file->attr&AM_DIR){
													//printf("Delete node:%s\n",activefilename);
													file_descr=sd_delete_node(activefilename, sizeof activefilename / sizeof activefilename[0], &sd_file_info);
													if (file_descr==FR_OK){
														del_files++;
														//printf("OK\n");
													} else {
														err_files++;
														//printf("ERROR\n");
													}
												}else{
													printf("Delete file>%s\n",activefilename);
													file_descr=sd_delete(activefilename);
													if (file_descr==FR_OK){
														del_files++;
														//printf("OK\n");
													}else{
														err_files++;
														//printf("ERROR\n");
													}
												}
												file->attr&=~SELECTED_FILE_ATTR;
											}
										}
										memset(temp_msg,0,sizeof(temp_msg));
										sprintf(temp_msg,"[%d]/ERR[%d] OF [%d] FILES DELETED!!!",del_files,err_files,sel_files);
										DialogBox("[DELETE]",temp_msg,CL_LT_BLUE,COLOR_BACKGOUND,DIALOG_OK);
										is_new_screen=true;
										N_files = read_select_dir(cur_dir_index);
										continue;
									} else {
										for(short int i=0;i<N_files+1;i++){
											file = (FileRec*)&files[i];
											if(file->attr&SELECTED_FILE_ATTR){
												file->attr^=SELECTED_FILE_ATTR;
											}
										}										
										is_new_screen=true;
										continue;
									}
								}
							}
							//стрелки вверх вниз
							if(((KBD_DOWN)||(data_joy==D_JOY_DOWN))&&(cur_file_index<(N_files))){ 
								cur_file_index++;
								clear_input();
								need_redraw=true;
								last_action = my_millis();
							}
							if(((KBD_UP)||(data_joy==D_JOY_UP))&&(cur_file_index>0)){
								cur_file_index--;
								clear_input();
								need_redraw=true;
								last_action = my_millis();
							}
							//начало и конец списка
							if((KBD_LEFT)){
								cur_file_index=0;
								shift_file_index=0;
								clear_input();
								need_redraw=true;
								last_action = my_millis();
							}
							if((KBD_RIGHT)){
								cur_file_index=N_files;
								shift_file_index=(N_files>=NUM_SHOW_FILES)?N_files-NUM_SHOW_FILES:0;
								clear_input();
								need_redraw=true;
								last_action = my_millis();
							}
							//PAGE_UP PAGE_DOWN
							if(((KBD_PAGE_DOWN)||(data_joy==D_JOY_RIGHT))&&(cur_file_index<(N_files))){
								cur_file_index+=NUM_SHOW_FILES;
								need_redraw=true;
								last_action = my_millis();
							}
							if(((KBD_PAGE_UP)||(data_joy==D_JOY_LEFT))&&(cur_file_index>0)){
								cur_file_index-=NUM_SHOW_FILES;
								need_redraw=true;
								last_action = my_millis();
							}
							//Возврат на уровень выше по BACKSPACE
							if((KBD_BACK_SPACE)||(data_joy==D_JOY_SELECT)){
								if (cur_dir_index==0){
									if (cur_file_index==0) cur_file_index=1;//не можем выбрать каталог вверх
									if (shift_file_index==0) shift_file_index=1;//не отображаем каталог вверх
									read_select_dir(cur_dir_index);
								} else {
									cur_dir_index--;
									N_files = read_select_dir(cur_dir_index);
									cur_file_index=0;
									//draw_text_len(FONT_W,FONT_H,"					",COLOR_BACKGOUND,COLOR_BORDER,20);
									draw_rect(FONT_W,FONT_H,(FONT_W*20),FONT_H,COLOR_MAIN_RAMK,true);
									cur_file_index = 0;
									shift_file_index = 0;
								}
								need_redraw=true;
								last_action = my_millis();
							}	
							if(need_redraw){
								//last_action = my_millis();
								scroll_lfn=false;
								if (cur_file_index<0) cur_file_index=0;
								if (cur_file_index>=N_files) cur_file_index=N_files;
								for (short int i=NUM_SHOW_FILES; i--;){
									if ((cur_file_index-shift_file_index)>=(NUM_SHOW_FILES)) shift_file_index++;
									if ((cur_file_index-shift_file_index)<0) shift_file_index--;
								}
								//ограничения корневого каталога
								if (cur_dir_index==0){
									if (cur_file_index==0) cur_file_index=1;//не можем выбрать каталог вверх
									if (shift_file_index==0) shift_file_index=1;//не отображаем каталог вверх
								}
								//printf("cur_dir_index:%d  cur_file_index:%d shift_file_index:%d\n",cur_dir_index,cur_file_index,shift_file_index);
								//прорисовка
								//заголовок окна - текущий каталог		
								//draw_text_len(FONT_W,FONT_H-1,dir_path+2,COLOR_TEXT,COLOR_FULLSCREEN,20);
								//sprintf(save_file_name_image,"0:/save/__F%d.Z80 ",inx_f1);
								////printf("Dir:%s",dir_path);
								if (strlen(dir_path+2)>0){
									draw_text_len(FONT_W,FONT_H,dir_path+2,COLOR_ITEXT,COLOR_MAIN_RAMK,20);
								} /*else {
									draw_text_len(FONT_W,FONT_H-1,"					",COLOR_BACKGOUND,COLOR_BORDER,20);
								}*/
								for(short int i=0;i<NUM_SHOW_FILES;i++){
									//busy_wait_us(150);
									//если файлов меньше, чем отведено экрана - заполняем пустыми строками
									if ((i>N_files)||((cur_dir_index==0)&&(i>(N_files-1)))){
										draw_rect(FONT_W,(i+2)*FONT_H,(FONT_W*FILE_NAME_LEN),SCREEN_H-((FONT_H*3)+(i*FONT_H)),COLOR_BACKGOUND,true);
										//draw_text_len(FONT_W,2*FONT_H+i*FONT_H," ",color_text,color_bg,14);
										//last_action = my_millis();
										scroll_lfn=false;
										clear_input();
										is_new_screen=false;
										continue;
									}									
									uint8_t color_text=COLOR_TEXT;
									uint8_t color_bg=COLOR_BACKGOUND;
									icon[0] = 0x20;
									icon[1] = 0x00;
									file = (FileRec*)&files[i+shift_file_index];
           							uint8_t len = strlen(file->filename)<(FILE_NAME_LEN)?strlen(file->filename):(FILE_NAME_LEN-1);

									if (i==(cur_file_index-shift_file_index)){
										color_text=COLOR_CURRENT_TEXT;
										color_bg=COLOR_CURRENT_BG;
										strncpy(current_lfn,get_lfn_from_dir(dir_path,file),200);
									}
									if (file->attr&AM_DIR){
										icon[0] = 0xF8; //folder
										draw_text_len(FONT_W,2*FONT_H+i*FONT_H,icon,CL_YELLOW,color_bg,1);
										//draw_text_len(FONT_W,2*FONT_H+i*FONT_H,"/",color_text,color_bg,1);
									} else {
										icon[0] = 0xF7; //file
										draw_text_len(FONT_W,2*FONT_H+i*FONT_H,icon,CL_GRAY,color_bg,1);
										//draw_text_len(FONT_W,2*FONT_H+i*FONT_H," ",color_text,color_bg,1);
									}
									memset(temp_msg,0x00,sizeof(temp_msg));
									strncpy(&temp_msg[FILE_NAME_LEN+20],file->filename,FILE_NAME_LEN-1);
									const char* ext = get_file_extension(&temp_msg[FILE_NAME_LEN+20]);
									//printf(">0 %s   %s   %d   %d\n",temp_msg,ext,len,strlen(ext));
									if(strlen(ext)>0){
										memset(temp_msg,0x20,FILE_NAME_LEN);
										strncpy(temp_msg,file->filename,(len-(strlen(ext)+1)));
										//printf(">1 %s   %s   %d   %d\n",temp_msg,ext,len,strlen(ext));
										strncpy(&temp_msg[(FILE_NAME_LEN-4)],ext,3);
										//printf(">2 %s   %s   %d   %d\n",temp_msg,ext,len,strlen(ext));
										temp_msg[(FILE_NAME_LEN-5)]=0x2E;
										//printf(">3 %s   %s   %d   %d\n",temp_msg,ext,len,strlen(ext));										
									} else {
										strncpy(temp_msg,file->filename,FILE_NAME_LEN-1);
									}
									//printf("> %s   %s   %d   %d\n",temp_msg,ext,len,strlen(ext));

									if(file->attr&AM_RDO)temp_msg[(FILE_NAME_LEN-5)]=0xB0;//(FILE_NAME_LEN-3)
									if(file->attr&AM_HID)temp_msg[(FILE_NAME_LEN-5)]=0xB1;//(FILE_NAME_LEN-3)
									if(file->attr&AM_SYS)temp_msg[(FILE_NAME_LEN-5)]=0xB2;//(FILE_NAME_LEN-3)
									if(file->attr&SELECTED_FILE_ATTR){
										color_text=COLOR_SELECT_TEXT;
									}
									draw_text_len(2*FONT_W,2*FONT_H+i*FONT_H,temp_msg,color_text,color_bg,(FILE_NAME_LEN-1));//get_file_from_dir("0:/z80",i)
									////printf("> %s %d, %d\n",files[i+shift_file_index],cur_file_index,display_file_index);
								}
								
								
								short int file_inx=cur_file_index-1;
								if (file_inx==-1) file_inx=0;
								if (file_inx==N_files) file_inx+=1;
								short int shft=208*(file_inx)/(N_files<=1?1:N_files-1); 
								draw_rect((PREVIEW_POS_X-FONT_W)+1,PREVIEW_POS_Y,FONT_W-2,SCREEN_H-(FONT_H*3),COLOR_BACKGOUND,true);  //Заливка фона полосы прокрутки
								draw_rect((PREVIEW_POS_X-FONT_W)+2,shft+(PREVIEW_POS_Y+1),4,5,COLOR_MAIN_RAMK,true);  //указатель полосы прокрутки

								//if(strcasecmp(files[cur_file_index], "..")==0) {
								file = (FileRec*)&files[cur_file_index];
								if(file->attr&~TOP_DIR_ATTR){
									display_file_index=cur_file_index;
									//draw_mur_logo();
								}
								/*if ((cur_file_index>0)&&(display_file_index==-1)){
									last_action = my_millis();
								}*/
								need_redraw=false;
							}
							clear_input();								
							//break;
						}
					break;
					case MENU_JOY_MAIN:
						if(is_new_screen){
							printf("Joy new screen\n");
							graphics_set_hud_handler(NULL);
							//printf("Joy draw screen\n");
							fast_menu_index = 0;
							draw_fast_menu((SCREEN_W/2)-(9*FONT_W),(SCREEN_H/2)-(5*FONT_H),true,fast_mode[fast_mode_ptr],fast_menu_index);
							is_new_screen=false;
							busy_wait_ms(150);
							clear_input();
							last_action=0;
							scroll_lfn=false;
						}
						/*--Return from Menu--*/
						if((KBD_F12)||(KBD_ESC)||((data_joy&D_JOY_START)&&(hat_switch==0))||(KBD_HOME)||(KBD_L_WIN)||(KBD_R_WIN)){
							hat_switch=0x80;
							menu_mode[menu_ptr]=EMULATION;
							fast_mode_ptr=0;
							fast_menu_index=0;
							menu_ptr--;
							old_menu_index=0;
							is_new_screen=true;
							busy_wait_ms(250);
							//clear_input();
							continue;
						}						
						/*--Return from Menu--*/
						if(((KBD_DOWN)||(data_joy==D_JOY_DOWN))&&(fast_menu_index<(short int)fast_menu_lines[(uint8_t)fast_mode[(uint8_t)fast_mode_ptr]])){ fast_menu_index++; need_redraw=true;}
						if(((KBD_UP)||(data_joy==D_JOY_UP))&&(fast_menu_index>=0)){fast_menu_index--;need_redraw=true;}
						//начало и конец списка
						if(((KBD_PAGE_DOWN)||(data_joy==D_JOY_RIGHT))&&(fast_menu_index<(short int)fast_menu_lines[(uint8_t)fast_mode[(uint8_t)fast_mode_ptr]])){fast_menu_index+=3;need_redraw=true;}
						if(((KBD_PAGE_UP)||(data_joy==D_JOY_LEFT))&&(fast_menu_index>0)){fast_menu_index-=3;need_redraw=true;}
						if (fast_menu_index<0) fast_menu_index=(short int)fast_menu_lines[(uint8_t)fast_mode[(uint8_t)fast_mode_ptr]]-1;
						if (fast_menu_index>=(short int)fast_menu_lines[(uint8_t)fast_mode[(uint8_t)fast_mode_ptr]]) fast_menu_index=0;
						if((KBD_ENTER)||(data_joy==D_JOY_A)){
							need_redraw=true;
							if(fast_mode[fast_mode_ptr]==FAST_MENU_MAIN){
								switch (fast_menu_index){
									case FAST_MAIN_MANAGER: //Main menu
										menu_ptr++;
										menu_mode[menu_ptr]=MENU_MAIN;
										fast_mode_ptr=0;
										fast_menu_index=2;
										is_new_screen=true;
										continue;
										break;
									case FAST_GAME_A: //Start game A
										softkey_Game_A_pressed=true;
										softkey_duration = GW_REFRESH_RATE;
										resume_pause();
										menu_mode[menu_ptr]=EMULATION;
										continue;
										break;
									case FAST_GAME_B: //Start game B
										softkey_Game_B_pressed=true;
										softkey_duration = GW_REFRESH_RATE;
										resume_pause();
										menu_mode[menu_ptr]=EMULATION;
										continue;
										break;
									case FAST_TIME: //Goto Time
										softkey_time_pressed=true;
										softkey_duration = GW_REFRESH_RATE;
										resume_pause();
										menu_mode[menu_ptr]=EMULATION;
										continue;
										break;
									case FAST_ALARM: //Set Alarm
										softkey_alarm_pressed=true;
										softkey_duration = GW_REFRESH_RATE;
										resume_pause();
										menu_mode[menu_ptr]=EMULATION;
										continue;
										break;
									case FAST_MAIN_HELP: //HELP
										menu_ptr++;
										old_menu_index = fast_menu_index;
										menu_mode[menu_ptr]=MENU_HELP;
										is_new_screen=true;
										continue;
										break;
									case FAST_MAIN_SETTINGS: //SETTINGS
										menu_ptr++;
										old_menu_index = fast_menu_index;
										menu_mode[menu_ptr]=MENU_SETTINGS;
										is_new_screen=true;
										continue;
										break;
									case FAST_MAIN_SOFT_RESET: //Soft reset
										if (gw_program!=NULL){
											resume_pause();
											gw_system_reset();
											start_watch_mode();
										}
										menu_mode[menu_ptr]=EMULATION;
										continue;
										break;
									case FAST_MAIN_HARD_RESET: //Hard reset
										software_reset();
										break;
									default:
										break;
								}
							}
						}
						if(need_redraw){
							draw_fast_menu((SCREEN_W/2)-(9*FONT_W),(SCREEN_H/2)-(5*FONT_H),false,fast_mode[fast_mode_ptr],fast_menu_index);
							need_redraw=false;
							clear_input();
						}
						continue;
					break;
					case MENU_HELP:
						if (is_new_screen){
							////printf("is_new_screen\n");
							draw_main_window();	
							is_new_screen=false;
							busy_wait_ms(150);
							clear_input();
						}
						/*--Return from Menu--*/
						if((KBD_F12)||(KBD_ESC)||(KBD_F1)||((data_joy&D_JOY_START)&&(hat_switch==0))||(KBD_HOME)||(KBD_L_WIN)||(KBD_R_WIN)){
							menu_ptr--;
							need_redraw=true;
							is_new_screen=true;
							clear_input();
							continue;
						}
						if((KBD_F3)||(data_joy==D_JOY_B)){
							help_lng=1;
							need_redraw=true;
							is_new_screen=true;
							clear_input();
							continue;
						}
						if((KBD_F2)||(data_joy==D_JOY_A)){
							help_lng=0;
							need_redraw=true;
							is_new_screen=true;
							clear_input();
							continue;
						}													

						/*--Return from Menu--*/
						if(((KBD_UP)||(data_joy==D_JOY_UP))&&(lineStart>0)){lineStart--;need_redraw=true;};
						if(((KBD_DOWN)||(data_joy==D_JOY_DOWN))&&(lineStart<HELP_LINES)){lineStart++;need_redraw=true;};
						if(((KBD_PAGE_UP)||(KBD_LEFT)||(data_joy==D_JOY_LEFT))&&(lineStart>0)){lineStart-=5;need_redraw=true;}
						if(((KBD_PAGE_DOWN)||(KBD_RIGHT)||(data_joy==D_JOY_RIGHT))&&(lineStart<HELP_LINES)){lineStart+=5;need_redraw=true;}
						if(lineStart<0){lineStart=0;};
						if(lineStart>(HELP_LINES-SCREEN_HELP_LINES)){lineStart=(HELP_LINES-SCREEN_HELP_LINES);};
						////printf("need_redraw:%d lineStart:%d\n",need_redraw,lineStart);
						if (data_joy>0){ //***********************
							old_data_joy=0;
							if(Joystics.Present_WII_joy){
								Wii_clear_old();
							}
						};
						if(need_redraw){
							draw_help_text(lineStart,help_lng);
							short int linePos=lineStart;
							if (linePos<0) linePos=0;
							if (linePos>(HELP_LINES-SCREEN_HELP_LINES)) linePos=(HELP_LINES-SCREEN_HELP_LINES);
							short int shft=(207*linePos)/(HELP_LINES-SCREEN_HELP_LINES); 
							draw_rect(FONT_W+1+FONT_W*37,PREVIEW_POS_Y-1,FONT_W,SCREEN_H-(FONT_H*3)+2,COLOR_MAIN_RAMK,false);//Рамка полосы прокрутки
							draw_rect(FONT_W+2+FONT_W*37,PREVIEW_POS_Y,FONT_W-2,SCREEN_H-(FONT_H*3),COLOR_BACKGOUND,true);  //Заливка фона полосы прокрутки
							draw_rect(FONT_W+3+FONT_W*37,shft+17,4,5,COLOR_TEXT,true);  //указатель полосы прокрутки
							if (conv_utf_cp866(help_head[help_lng], temp_msg, strlen(help_head[help_lng]))>0){
								draw_text5x7_len((23*FONT_5x7_W),SCREEN_H-FONT_5x7_H,temp_msg,CL_BLACK,CL_WHITE,18);
							}
							need_redraw=false;
							clear_input();
							continue;
						}
					break;
					case MENU_SETTINGS:
						if (is_new_screen){
							draw_config_menu((SCREEN_W/2)-(14*FONT_W),(SCREEN_H/2)-((CONFIG_MENU_ITEMS/2)*FONT_H),true,settings_index);
							is_new_screen=false;
							need_redraw=true;
							busy_wait_ms(150);
							clear_input();
						}
						if((KBD_F12)||(KBD_ESC)||(data_joy&D_JOY_START)||(KBD_HOME)||(KBD_L_WIN)||(KBD_R_WIN)){
							fast_menu_index=old_menu_index;
							menu_ptr--;
							old_menu_index=0;
							is_new_screen=true;
							clear_input();
							continue;
						}
						if((KBD_UP)||(data_joy==D_JOY_UP)){settings_index--;need_redraw=true;}
						if((KBD_DOWN)||(data_joy==D_JOY_DOWN)){settings_index++;need_redraw=true;}
						if((KBD_LEFT)||(data_joy==D_JOY_LEFT)){menu_inc_dec=-1;need_redraw=true;}
						if((KBD_RIGHT)||(data_joy==D_JOY_RIGHT)){menu_inc_dec=1;need_redraw=true;}
						/*if(KBD_PRESS){
							//printf("settings_index:%d   menu_inc_dec:%d   settings_lines:%d\n",settings_index,menu_inc_dec,settings_lines);
						}*/
						if (settings_index<0){settings_index=settings_lines;}
						if (settings_index>settings_lines){settings_index=0;}
						if ((menu_inc_dec!=0)&&(need_redraw==true)){
								if (settings_index==0){ //Boot go to :
									cfg_boot_scr+=menu_inc_dec;
									if((cfg_boot_scr>MAX_CFG_BOOT_SCR_MODE)&&(menu_inc_dec>0)){
										cfg_boot_scr=0;
									}
									if((cfg_boot_scr>MAX_CFG_BOOT_SCR_MODE)&&(menu_inc_dec<0)){
										cfg_boot_scr=MAX_CFG_BOOT_SCR_MODE;
									}
								}							
								if (settings_index==1){ //Head Up Display:
									cfg_hud_enable+=menu_inc_dec;
									if((cfg_hud_enable>MAX_CFG_HUD_MODE)&&(menu_inc_dec>0)){
										cfg_hud_enable=0;
									}
									if((cfg_hud_enable>MAX_CFG_HUD_MODE)&&(menu_inc_dec<0)){
										cfg_hud_enable=MAX_CFG_HUD_MODE;
									}
									if(cfg_hud_enable!=current_hud_mode){
										current_hud_mode&=~0x03;
										current_hud_mode|=cfg_hud_enable;
									}
								}							
								if (settings_index==2){ //Sound out mode:
									cfg_sound_out_mode+=menu_inc_dec;
									if((cfg_sound_out_mode>MAX_CFG_OUT_MODE)&&(menu_inc_dec>0)){
										cfg_sound_out_mode=0;
									}
									if((cfg_sound_out_mode>MAX_CFG_OUT_MODE)&&(menu_inc_dec<0)){
										cfg_sound_out_mode=MAX_CFG_OUT_MODE;
									}
								}
								if (settings_index==3){ //Soft Sound Vol:
									cfg_volume+=(menu_inc_dec*CFG_VOLUME_STEP);
									if((cfg_volume>MAX_CFG_VOLUME_MODE)&&(menu_inc_dec>0)){
										cfg_volume=MAX_CFG_VOLUME_MODE;
									}
									if((cfg_volume<0)&&(menu_inc_dec<0)){
										cfg_volume=0;
									}
								}
								if (settings_index==4){ //Video output:
									cfg_video_out+=(uint8_t)menu_inc_dec;
									if((cfg_video_out>MAX_CFG_VIDEO_MODE)&&(menu_inc_dec>0)){
										cfg_video_out=0;
									}
									if((cfg_video_out>MAX_CFG_VIDEO_MODE)&&(menu_inc_dec<0)){
										cfg_video_out=MAX_CFG_VIDEO_MODE;
									}
									//printf("cfg_video_out:%d\n",cfg_video_out);
									//if((g_out)cfg_video_out<g_out_TFT_ST7789){
										//printf("Test framerate begin \n");
										if(current_video_out!=cfg_video_out){
											cfg_frame_rate=0;
											uint8_t test=10;
											while(!graphics_try_framerate((g_out) cfg_video_out,(fr_rate) cfg_frame_rate, false)){
												cfg_frame_rate+=1;
												if((cfg_frame_rate>MAX_CFG_VIDEO_FREQ_MODE)){
													cfg_frame_rate=0;
												}
												test--;
												if(test==0) break;
											}
										}
										//printf("Test framerate end \n");
									//}
								}	
								if (settings_index==5){ //Video framerate:
									cfg_frame_rate+=menu_inc_dec;
									if((cfg_frame_rate>MAX_CFG_VIDEO_FREQ_MODE)&&(menu_inc_dec>0)){
										cfg_frame_rate=0;
									}
									if((cfg_frame_rate>MAX_CFG_VIDEO_FREQ_MODE)&&(menu_inc_dec<0)){
										cfg_frame_rate=MAX_CFG_VIDEO_FREQ_MODE;
									}
									uint8_t test=10;
									while(!graphics_try_framerate((g_out) cfg_video_out,(fr_rate) cfg_frame_rate, false)){
										cfg_frame_rate+=1;
										if((cfg_frame_rate>MAX_CFG_VIDEO_FREQ_MODE)){
											cfg_frame_rate=0;
										}
										test--;
										if(test==0) break;
									}
								}
								if (settings_index==6){ //Mobile Murmulator:
									cfg_mobile_mode+=menu_inc_dec;
									if((cfg_mobile_mode>MAX_CFG_MOBILE_MODE)&&(menu_inc_dec>0)){
										cfg_mobile_mode=MAX_CFG_MOBILE_MODE;
									}
									if((cfg_mobile_mode>MAX_CFG_MOBILE_MODE)&&(menu_inc_dec<0)){
										cfg_mobile_mode=DEF_CFG_MOBILE_MODE;
									}
								}								
								#ifdef VGA_HDMI
								if((g_out)cfg_video_out>g_out_HDMI){
									if (settings_index==7){ //LCD BrightLev:
										cfg_brightness+=menu_inc_dec;
										if((cfg_brightness>MAX_CFG_BRIGHT_MODE)&&(menu_inc_dec>0)){
											cfg_brightness=MAX_CFG_BRIGHT_MODE;
										}
										if((cfg_brightness>MAX_CFG_BRIGHT_MODE)&&(menu_inc_dec<0)){
											cfg_brightness=0;
										}
										pwm_set_gpio_level(TFT_LED_PIN,(TFT_MIN_BRIGHTNESS+(cfg_brightness*10)));			//уровень подсветки TFT
									}
									if (settings_index==8){ //LCD Rotate:
										cfg_rotate+=menu_inc_dec;
										if((cfg_rotate>MAX_CFG_ROTATE)&&(menu_inc_dec>0)){
											cfg_rotate=MAX_CFG_ROTATE;
										}
										if((cfg_rotate>MAX_CFG_ROTATE)&&(menu_inc_dec<0)){
											cfg_rotate=0;
										}
									}	
									if (settings_index==9){ //LCD Inversion:
										cfg_inversion+=menu_inc_dec;
										if((cfg_inversion>MAX_CFG_INVERSION)&&(menu_inc_dec>0)){
											cfg_inversion=MAX_CFG_INVERSION;
										}
										if((cfg_inversion>MAX_CFG_INVERSION)&&(menu_inc_dec<0)){
											cfg_inversion=0;
										}
									}
									if (settings_index==10){ //LCD Pixel format:
										cfg_pixels+=menu_inc_dec;
										if((cfg_pixels>MAX_CFG_PIXELS)&&(menu_inc_dec>0)){
											cfg_pixels=MAX_CFG_PIXELS;
										}
										if((cfg_pixels>MAX_CFG_PIXELS)&&(menu_inc_dec<0)){
											cfg_pixels=0;
										}
									}
								}
								#endif
								menu_inc_dec=0;
							}
						if((KBD_ENTER)||(data_joy==D_JOY_A)){
							if(init_fs){
								if(settings_index==settings_lines-4){
									config_read();
									settings_index = 0;
									MessageBox("SETTINGS"," LOADED ",CL_LT_YELLOW,CL_BLUE,3);
									is_new_screen = true;
								}
								if(settings_index==settings_lines-3){
									config_write_defaults();
									config_read();
									settings_index = 0;
									MessageBox("SETTINGS"," RESET TO DEFAUTS ",CL_LT_YELLOW,CL_BLUE,2);
									is_new_screen = true;
								}
								if(settings_index==settings_lines-2){
									if((g_out)cfg_video_out>g_out_HDMI){
										cfg_lcd_video_out=cfg_video_out;
										//printf("Set vout: %d \n",cfg_video_out);
									}
									config_save();
									settings_index = 0;
									MessageBox("SETTINGS"," ALL SAVED ",CL_LT_YELLOW,CL_BLUE,4);
									if ((current_frame_rate!=cfg_frame_rate)||
										(current_video_out!=cfg_video_out)||
										(current_rotate!=cfg_rotate)||
										(current_inversion!=cfg_inversion)||
										(current_pixels!=cfg_pixels)||
										(current_sound_out!=cfg_sound_out_mode)||
										(current_mobile_mode!=cfg_mobile_mode)
									){
										//MessageBox("Critical settings changed!!!   ","        Please REBOOT!!!       ",CL_LT_YELLOW,CL_BLUE,1);
										MessageBox("SETTINGS","REBOOT DEVICE",CL_LT_YELLOW,CL_GRAY,1);
										busy_wait_ms(500);
										software_reset();
									}									
									is_new_screen = true;
								}
							}
							if(settings_index==settings_lines-1){
								MessageBox("SETTINGS","REBOOT DEVICE TO EXIT FIRMWARE UPDATE",CL_LT_GREEN,CL_BLUE,1);
								busy_wait_ms(500);
								reset_usb_boot(0,0);
							}
							if(settings_index==settings_lines){
								//MessageBox("SETTINGS","REBOOT DEVICE",CL_LT_BLUE,CL_YELLOW,1);
								MessageBox("SETTINGS","REBOOT DEVICE",CL_LT_YELLOW,CL_GRAY,1);
								busy_wait_ms(500);
								software_reset();
							}
						}
						/*if(((KBD_ESC)||(data_joy&D_JOY_START))){
								paused=10;
								is_fast_menu_mode=false;
								is_pause_mode=false;
								go_menu_mode=false;
								is_emulation_mode=true;
								zx_machine_enable_vbuf(true);
								//printf("exit settings");
								continue;
							}
						if((is_menu_mode)&&(kb_st_ps2.u[1]&KB_U1_ESC)){
								is_settings_mode=false;
								is_new_screen=true;
								go_menu_mode=true;		
								//printf("exit settings-esc");
								continue;					
							}
						*/
						if(need_redraw){
							draw_config_menu((SCREEN_W/2)-(14*FONT_W),(SCREEN_H/2)-((CONFIG_MENU_ITEMS/2)*FONT_H),false,settings_index);
							need_redraw=false;
							clear_input();
						}
						break;
					default:
						break;
				}//switch (menu_mode[menu_ptr])
				/*SHOW */
				if(menu_mode[menu_ptr]==MENU_MAIN){
					if((init_fs)&&(last_action>0)&&(my_millis()-last_action)>SHOW_SCREEN_DELAY){
						////printf("Timers1: LA:%d GB:%d\n",last_action,my_millis());
						last_action=0;
						//if (!files[cur_file_index][(FILE_NAME_LEN-1)]){
							scroll_lfn = true;
							scroll_action = my_millis();
							scroll_pos=0;
						/*} else {
							scroll_lfn = false;
							scroll_action = 0;
							scroll_pos=0;
						}*/
						draw_rect(PREVIEW_POS_X,SCREEN_H-(FONT_H*4),PREVIEW_WIDTH,(FONT_H*3),COLOR_BACKGOUND,true);//Фон отображения информации о файле //COLOR_BACKGOUND
						//const char* ext = get_file_extension(files[cur_file_index]);
						file = (FileRec*)&files[cur_file_index];
						memset(temp_msg,0x00,sizeof(temp_msg));
						strncpy(temp_msg,file->filename,FILE_NAME_LEN-1);
						const char* ext = get_file_extension(temp_msg);
						////printf("ext:%s\n",ext);
						strcpy(activefilename,dir_path);
						strcat(activefilename,"/");
						strcat(activefilename,current_lfn);
						display_file_index=cur_file_index;
						if(strcasecmp(ext, "gwm")==0) {
							//printf("LoadScreenshot: %s\n",activefilename);
							//if(LoadScreenFromZ80Snapshot(activefilename)) //printf("show - OK \n"); else //printf("screen not found");
							if(!LoadScreenFromGWNSnapshot(activefilename)){
								draw_rect(PREVIEW_POS_X,PREVIEW_POS_Y,PREVIEW_WIDTH,PREVIEW_HEIGHT,COLOR_PIC_BG,true);
								draw_mur_logo();
							} else {
								reload_last_rom=true;
							}
							continue;
						} else
						if(strcasecmp(ext, "txt") == 0) {
							//printf("LoadScreenshot: %s\n",activefilename);
							draw_rect(PREVIEW_POS_X,PREVIEW_POS_Y,PREVIEW_WIDTH,PREVIEW_HEIGHT,COLOR_PIC_BG,true);
							if(!LoadTxt(activefilename)){
								draw_rect(PREVIEW_POS_X,PREVIEW_POS_Y,PREVIEW_WIDTH,PREVIEW_HEIGHT,COLOR_PIC_BG,true);
								draw_mur_logo();
							}
							continue;
						} else
						if(strcasecmp(ext, "cfg") == 0) {
							//printf("LoadScreenshot: %s\n",activefilename);
							draw_rect(PREVIEW_POS_X,PREVIEW_POS_Y,PREVIEW_WIDTH,PREVIEW_HEIGHT,COLOR_PIC_BG,true);
							if(!LoadTxt(activefilename)){
								draw_rect(PREVIEW_POS_X,PREVIEW_POS_Y,PREVIEW_WIDTH,PREVIEW_HEIGHT,COLOR_PIC_BG,true);
								draw_mur_logo();
							}
							continue;
						} else {
							if (display_file_index==-1){
								draw_rect(PREVIEW_POS_X,PREVIEW_POS_Y,PREVIEW_WIDTH,PREVIEW_HEIGHT,COLOR_BACKGOUND,true);//Фон отображения скринов						
								draw_mur_logo_big(155,60,1);
							} else {
								draw_rect(PREVIEW_POS_X,PREVIEW_POS_Y,PREVIEW_WIDTH,PREVIEW_HEIGHT,COLOR_PIC_BG,true);	
								draw_mur_logo();
								if((!(file->attr&TOP_DIR_ATTR))&&(!(file->attr&AM_DIR))){
									//printf("z>%s    %02X\n",temp_msg,file->attr);
									file_descr = sd_open_file(&sd_file,activefilename,FA_READ);
									if (file_descr!=FR_OK){sd_close_file(&sd_file);return false;}
									memset(temp_msg, 0, sizeof(temp_msg));
									sprintf(temp_msg,"FSize: %ldKb",(long)sd_file_size(&sd_file)/1024);
									draw_text_len(PREVIEW_POS_X,SCREEN_H-(FONT_H*3), temp_msg,COLOR_TEXT,COLOR_BACKGOUND,22);
									sd_close_file(&sd_file);
									memset(temp_msg, 0, sizeof(temp_msg));
								}
							};
							display_file_index=cur_file_index;
							////printf("Draw Mur Logo \n");
						}
					}

					if((init_fs)&&(scroll_lfn)&&(my_millis()-scroll_action)>(SHOW_SCREEN_DELAY/2)){
						scroll_action = my_millis();
						//uint8_t pos = cur_file_index-shift_file_index;
						if (strlen(current_lfn)>24){
							if(scroll_pos<(strlen(current_lfn)-23)){
								strncpy(temp_msg,current_lfn+scroll_pos,24);
								scroll_pos++;
							} else {
								scroll_pos=0;
							}
						} else {
							strncpy(temp_msg,current_lfn,23);
						}
						//draw_text_len(2*FONT_W,2*FONT_H+pos*FONT_H,temp_msg,COLOR_SELECT_TEXT,COLOR_CURRENT_BG,(FILE_NAME_LEN-1));
						draw_text_len(PREVIEW_POS_X,SCREEN_H-PREVIEW_POS_Y, temp_msg,COLOR_TEXT,COLOR_FILE_BG,24);//COLOR_BACKGOUND
					}
				}
				////printf("mel>%d\n",my_millis()-main_loop);
				//main_loop = my_millis();				
				if(ticker>131070){ticker=0;}
			}while(menu_mode[menu_ptr]<EMULATION); //while(1) main menu loop

		}
		//END MENU LOOP
		
		#ifdef DEBUG_BLINK
		if((my_millis()-timer_update)>TIMER_PERIOD){
			timer_update=my_millis();
			gpio_put(WORK_LED_PIN,1);
			////printf("Time is:%d \n",my_millis());
			busy_wait_ms(50);
			gpio_put(WORK_LED_PIN,0);
			busy_wait_ms(50);
		};
		#endif



		if(menu_mode[menu_ptr]==EMULATION){
			memset(hud_line,0x11,SCREEN_W);
			//graphics_set_hud_handler(hud_ptr[current_hud]);
			//printf("HM1>[%04X]\n",current_hud_mode);
			if(current_hud_mode&HM_ON){
				current_hud_mode|=HM_MAIN_HUD;
				old_hud_mode=current_hud_mode;
				hud_timer=0;
			};
			if(current_hud_mode&HM_TIME){
				current_hud_mode|=HM_MAIN_HUD;
				old_hud_mode=current_hud_mode;
				hud_timer = my_millis();
			};
			if(current_hud_mode&HM_SHOW_BATTERY)	{hud_ptr=&hud_battery;};
			if(current_hud_mode&HM_SHOW_VOLUME)		{hud_ptr=&hud_scale;};
			if(current_hud_mode&HM_SHOW_BRIGHT)		{hud_ptr=&hud_scale;};
			if(current_hud_mode&HM_SHOW_KEYLOCK)	{hud_ptr=&hud_kb_lock;};
			graphics_set_hud_handler(hud_ptr);
			current_hud_mode=old_hud_mode;
			//printf("HM2>[%04X]\n",current_hud_mode);
			//if(strlen(activefilename)==0){
			//	memcpy()
			//}
			if((gw_program!=NULL)&&(reload_last_rom)&&(strlen(romfilename)>0)){
				if(gw_romloader(romfilename)){
					gw_assign_ptrs();
					//gw_dump_struct();
					printf("Reload ROM - OK\n");
				}
			}
			
			if ((gw_program==NULL)||((reload_last_rom)&&(strlen(romfilename)==0))){
				memcpy(&gw_head, default_game, sizeof(gw_head));
				//dump_struct();
				uint32_t rom_size_dest = gw_head.prewiew_palette;
				//printf("rom_size_dest:[%d]\n",rom_size_dest);
				memcpy(game_buff, default_game, rom_size_dest);
				gw_assign_ptrs();
				//gw_dump_struct();

				updatePaletteGame((uint8_t*)gw_background_pal, gw_head.background_palette_size);
				gw_set_background();

				//printf("PreRender BG\n");
				//gw_draw = true;

				/*** Configure the emulated system ***/
				if(!reload_last_rom){
					if(gw_system_config()){
						printf("G&W configured\n");
						// Start and Reset the emulated system 
						gw_system_start();
						printf("G&W start\n");
						gw_system_reset();
						printf("G&W reset\n");
						start_watch_mode();
			   		}
   				}
			} else {
				updatePaletteGame((uint8_t*)gw_background_pal, gw_head.background_palette_size);
				gw_set_background();				
			}


			menu_ptr=0;
			fast_mode_ptr=0;
			/*if(old_show_hud!=show_hud){
				show_hud=old_show_hud;
			}*/

			printf("Enter Emulation Mode\n");
			//zx_machine_enable_vbuf(true);
			if (emu_paused){
				resume_pause();				
			}


			if(hat_switch==0) clear_input();
			//BEGIN EMULATION LOOP
			main_loop = time_us_32();
			//printf(">hat_switch:[%02X]\n",hat_switch);
			ticker=0;

			//gw_system_blit(&graph_buf[0]);

			do{
				//printf("tap_loader_active>[%04X]\n",tap_loader_active);
				ticker++;

				//printf("ticker:[%d]\n",ticker);
				//gpio_put(WORK_LED_PIN,1);
				/*if ((ticker%1024)==0){
					
					//printf("beep\n");
					//printf("%d\t\t%d\t\t%d\t\t%d\t\t%d\t\t%d\t\t%d\t\t%d\n",mixL,mixR,outL,outR,beeper_signed,tape_signed,beep_data,beep_data_old);
					//printf("HM1>[%04X] %d\n",current_hud_mode,hud_timer);
				}*/
				if ((ticker%262144)==0){
					if(current_hud_mode&HM_SHOW_BATTERY){
						get_battery_stats();
					}
				}
			
				/*--Show Volume Ind--*/
				if((current_hud_mode&HM_SHOW_BRIGHT)||(current_hud_mode&HM_SHOW_VOLUME)){
					if(current_hud_mode&HM_SHOW_VOLUME){
						if(cfg_volume<=MAX_CFG_VOLUME_MODE){
							vol_strength=3;
						}
						if(cfg_volume<216){
							vol_strength=2;
						}
						if(cfg_volume<56){
							vol_strength=1;
						}
						if(cfg_volume<1){
							vol_strength=0;
						}						
						//vol_bank = AY_get_ampl();
					}					
					if((hud_timer>0)&&(my_millis()-hud_timer)>(SHOW_SCREEN_DELAY)){
						hud_timer=0;
						printf("Vol/Bright timer off\n");
						current_hud_mode=old_hud_mode;
						current_hud_mode&=~HM_SHOW_VOLUME;
						current_hud_mode&=~HM_SHOW_BRIGHT;
						//printf("HM3>[%04X]\n",current_hud_mode);
					}

				};
				/*--Show Volume Ind--*/

				/*--KEYBOARD LOCK--*/
				if(current_hud_mode&HM_SHOW_KEYLOCK){
					if(kbd_lock){
						if((hud_timer>0)&&(my_millis()-hud_timer)>(SHOW_SCREEN_DELAY*2)){
							if(cfg_mobile_mode==MOBILE_MURM_ON){
								pwm_set_gpio_level(TFT_LED_PIN,0);			//уровень подсветки TFT
							}
							printf("Screen OFF\n");
							hud_timer=0;
							continue;
						}
					} else {
						if(cfg_mobile_mode==MOBILE_MURM_ON){
							pwm_set_gpio_level(TFT_LED_PIN,(TFT_MIN_BRIGHTNESS+(cfg_brightness*10)));			//уровень подсветки TFT
						}
						if((hud_timer>0)&&(my_millis()-hud_timer)>(SHOW_SCREEN_DELAY*2)){
							printf("Screen ON\n");
							current_hud_mode&=~HM_SHOW_KEYLOCK;
							hud_timer=0;
							continue;
						}
					}

				}
				/*--KEYBOARD LOCK--*/



				/*--HUD Switch--*/
				if((current_hud_mode&HM_MAIN_HUD)){
					if((hud_timer>0)&&(my_millis()-hud_timer)>(SHOW_SCREEN_DELAY*4)){ 
						
						//printf("Main/Tape timer off\n");
						current_hud_mode=old_hud_mode;
						if(current_hud_mode&HM_ON){
							if(current_hud_mode&HM_SHOW_VOLUME)		{hud_ptr=&hud_scale;};
							if(current_hud_mode&HM_SHOW_BRIGHT)		{hud_ptr=&hud_scale;};
							if(current_hud_mode&HM_SHOW_KEYLOCK)	{hud_ptr=&hud_kb_lock;};

						} else if(current_hud_mode&HM_TIME){
							//printf("HM5>[%04X]\n",current_hud_mode);
							current_hud_mode&=~HM_MAIN_HUD;
							hud_ptr=NULL;
						}
					}
				}
				if(current_hud_mode!=old_hud_mode){
					if(current_hud_mode==HM_OFF)			{hud_ptr=NULL;};
					if(current_hud_mode&HM_SHOW_BATTERY)	{hud_ptr=&hud_battery;};
					if(current_hud_mode&HM_SHOW_VOLUME)		{hud_ptr=&hud_scale;};
					if(current_hud_mode&HM_SHOW_BRIGHT)		{hud_ptr=&hud_scale;};
					if(current_hud_mode&HM_SHOW_KEYLOCK)	{hud_ptr=&hud_kb_lock;};	

					graphics_set_hud_handler(hud_ptr);
					old_hud_mode=current_hud_mode;
					//printf("HM6>[%04X]\n",current_hud_mode);
					//printf("tape_disp>[%04d]\n",tape_disp);
					
				}
				/*--HUD Switch--*/

				if ((ticker%32)==0){ //4608
					process_input();
					if(!kbd_lock){
						//memset(zx_write_buffer->kb_data,0,8);
					}
					/*if((keyPressed)&&(menu_mode[menu_ptr]==EMULATION)){
						
					}*/
					
					/*
					if(KBD_PRESS){
						memset(temp_msg,0,sizeof(temp_msg));
						keys_to_str(temp_msg,' ',kb_st_ps2);
						//printf("0> kbd[0][%08lX]   kbd[1][%08lX]   kbd[2][%08lX]   kbd[2][%08lX] [%s]\n",kb_st_ps2.u[0],kb_st_ps2.u[1],kb_st_ps2.u[2],kb_st_ps2.u[3],temp_msg);
						printf("key>[%s]\n",temp_msg);
					}
					*/

					/*
					//DUMP KBD
					printf("i2cKbdMode:%d  ",i2cKbdMode);
					for (uint8_t i=0;i<8;i++){
						printf("kb[%d]:[%02X]   ",i,zx_write_buffer->kb_data[i]);
					}
					for (uint8_t i=0;i<4;i++){
						printf("ps[%d]:[%08X]   ",i,kb_st_ps2.u[i]);
					}
					printf("\n");
					*/
					/*
						#define HAT_UP		1>>1;
						#define HAT_DOWN	1>>2;
						#define HAT_LEFT	1>>3;
						#define HAT_RIGHT	1>>4;
						#define HAT_A		1>>5;
						#define HAT_B		1>>6;
					*/
					//hat_switch=0;

					/*switch input mode*/

					if((joy_pressed)){
						hat_switch|=hat_switch_process((uint16_t)data_joy);
						hat_switch|=hat_switch_process((uint16_t)(data_joy>>16));
					}
					if(!kbd_lock){
					/*switch input mode*/
						//printf("data_joy:[%08X]\that_locked:[%02X]\n",data_joy,hat_locked);
						if(hat_switch==0){
							if(KBD_RIGHT){
								data_joy|=D_JOY_RIGHT;
								////printf("KBD Right\n");
							};
							if(KBD_LEFT){
								data_joy|=D_JOY_LEFT;
								////printf("KBD Left\n");
							};
							if(KBD_DOWN){
								data_joy|=D_JOY_DOWN;
								////printf("KBD Down\n");
							};
							if(KBD_UP){
								data_joy|=D_JOY_UP;
								////printf("KBD Up\n");
							};
							if((KBD_X)||(KBD_R_ALT)||(KBD_R_SHIFT)||(KBD_NUM_PERIOD)||(KBD_DELETE)){
								data_joy|=D_JOY_A;
								////printf("KBD Alt\n");
							};
							if((KBD_Z)||(KBD_R_ALT)||(KBD_R_SHIFT)||(KBD_NUM_PERIOD)||(KBD_DELETE)){
								data_joy|=D_JOY_B;
								////printf("KBD Alt\n");
							};
							if((KBD_Q)){
								hat_switch|=HAT_START;
								hat_switch|=HAT_A;
							};
							if((KBD_W)){
								hat_switch|=HAT_START;
								hat_switch|=HAT_B;
							};
						}
						//printf("u[0]:[%08lX]\tu[1]:[%08lX]\tu[2]:[%08lX]\tu[3]:[%08lX]\n",kb_st_ps2.u[0],kb_st_ps2.u[1],kb_st_ps2.u[2],kb_st_ps2.u[3]);
						
						/*----------process keyboard-----------*/
						//convert_kb_u_to_kb_zx(&kb_st_ps2,zx_write_buffer->kb_data);

						/*
						if(hat_locked>0){
							if(((data_joy&D_JOY_HAT_UNLOCK)==D_JOY_HAT_UNLOCK)||(((data_joy>>16)&D_JOY_HAT_UNLOCK)==D_JOY_HAT_UNLOCK)){
								hat_locked=0;
								menu_ptr++;
								menu_mode[menu_ptr]=MENU_JOY_MAIN;
								rel_data_joy=data_joy;
								break;
							}
						}
						*/
						if((!joy_pressed)&&((rel_data_joy&D_JOY_START)||((rel_data_joy>>16)&D_JOY_START))&&(hat_switch==0)){
							menu_ptr++;
							menu_mode[menu_ptr]=MENU_JOY_MAIN;
							rel_data_joy=data_joy;
							break;
						}
					}
					/*
					printf("[%010ld]>>> ZX>KB[%02X][%02X][%02X][%02X][%02X][%02X][%02X][%02X] KEMP["BYTE_TO_BINARY_PATTERN"] *** EXT>KB[%08lX][%08lX][%08lX][%08lX]:[%02X]  JOY["BYTE_TO_BINARY_PATTERN"]["BYTE_TO_BINARY_PATTERN"]["BYTE_TO_BINARY_PATTERN"]\n", 
							time_us_32()-main_loop,
							zx_write_buffer->kb_data[0],zx_write_buffer->kb_data[1],zx_write_buffer->kb_data[2],zx_write_buffer->kb_data[3],zx_write_buffer->kb_data[4],zx_write_buffer->kb_data[5],zx_write_buffer->kb_data[6],zx_write_buffer->kb_data[7],
							BYTE_TO_BINARY(zx_write_buffer->kempston),
							kb_st_ps2.u[0],kb_st_ps2.u[1],kb_st_ps2.u[2],kb_st_ps2.u[3],kb_st_ps2.state,
							BYTE_TO_BINARY(data_joy),BYTE_TO_BINARY(old_data_joy),BYTE_TO_BINARY(rel_data_joy));
					*/
					main_loop = time_us_32();
					rel_data_joy=data_joy;
					ack_input=false;
				} else {
					if(!kbd_lock){					
						/*------SET INPUT------*/
						//zx_machine_input_set();
					}
				}
				if(hat_switch>0){
					if(current_hud_mode&HUD_TIME){hud_timer = my_millis();};
					//printf(">hat_switch:[%02X]\n",hat_switch);
					if((hat_switch&HAT_START)){
						if(hat_switch&HAT_UP){
							hat_switch&=~HAT_UP;
							softkey_only=true;
							softkey_time_pressed=true;
							softkey_duration = GW_REFRESH_RATE;
							gw_system_run(GW_SYS_FREQ/2);
							softkey_only=false;
						}
						if(hat_switch&HAT_DOWN){
							hat_switch&=~HAT_DOWN;
							hat_switch&=~HAT_UP;
							softkey_only=true;
							softkey_alarm_pressed=true;
							softkey_duration = GW_REFRESH_RATE;
							gw_system_run(GW_SYS_FREQ/2);
							softkey_only=false;
						}
						if(hat_switch&HAT_A){
							hat_switch&=~HAT_A;
							softkey_only=true;
							softkey_Game_A_pressed=true;
							softkey_duration = GW_REFRESH_RATE;
							gw_system_run(GW_SYS_FREQ/2);
							softkey_only=false;
							//resume_pause();
							continue;
						}
						if(hat_switch&HAT_B){
							hat_switch&=~HAT_B;
							softkey_only=true;
							softkey_Game_B_pressed=true;
							softkey_duration = GW_REFRESH_RATE;
							gw_system_run(GW_SYS_FREQ/2);
							softkey_only=false;
							//resume_pause();
							continue;
						}
					}
					
					if(hat_switch&HAT_SELECT){

						if(hat_switch&HAT_UP){
							hat_switch&=~HAT_UP;
							kb_st_ps2.u[2]|=KB_U2_NUM_PLUS;
							busy_wait_ms(50);
							//memset(zx_write_buffer->kb_data,0,8);
							printf("Vol UP\n");
						}
						if(hat_switch&HAT_DOWN){
							hat_switch&=~HAT_DOWN;
							kb_st_ps2.u[2]|=KB_U2_NUM_MINUS;
							busy_wait_ms(50);
							//memset(zx_write_buffer->kb_data,0,8);
							printf("Vol DOWN\n");
						}
						#ifdef VGA_HDMI
						if(cfg_mobile_mode==MOBILE_MURM_ON){
							if(hat_switch&HAT_RIGHT){
								hat_switch&=~HAT_RIGHT;
								cfg_brightness+=1;
								if(cfg_brightness>MAX_CFG_BRIGHT_MODE){
									cfg_brightness=MAX_CFG_BRIGHT_MODE;
								}
								pwm_set_gpio_level(TFT_LED_PIN,(TFT_MIN_BRIGHTNESS+(cfg_brightness*10)));			//уровень подсветки TFT
								hud_timer = my_millis();
								current_hud_mode|=HM_SHOW_BRIGHT;
								current_hud_mode&=~HM_SHOW_VOLUME;
								printf("Bri UP\n");
								busy_wait_ms(150);
							}
							if(hat_switch&HAT_LEFT){
								hat_switch&=~HAT_LEFT;
								cfg_brightness-=1;
								if(cfg_brightness>MAX_CFG_BRIGHT_MODE){
									cfg_brightness=0;
								}
								pwm_set_gpio_level(TFT_LED_PIN,(TFT_MIN_BRIGHTNESS+(cfg_brightness*10)));			//уровень подсветки TFT
								hud_timer = my_millis();
								current_hud_mode|=HM_SHOW_BRIGHT;
								current_hud_mode&=~HM_SHOW_VOLUME;
								printf("Bri DOWN\n");
								busy_wait_ms(150);
							}
							if(hat_switch&HAT_A){
								//hat_switch&=~HAT_A;
								hat_switch=0;
								kbd_lock=true;
								hud_timer = my_millis();
								current_hud_mode|=HM_SHOW_KEYLOCK;
								printf("KB LOCK\n");
								busy_wait_ms(50);
								continue;
							}
							if(hat_switch&HAT_B){
								//hat_switch&=~HAT_B;
								hat_switch=0;
								kbd_lock=false;
								hud_timer = my_millis();
								current_hud_mode|=HM_SHOW_KEYLOCK;
								printf("KB UNLOCK\n");
								busy_wait_ms(50);
								continue;
							}
						}
						#endif					
					}
					
				}
				//printf("["BYTE_TO_BINARY_PATTERN"]["BYTE_TO_BINARY_PATTERN"]["BYTE_TO_BINARY_PATTERN"]\n",BYTE_TO_BINARY(hat_switch),BYTE_TO_BINARY(rel_data_joy),BYTE_TO_BINARY(joy_pressed));
				
				/*if ((ticker%32768)==0){
					printf("E> joy_pressed:%d   data_joy:[%02X]   old_data_joy:[%02X]   rel_data_joy:[%02X]   hat_switch:[%02X] \n",joy_pressed,data_joy,old_data_joy,rel_data_joy,hat_switch);
				}*/

				if((!joy_pressed)&&(hat_switch>0)){
					hat_switch=0;
					clear_input();
				}

				/*
				if ((ticker%256)==0){
				//if((data_joy!=0)||(KBD_PRESS)){
					printf("kbd[0][%08lX]   kbd[1][%08lX]   kbd[2][%08lX]   kbd[3][%08lX] data_joy:[%02X]\n",kb_st_ps2.u[0],kb_st_ps2.u[1],kb_st_ps2.u[2],kb_st_ps2.u[3],data_joy);
					//printf("menu_mode[menu_ptr]>%d   menu_ptr>%d   fast_mode[fast_mode_ptr]>%d   fast_mode_ptr>%d   fast_menu_index>%d   old_menu_index>%d\n",menu_mode[menu_ptr],menu_ptr,fast_mode[fast_mode_ptr],fast_mode_ptr,fast_menu_index,old_menu_index);
				}*/
				/* hotkey process*/
				if(!kbd_lock)
				if(KBD_PRESS){
					/*if (KBD_SCROLL_LOCK) {
						is_skipframe^=1;
						//printf("Skip frame:%d\n",is_skipframe);
						zx_machine_slow_FR(is_skipframe);
					}*/
					if((KBD_HOME)||(KBD_L_WIN)||(KBD_R_WIN)){
						menu_ptr++;
						menu_mode[menu_ptr]=MENU_MAIN;
						break;
					}
					/*--PAUSE EMULATION--*/
					if((KBD_PAUSE_BREAK)){
						enter_pause();
						printf("Paused\n");
						//zx_machine_enable_vbuf(false);
						busy_wait_ms(10);
						MessageBox("PAUSE","Screen refresh stopped",CL_LT_GREEN,CL_BLUE,0);
						busy_wait_ms(100);
						clear_input();
						while(!wait_kbdjoy_emu()){
							busy_wait_ms(10);
						}
						printf("Resume Pause\n");
						clear_input();
						resume_pause();
						//zx_machine_enable_vbuf(true);
						emu_paused = false;
					}
					/*--PAUSE EMULATION--*/
					/*--Flat Key Operations--*/
					if (!((KBD_L_SHIFT||KBD_R_SHIFT)||(KBD_L_CTRL||KBD_R_CTRL))){
						/*--HELP--*/
						if (KBD_F1) {
							menu_ptr++;
							menu_mode[menu_ptr]=MENU_HELP;
							need_redraw=true;
							last_action=0;
							break;
						}
						/*--HELP--*/
					}
					/*--Flat Key Operations--*/
					/*--Emulator reset--*/
					if (((KBD_L_CTRL)||(KBD_R_CTRL))&&((KBD_L_ALT)||(KBD_R_ALT))&&(KBD_DELETE)){
						clear_input();
						printf("Restart\n");
						if (gw_program!=NULL){
							resume_pause();
							gw_system_reset();
							start_watch_mode();
						}						
						busy_wait_ms(150);
						clear_input();
					}
					if (((KBD_L_CTRL)||(KBD_R_CTRL))&&((KBD_L_ALT)||(KBD_R_ALT))&&(KBD_TAB)){
						clear_input();
						printf("Restart 2\n");
						if (gw_program!=NULL){
							resume_pause();
							gw_system_reset();
							start_watch_mode();
						}						
						busy_wait_ms(150);
						clear_input();
					}					
					/*--Emulator reset--*/
					/*--HARD reset--*/
					if (((KBD_L_SHIFT)||(KBD_R_SHIFT))&&((KBD_L_ALT)||(KBD_R_ALT))&&(KBD_DELETE)){ //((KBD_L_CTRL)||(KBD_R_CTRL))&&
						software_reset();
					}
					/*--HARD reset--*/
					/*--DEF reset--*/
					if (((KBD_L_CTRL)||(KBD_R_CTRL))&&((KBD_L_ALT)||(KBD_R_ALT))&&(KBD_BACK_SPACE)){ 
						config_write_defaults();
						busy_wait_ms(100);
						software_reset();
					}
					/*--DEF reset--*/
					/*--Volume--*/
					if ((KBD_NUM_PLUS)||(((KBD_L_ALT)||(KBD_R_ALT))&&(KBD_PAGE_UP))){ 
						cfg_volume+=CFG_VOLUME_STEP;
						if(cfg_volume>MAX_CFG_VOLUME_MODE){
								cfg_volume=MAX_CFG_VOLUME_MODE;
							}
						current_hud_mode|=HM_SHOW_VOLUME;
						current_hud_mode&=~HM_SHOW_BRIGHT;
						hud_timer = my_millis();
						busy_wait_ms(100);							
						clear_input();
					}
					if ((KBD_NUM_MINUS)||(((KBD_L_ALT)||(KBD_R_ALT))&&(KBD_PAGE_DOWN))){ 
						cfg_volume-=CFG_VOLUME_STEP;
						if(cfg_volume<0){
							cfg_volume=0;
						}
						current_hud_mode|=HM_SHOW_VOLUME;
						current_hud_mode&=~HM_SHOW_BRIGHT;
						hud_timer = my_millis();
						busy_wait_ms(100);
						clear_input();
					}
					if ((KBD_NUM_MULT)||(((KBD_L_ALT)||(KBD_R_ALT))&&(KBD_END))){
						if(cfg_volume>0){
							old_volume=cfg_volume;
							cfg_volume=0;
						} else if (cfg_volume==0){
							if(old_volume>0){
								cfg_volume=old_volume;
							} else {
								cfg_volume=DEF_CFG_VOLUME_MODE;
							}
							old_volume=0;
						}
						current_hud_mode|=HM_SHOW_VOLUME;
						current_hud_mode&=~HM_SHOW_BRIGHT;
						hud_timer = my_millis();
						busy_wait_ms(100);							
						clear_input();
					}
					/*--Volume--*/
					/*switch input mode/
					if(((KBD_L_ALT)||(KBD_R_ALT))&&(KBD_F11)){ 
						now_joy1_mode++;
						if(now_joy1_mode>=MAX_JOY_MODE) now_joy1_mode=5;
						if(now_joy1_mode<5) now_joy1_mode=5;
						clear_input();
					}
					/*switch input mode*/
					/*switch HUD*/
					if (KBD_F11){ //Show HUD
						uint8_t chm = current_hud_mode&0x03;
						chm++;
						//printf("CHM>%d\n",chm);
						if(chm>HM_TIME)chm=HM_OFF;
						if(chm==HM_OFF){
							MessageBox("HUD is ON","\0",CL_GREEN,CL_WHITE,2);
							gw_set_background();
							hud_timer=0;
							current_hud_mode=HM_ON;
							current_hud_mode|=HM_MAIN_HUD;
							continue;
						} 
						if(chm==HM_ON){
							hud_timer=my_millis();
							current_hud_mode=HM_TIME;
							current_hud_mode|=HM_MAIN_HUD;
							MessageBox("HUD is OFF by TIME","\0",CL_BLUE,CL_WHITE,2);
							gw_set_background();
							continue;
						}
						if(chm==HM_TIME){
							hud_timer=0;
							current_hud_mode=HM_OFF;
							MessageBox("HUD is OFF","\0",CL_BLUE,CL_WHITE,2);
							gw_set_background();
							continue;
						}
						clear_input();
					}
					/*switch HUD*/
					/*--QUICK FILE OPERATIONS--*/
					if(init_fs){
					}
					/*--QUICK FILE OPERATIONS--*/
					/*--Joy Menu--*/
					if(((KBD_F12))||(KBD_PRT_SCR)){
						menu_ptr++;
						menu_mode[menu_ptr]=MENU_JOY_MAIN;
						break;
					}
					/*--Joy Menu--*/

					//for(uint8_t j=0;j<8;j++){//printf("\t%02X",zx_write_buffer->kb_data[j]);};//printf("\n");//DEBUG
				}

				//
				/*-------Emulation--------*/
				//if ((ticker%80)==0){
				//}
				//soft keys emulation
				if((emulation)&&(emu_trigger)){
					emu_trigger=false;
					gw_system_run(GW_SYS_FREQ / rate);
					gpio_put(WORK_LED_PIN, gpio_get(WORK_LED_PIN)^1);
					//printf("gw_system_run\n");
					if (softkey_duration > 0)	softkey_duration--;
					if (softkey_duration == 0){
						softkey_A_pressed		= 0;
						softkey_B_pressed		= 0;
						softkey_Game_A_pressed	= 0;
						softkey_Game_B_pressed	= 0;
						softkey_time_pressed	= 0;
						softkey_alarm_pressed	= 0;
					}					
				}	
			
				/*while(graphics_draw_screen) {
					busy_wait_us(5);
					//continue;
				};//
				*/
				//if (graphics_frame_count%2==0) 
				//gw_system_blit(&graph_buf[0]);
				/*if (!graphics_begin_screen){
					gw_system_blit(&graph_buf[0]);
				}*/

				/*if ((ticker%2)==0){
					gw_system_run(GW_SYSTEM_CYCLES);
					gw_system_blit(&graph_buf[0]);
	
					//gw_emulate=true;
				}*/
				//
				/*gpio_put(WORK_LED_PIN,1);
				busy_wait_ms(33);
				gpio_put(WORK_LED_PIN,0);
				busy_wait_ms(33);
				/*}
				if(gw_emulate){
					gw_system_run(GW_SYSTEM_CYCLES);
					gw_emulate = false;
					gw_system_blit(&graph_buf[0]);
				}
				/*-------Emulation--------*/
				//gpio_put(WORK_LED_PIN,0);
				if(ticker>16777216){ticker=0;}
				tight_loop_contents();
			}while(menu_mode[menu_ptr]==EMULATION); //while(1) emulation loop
			//END EMULATION LOOP
			//clear_input();
			
			//zx_machine_enable_vbuf(false);
			if(menu_mode[menu_ptr]!=MENU_HELP){
				enter_pause();
			}
			//TRDOS_disabled = true;
		}
		//printf("3 menu_ptr:%d menu_mode:%d\n",menu_ptr,menu_mode[menu_ptr]);
	//END GLOBAL LOOP
	} while(true);//while(1) main loop
	software_reset();
}
