#include <stdio.h>
#include <string.h>
#include "inttypes.h"
#include <pico/stdlib.h>

#include "Joystics.h"
#include "util_Wii_Joy.h"
#include "util_i2c_joy.h"
#include "util_i2c_MCP23017.h"

PresentJoystics Joystics;
DataJoystics Joy_data;

uint32_t maskfornes;
 
void data_joy_input(int Type_Joy){
    uint32_t temp;
    switch (Type_Joy){
            case NES_joy1_2:                // 
                /* code */
                temp = d_joy_get_data();            
                Joy_data.Data_NES_joy1_2 = temp&maskfornes;                   
                break;
            case i2c_PCF_8_buttons:
                /* code */
                if(Joystics.Present_i2c_PCF_8_buttons) {Joy_data.Data_i2c_PCF_8_buttons=i2c_PCF_8button();}else {Joy_data.Data_i2c_PCF_8_buttons=0;}
                break;               
            case i2c_PCF_16_buttons:
                /* code */
                if(Joystics.Present_i2c_PCF_16_buttons) {Joy_data.Data_i2c_PCF_16_buttons=i2c_PCF_16button();}else {Joy_data.Data_i2c_PCF_16_buttons=0;}
                break;
            case i2c_PCF_NES_joysticks:
                /* code */
   
                temp = i2c_PCF_NES_joy();
                if ((temp&0x00080000)>0) {
                    Joystics.Present_i2c_PCF_NES_joy2=false;
                    }else {
                        Joystics.Present_i2c_PCF_NES_joy2=true;
                        Joy_data.Data_i2c_PCF_NES_joy1_2|=temp&0xFFF80000;
                        // printf("I2C_NES_joystick2 is connected\n");
                        }
                if ((temp&0x00000008)>0) {
                    Joystics.Present_i2c_PCF_NES_joy1=false;
                    }else {
                        Joystics.Present_i2c_PCF_NES_joy1=true;
                        Joy_data.Data_i2c_PCF_NES_joy1_2|=temp&0x0000FFF8;
                        // printf("I2C_NES_joystick1 is connected\n");
                        }
                break;
            case i2c_PCF_SEGA:
                temp = i2c_PCF_SEGA_joy();
                Joy_data.Data_i2c_PCF_SEGA_joy = temp;               
                break;    
            case i2c_MCP_16_buttons:
                    if(Joystics.Present_i2c_MCP_16_buttons) {Joy_data.Data_i2c_MCP_16_buttons=i2c_MCP_16button();}else {Joy_data.Data_i2c_MCP_16_buttons=0;}
                break;
            case i2c_MCP_NES_joysticks:
                
                temp = i2c_MCP_NES_joy();
                if ((temp&0x00080000)>0) {
                    Joystics.Present_i2c_MCP_NES_joy2=false;
                    }else {
                        Joystics.Present_i2c_MCP_NES_joy2=true;
                        Joy_data.Data_i2c_MCP_NES_joy1_2|=temp&0xFFF80000;
                        // printf("I2C_MCP_NES_joystick2 is connected\n");
                        }
                if ((temp&0x00000008)>0) {
                    Joystics.Present_i2c_MCP_NES_joy1=false;
                    }else {
                        Joystics.Present_i2c_MCP_NES_joy1=true;
                        Joy_data.Data_i2c_MCP_NES_joy1_2|=temp&0x0000FFF8;
                        // printf("I2C_MCP_NES_joystick1 is connected\n");
                        }
            
                break;
            case i2c_MCP_SEGA:                
                    temp = i2c_MCP_SEGA_joy(); 
                    Joy_data.Data_i2c_MCP_SEGA_joy = temp;
                break;    
            case WII_joy:
                /* code */
                if(Joystics.Present_WII_joy) {
                    // Wii_clear_old();
                    if(Wii_decode_joy()==0x01){
                        //Wii_debug(&Wii_joy_data);
                        Joy_data.Data_WII_joy = map_to_nes(&Wii_joy_data);
                    } else {
                        Joy_data.Data_WII_joy = 0;    
                    }
                } else {
                    Joy_data.Data_WII_joy = 0;
                }

                break;
            default:
                break;
            }

};
    
int active_joystick_data(int active_joystick){
    uint32_t result;
    switch (active_joystick){
        case NES_joy1_2:
            /* code */
            result = Joy_data.Data_NES_joy1_2;      // маска кнопок NES джойстиков  
            //printf("NES result = %04lx\n",result);  
            if(result!=0) {Joystics.joy_pressed = true;}else {Joystics.joy_pressed = false;}
            Joy_data.Data_NES_joy1_2 = 0;           // обнуление после получения данных
            break;
        case i2c_PCF_8_buttons:
            /* code */
            result = 0x0000ff00&Joy_data.Data_i2c_PCF_8_buttons;      // маска 8 кнопок
            if(result!=0) {Joystics.joy_pressed = true;}else {Joystics.joy_pressed = false;}
            Joy_data.Data_i2c_PCF_8_buttons = 0;           // обнуление после получения данных
            break;            
        case i2c_PCF_16_buttons:
            /* code */
            result = 0x0000ffff&Joy_data.Data_i2c_PCF_16_buttons;      // маска 16 кнопок
            if(result!=0) {Joystics.joy_pressed = true;}else {Joystics.joy_pressed = false;}
            Joy_data.Data_i2c_PCF_16_buttons = 0;           // обнуление после получения данных
            break;
                    
        case i2c_PCF_NES_joysticks:
            /* code */
            result = Joy_data.Data_i2c_PCF_NES_joy1_2;      //маска кнопок NES джойстиков
            if(result!=0) {Joystics.joy_pressed = true;}else {Joystics.joy_pressed = false;}
            Joy_data.Data_i2c_PCF_NES_joy1_2 = 0;           // обнуление после получения данных
            break;
        case i2c_PCF_SEGA:
            /* code */
            result = Joy_data.Data_i2c_PCF_SEGA_joy;      //маска кнопок NES джойстиков
            if(result!=0) {Joystics.joy_pressed = true;}else {Joystics.joy_pressed = false;}
            Joy_data.Data_i2c_PCF_SEGA_joy = 0;           // обнуление после получения данных
            break;            
        case i2c_MCP_16_buttons:
            /* code */
            result = 0x0000ffff&Joy_data.Data_i2c_MCP_16_buttons;      //
            if(result!=0) {Joystics.joy_pressed = true;}else {Joystics.joy_pressed = false;}
            Joy_data.Data_i2c_MCP_16_buttons = 0;           // обнуление после получения данных
            break;
                    
        case i2c_MCP_NES_joysticks:
            /* code */
            result = Joy_data.Data_i2c_MCP_NES_joy1_2;      //маска кнопок NES джойстиков
            if(result!=0) {Joystics.joy_pressed = true;}else {Joystics.joy_pressed = false;}
            Joy_data.Data_i2c_MCP_NES_joy1_2 = 0;           // обнуление после получения данных
            break; 
        case i2c_MCP_SEGA:
            /* code */
            result = Joy_data.Data_i2c_MCP_SEGA_joy;      //маска кнопок NES джойстиков
            if(result!=0) {Joystics.joy_pressed = true;}else {Joystics.joy_pressed = false;}
            Joy_data.Data_i2c_MCP_SEGA_joy = 0;           // обнуление после получения данных
            break;                
        case WII_joy:
            result = Joy_data.Data_WII_joy&0x0000ffff;
            if(result!=0) {Joystics.joy_pressed = true;}else {Joystics.joy_pressed = false;}            
            // Joy_data.Data_WII_joy = 0;
            break;

        default:
            break;
    }
    return result;
};

int joy_start(){
    uint8_t is_present = NES_joy1_2;
    if (i2c_joy_start()) {
        if(Joystics.Present_i2c_PCF_8_buttons){is_present = i2c_PCF_8_buttons;}
        if(Joystics.Present_i2c_PCF_16_buttons){is_present = i2c_PCF_16_buttons;}
        if(Joystics.Present_i2c_PCF_Joysticks) {is_present = i2c_PCF_NES_joysticks;}
        if(Joystics.Present_i2c_PCF_SEGA_joy){is_present = i2c_PCF_SEGA;}
        if(Joystics.Present_i2c_MCP_16_buttons){is_present = i2c_MCP_16_buttons;}
        if(Joystics.Present_i2c_MCP_Joysticks) {is_present = i2c_MCP_NES_joysticks;}
        if(Joystics.Present_i2c_MCP_SEGA_joy){is_present = i2c_MCP_SEGA;}
        if(Joystics.Present_WII_joy){is_present = WII_joy;}
        Joystics.joy_connected = true;
        printf ("I2C joystick is present\n");
    } else {
        d_joy_init();
        Joystics.joy_connected = false;
        uint32_t temp = d_joy_get_data();
        if((temp&MASK_SNES_1)==0){
            maskfornes = MASK_SNES_1;
            Joystics.Present_NES_joy1=true;
            Joystics.joy_connected = true;
            printf ("NES joystick 1 is present\n");
        }else {
            if((temp&MASK_NES_1)==0){
                maskfornes = MASK_NES_1;
                Joystics.Present_NES_joy1=true;
                Joystics.joy_connected = true;
            } else {
                Joystics.Present_NES_joy1=false;
            }
        }
        if((temp&MASK_SNES_2)==0){
            maskfornes |= MASK_SNES_2;
            Joystics.Present_NES_joy2=true;
            Joystics.joy_connected = true;
            printf ("NES joystick 2 is present\n");
        }else {
            if((temp&MASK_NES_2)==0){
                maskfornes |= MASK_NES_2;
                Joystics.Present_NES_joy2=true;
                Joystics.joy_connected = true;
            } else {
                Joystics.Present_NES_joy2=false;
            }
        }          
    }
            
    return is_present;
};
