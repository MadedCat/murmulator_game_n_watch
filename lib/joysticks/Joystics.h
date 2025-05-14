#pragma once
#include <stdio.h>
#include "inttypes.h"

#include "util_i2c_joy.h"
#include "util_NES_Joy.h"

#define QNT_IMP_NES (13)                // количество импульсов чтения джойстика NES

#define MASK_NES_1  (0x0000ff00)
#define MASK_SNES_1 (0x0000fff0)    
#define MASK_NES_2  (0xff000000)
#define MASK_SNES_2 (0xfff00000) 

//NES                       JOYSTICK-1                JOYSTICK-2

// #define DPAD_RIGHT		0x00000100	              0x01000000
// #define DPAD_LEFT		0x00000200                0x02000000
// #define DPAD_DOWN		0x00000400	              0x04000000
// #define DPAD_UP		    0x00000800	              0x08000000

// #define BUTTON_START		0x00001000
// #define BUTTON_SELECT	0x00002000
// #define BUTTON_A			0x00004000	
// #define BUTTON_B			0x00008000	

//SNES

// #define DPAD_RIGHT		0x00000100	
// #define DPAD_LEFT		0x00000200	
// #define DPAD_DOWN		0x00000400	
// #define DPAD_UP		    0x00000800	

// #define BUTTON_START		0x00001000
// #define BUTTON_SELECT	0x00002000
// #define BUTTON_Y			0x00004000	
// #define BUTTON_B			0x00008000

// #define BUTTON_RT        0x00000010
// #define BUTTON_LT        0x00000020
// #define BUTTON_X         0x00000040   
// #define BUTTON_A         0x00000080

#define D_JOY_RIGHT		0x0001	//(data_joy==0x01)
#define D_JOY_LEFT		0x0002	//(data_joy==0x02)
#define D_JOY_DOWN		0x0004	//(data_joy==0x04)
#define D_JOY_UP		0x0008	//(data_joy==0x08)
#define D_JOY_A			0x0010	//(data_joy==0x20)
#define D_JOY_B			0x0020	//(data_joy==0x10)
#define D_JOY_SELECT	0x0040	//(data_joy==0x40)
#define D_JOY_START		0x0080	//(data_joy==0x80)
#define D_JOY_X			0x0100	//(data_joy==0x20)
#define D_JOY_Y			0x0200	//(data_joy==0x10)


enum TypeJoy{
    NES_joy1_2,                                 // первый джойстик NES ногодрыг и  второй джойстик NES ногодрыг
    i2c_PCF_8_buttons,
    i2c_PCF_16_buttons,                         // 16 кнопок с расширителя портов PCF8575 400 kHz адрес             0x20h
    i2c_PCF_NES_joysticks,                      // два NES джойстика расширителя портов PCF8575 400 kHz адрес       0x21h
    i2c_PCF_SEGA,                               // SEGA джойстик  расширителя портов PCF8575 400 kHz адрес          0x22h
    i2c_MCP_16_buttons,                         // 16 кнопок с расширителя портов MCP23017 400 kHz адрес            0x23h
    i2c_MCP_NES_joysticks,                      // два NES джойстика расширителя портов MCP23017 400 kHz адрес      0x24h
    i2c_MCP_SEGA,                               // SEGA джойстик  расширителя портов MCP23017 400 kHz адрес         0x22h
    WII_joy,                                    // WII джойстик адрес 0x52h
};


typedef struct PresentJoystics PresentJoystics;
struct PresentJoystics{
    bool Present_NES_joy1;
    bool Present_NES_joy2;
    bool Present_i2c_PCF_8_buttons;
    bool Present_i2c_PCF_16_buttons;
    bool Present_i2c_PCF_Joysticks;    
    bool Present_i2c_PCF_NES_joy1;
    bool Present_i2c_PCF_NES_joy2;
    bool Present_i2c_PCF_SEGA_joy;
    bool Present_i2c_MCP_16_buttons;
    bool Present_i2c_MCP_Joysticks;    
    bool Present_i2c_MCP_NES_joy1;
    bool Present_i2c_MCP_NES_joy2;
    bool Present_i2c_MCP_SEGA_joy;    
    bool Present_WII_joy;
    bool joy_connected;
    bool joy_pressed;
};

// 8 bit axis X, 8 bit axis Y, 8 bit SNES extend data, 8 bit NES data
// or
// 8 bit NES extend data joy2, 8 bit SNES data joy2, 8 bit NES extend data joy1, 8 bit SNES data joy1
typedef struct DataJoystics DataJoystics;
struct DataJoystics{
    uint32_t Data_NES_joy1_2;
    uint32_t Data_i2c_PCF_8_buttons;    
    uint32_t Data_i2c_PCF_16_buttons;
    uint32_t Data_i2c_PCF_NES_joy1_2;
    uint32_t Data_i2c_PCF_SEGA_joy;
    uint32_t Data_i2c_MCP_16_buttons;
    uint32_t Data_i2c_MCP_NES_joy1_2;
    uint32_t Data_i2c_MCP_SEGA_joy;            
    uint32_t Data_WII_joy;   
};

extern PresentJoystics Joystics;
extern DataJoystics Joy_data;

int joy_start();
int active_joystick_data(int active_joystick);
void data_joy_input(int Type_Joy);

