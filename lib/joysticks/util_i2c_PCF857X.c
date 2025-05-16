
#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/binary_info.h"
#include "hardware/i2c.h"

#include "util_i2c_PCF857X.h"
#include "util_i2c_joy.h"

extern uint8_t i2c_joy_data[16];

uint32_t i2c_PCF_8button(){
        uint32_t result= 0xffff00ff;
        i2c_read_blocking(i2c_joy_port, I2C_PCF8574_8BUTTON_ADDR, &i2c_joy_data[0], 1, false);
        result |= ((uint32_t)i2c_joy_data[0]<<8);
    return ~result;
};
uint32_t i2c_PCF_16button(){
        uint32_t result= 0xffff0000;
        i2c_read_blocking(i2c_joy_port, I2C_PCF_16BUTTON_ADDR, &i2c_joy_data[0], 2, false);
        result |= ((uint32_t)i2c_joy_data[0]<<8)|i2c_joy_data[1];
    return ~result;
};
uint32_t i2c_PCF_NES_joy(){
    uint32_t result;
    const uint8_t latch[2] = {0xff,0xff};
    const uint8_t high_clk[2] = {0xfb,0xfb};
    const uint8_t low_clk[2] = {0xf3,0xf3};
    uint8_t ret;
    uint32_t joy1=0;
    uint32_t joy2=0; 
    busy_wait_us(200);
    ret = i2c_write_blocking(i2c_joy_port, I2C_PCF_NES_JOY_ADDR, &latch[0] ,2, true);
    for(int i=0; i<QNT_IMP_NES; i++ ){            
        ret = i2c_write_blocking(i2c_joy_port, I2C_PCF_NES_JOY_ADDR, &high_clk[0], 2, true);
        ret = i2c_read_blocking(i2c_joy_port, I2C_PCF_NES_JOY_ADDR, &i2c_joy_data[0], 1, true);
        ret = i2c_write_blocking(i2c_joy_port, I2C_PCF_NES_JOY_ADDR, &low_clk[0], 2, true);
        joy1 <<= 1;
        joy2 <<= 1;
        joy1 |= (0x01 & i2c_joy_data[0]);
        joy2 |= ((0x02 & i2c_joy_data[0])>>1);                                
    }
    busy_wait_us(200);
    ret = i2c_write_blocking(i2c_joy_port, I2C_PCF_NES_JOY_ADDR, &high_clk[0], 2, false);     
    result = (joy2<<16)|joy1;
    result = result<<(16-QNT_IMP_NES);
    return ~result;
};
uint32_t i2c_PCF_SEGA_joy(){
    uint32_t result =0 ;
    const uint8_t val[4] = {
                            0x7f,0x7f,          // [0] high clock
                            0x3f,0x3f,          // [2]  low clock  
                                };
    uint8_t ret;


        for(int i=0; i<4; i++ ){
            ret = i2c_write_blocking(i2c_joy_port, I2C_PCF_SEGA_JOY_ADDR, &val[0], 2, true);
            ret = i2c_read_blocking(i2c_joy_port, I2C_PCF_SEGA_JOY_ADDR, &i2c_joy_data[i*4], 2, true); 
            ret = i2c_write_blocking(i2c_joy_port, I2C_PCF_SEGA_JOY_ADDR, &val[2], 2, true);
            ret = i2c_read_blocking(i2c_joy_port, I2C_PCF_SEGA_JOY_ADDR, &i2c_joy_data[(i*4)+2], 2, true);  
        }
        ret = i2c_write_blocking(i2c_joy_port, I2C_PCF_SEGA_JOY_ADDR, &val[0], 2, false);

        uint8_t temp0 = ~i2c_joy_data[0];
        uint8_t temp1 = ~i2c_joy_data[1];
        uint8_t temp2 = ~i2c_joy_data[2];
        uint8_t temp3 = ~i2c_joy_data[3];
        uint8_t temp12 = ~i2c_joy_data[12];
        uint8_t temp13 = ~i2c_joy_data[13];
        // первый джойстик
            if (temp0&0x08)	        { result|=0x00000100;}              //right
            if (temp0&0x04)	        { result|=0x00000200;}              //left
            if (temp0&0x02)	        { result|=0x00000400;}              //down
            if (temp0&0x01)		    { result|=0x00000800;}              //up

            if (temp2&0x20)		    { result|=0x00001000;}              //start
            if (temp0&0x20)		    { result|=0x00002000;}              //C->select
            if (temp2&0x10)		    { result|=0x00004000;}              //A
            if (temp0&0x10)		    { result|=0x00008000;}              //B

            if (temp12&0x01)	    { result|=0x00000001;}              //Z
            if (temp12&0x02)	    { result|=0x00000002;}              //X
            if (temp12&0x04)	    { result|=0x00000004;}              //Y
            if (temp12&0x08)	    { result|=0x00000008;}              //MODE
        // второй джойстик      
            if (temp1&0x08)	        { result|=0x01000000;}              //right
            if (temp1&0x04)	        { result|=0x02000000;}              //left
            if (temp1&0x02)	        { result|=0x04000000;}              //down
            if (temp1&0x01)		    { result|=0x08000000;}              //up

            if (temp3&0x20)		    { result|=0x10000000;}              //start
            if (temp1&0x20)		    { result|=0x20000000;}              //C->select
            if (temp3&0x10)		    { result|=0x40000000;}              //A
            if (temp1&0x10)		    { result|=0x80000000;}              //B

            if (temp13&0x01)	    { result|=0x00010000;}              //Z
            if (temp13&0x02)	    { result|=0x00020000;}              //X
            if (temp13&0x04)	    { result|=0x00040000;}              //Y
            if (temp13&0x08)	    { result|=0x00080000;}              //MODE

    return result;
};
bool init_PCF857X(uint8_t ADDR){ 
    int ret;
    ret = i2c_read_blocking(i2c_joy_port, ADDR, &i2c_joy_data[0], 2, true);
    if (ret != 2){return false;}
    return true;
};