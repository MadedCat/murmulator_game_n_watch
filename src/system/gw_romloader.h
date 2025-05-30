/*

This program permits to load ROM generated by LCD-Game-Shrinker.

This program is free software: you can redistribute it and/or modify it under
the terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later
version.
This program is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
You should have received a copy of the GNU General Public License along with
this program. If not, see <http://www.gnu.org/licenses/>.

__author__ = "bzhxx"
__contact__ = "https://github.com/bzhxx"
__license__ = "GPLv3"

*/

#ifndef _GW_ROMLOADER_H_
#define _GW_ROMLOADER_H_

#include <stdint.h>

#define  ROM_HEADER_SIZE   (150)

/* Different CPU 4bits from SHARP */
#define  ROM_CPU_SM5A        "SM5A_"
#define  ROM_CPU_SM500       "SM500"
#define  ROM_CPU_SM510       "SM510"
#define  ROM_CPU_SM511       "SM511"
#define  ROM_CPU_SM512       "SM512"
#define  ROM_CPU_SM520       "SM520"
#define  ROM_CPU_SM530       "SM530"

/* flags in ROM file */
// use to check 'flag_rendering_lcd_inverted' bit0 in rom_head.flags
// use to check 'flag_segments_4bits'         bit4 in rom_head.flags
// use to check 'flag_background_jpeg'        bit5 in rom_head.flags
// use to check 'flag_segments_2bits'         bit8 in rom_head.flags
#define FLAG_RENDERING_LCD_INVERTED 0x01
#define FLAG_SEGMENTS_4BITS         0x10
#define FLAG_SEGMENTS_2BITS         0x100
#define FLAG_BACKGROUND_JPEG        0x20

// use to determine the piezo buzzer mode bits1..3 in rom_head.flags

#define FLAG_SOUND_MASK 0xE

#define FLAG_SOUND_R1_PIEZO   1 << 1
#define FLAG_SOUND_R2_PIEZO   2 << 1
#define FLAG_SOUND_R1R2_PIEZO 3 << 1
#define FLAG_SOUND_R1S1_PIEZO 4 << 1
#define FLAG_SOUND_S1R1_PIEZO 5 << 1

// use to determine the LCD deflicker mode bits6.7 in rom_head.flags
#define FLAG_LCD_DEFLICKER_MASK 0xC0

#define FLAG_LCD_DEFLICKER_OFF 0x00
#define FLAG_LCD_DEFLICKER_1   0x40
#define FLAG_LCD_DEFLICKER_2   0x80
#define FLAG_LCD_DEFLICKER_3   0xC0

/* ADD ROM SUPPORT */
/* Large memory to store all objects from external flash */
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

/* ROM header */
typedef struct gwmromheader_s
{
   /* Name of the sharp microcomputers to emulate */
   char cpu_name[8]                ;

   /* Signature of the original ROM name (8 last characters) */
   /* used to check coherency with save */

   char rom_signature[8]           ;

   /* Address counter time used by the program to manage it RTC */
   uint8_t time_hour_address_msb;
   uint8_t time_hour_address_lsb;
   uint8_t time_min_address_msb ;
   uint8_t time_min_address_lsb ;
   uint8_t time_sec_address_msb ;
   uint8_t time_sec_address_lsb ;
   uint8_t time_hour_msb_pm;

   /* spare reserved for futur used */
   uint8_t byte_spare1       ;

   /* Flags to describe hardware : buzzer and lcd deflicker filter */
   uint32_t flags                  ;

   /* Background palette RGB888 */
   uint32_t background_palette       ;
   uint32_t background_palette_size  ;
   
   /* Background pixels 320x240 RGB565 */
   uint32_t background_pixel       ;
   uint32_t background_pixel_size  ;

   /* Segments pixels packed Maximum size 320x240 RGB565 */
   uint32_t segments_pixel         ;
   uint32_t segments_pixel_size    ;

   /* Segments offset and dimensions : x,y,height,width */
   uint32_t segments_offset        ;
   uint32_t segments_offset_size   ;
   uint32_t segments_x             ;
   uint32_t segments_x_size        ;
   uint32_t segments_y             ;
   uint32_t segments_y_size        ;
   uint32_t segments_height        ;
   uint32_t segments_height_size   ;
   uint32_t segments_width         ;
   uint32_t segments_width_size    ;

   /* Melody */
   uint32_t melody         ;
   uint32_t melody_size    ;

   /* CPU program */
   uint32_t program       ;
   uint32_t program_size  ;

   /* CPU keyboard mapping */
   uint32_t keyboard      ;
   uint32_t keyboard_size ;

   /* Prewiew palette RGB888 */
   uint32_t prewiew_palette       ;
   uint32_t prewiew_palette_size  ;
   
   /* Prewiew pixels 184x103 RGBi8 */
   uint32_t prewiew_pixel       ;
   uint32_t prewiew_pixel_size  ;
   
   /* TODO Add support for dual vertical screen */
   // hook on segments to monitor
   // 2nd background set when the hook select the upper screen
   // 2nd segments set when the hook select the upper screen

} gwromheader_t;

extern gwromheader_t gw_head;

extern uint8_t prevew_buff[26000];
extern uint8_t prevew_buff_pal[700];
extern void updatePalette(const uint8_t *palette,uint16_t length);

bool gw_romloader();
void gw_assign_ptrs();
void gw_dump_struct();
bool LoadScreenFromGWNSnapshot(char *file_name);

#endif /* _GW_ROMLOADER_H_ */
