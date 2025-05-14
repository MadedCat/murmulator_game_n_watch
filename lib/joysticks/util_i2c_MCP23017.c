#include "pico/stdlib.h"
#include "pico/binary_info.h"
#include "hardware/i2c.h"

#include "util_i2c_MCP23017.h"
#include "util_i2c_joy.h"

extern uint8_t i2c_joy_data[16];



static bool mcp23017_set_regaddr(uint8_t ADDR, uint8_t reg, bool nostop) {
    int ret;
    ret = i2c_write_blocking(i2c_joy_port, ADDR, &reg, 1, nostop);
    if (ret != 1) {
        // printf("MCP23017: failed to set register address: ADDR = %02x\n",ADDR);
        return false;
    }
    return true;
}

static bool mcp23017_read(uint8_t ADDR, uint8_t reg, uint8_t* buf, size_t n) {
    int ret;
    if (!mcp23017_set_regaddr(ADDR, reg, false)) {
        return false;
    }
    ret = i2c_read_blocking(i2c_joy_port, ADDR, buf, n, false);
    if (ret != n) {
        // printf("MCP23017: failed to read register value:\n");
        return false;
    }
    return true;
}

static bool mcp23017_write(uint8_t ADDR, uint8_t reg, const uint8_t* buf, size_t n) {
    int ret;
    if (!mcp23017_set_regaddr(ADDR, reg, true)) {
        return false;
    }
    ret = i2c_write_blocking(i2c_joy_port, ADDR, buf, n, false);   
    if (ret != n) {
        // printf("MCP23017: failed to write register value:\n");
        return false;
    }
    return true;
}

uint32_t i2c_MCP_16button(){
    uint32_t result;
    uint8_t reg = MCP_REG_GPIOA;
    size_t n = 2;
    uint8_t ADDR = MCP_16BUTTON_ADDR;
    if(mcp23017_read(ADDR,reg,&i2c_joy_data[0],n)){result = ((uint32_t)i2c_joy_data[0]<<8)|i2c_joy_data[1];}
    return ~result;
}
uint32_t i2c_MCP_NES_joy(){
    const uint8_t val[6]= {
                    0x00,0x00,              // [0] NES low clock
                    0x0c,0x0c,              // [2] NES latch+clock
                    0x08,0x08,              // [4] NES high clock
                    };
    uint32_t result=0;
    uint8_t ret;
    uint32_t joy1=0;
    uint32_t joy2=0;
    uint8_t ADDR = I2C_MCP_NES_JOY_ADDR;
    uint8_t reg = MCP_REG_GPIOA;
    uint8_t n = 1;
    mcp23017_write(ADDR,reg,&val[2],2);
        for(int i=0; i<QNT_IMP_NES; i++ )
        {            
            mcp23017_write(ADDR, reg, &val[4] ,n);
            mcp23017_read(ADDR,reg,&i2c_joy_data[0],n);
            mcp23017_write(ADDR, reg, &val[0] ,n);
            joy1 <<= 1;
            joy2 <<= 1;
            joy1 |= (0x01 & i2c_joy_data[0]);
            joy2 |= ((0x02 & i2c_joy_data[0])>>1);                                
         }
    mcp23017_write(ADDR,reg,&val[4],n);       
    result = (joy2<<16)|joy1;
    result = result<<(16-QNT_IMP_NES);                 // сдвиг до старшего байта  если цикл 9 то 7 если цикл 8 то 8
    return ~result;
};
uint32_t i2c_MCP_SEGA_joy(){
    uint32_t result = 0;
    const uint8_t val[4] = {0xc0,0xc0,0x80,0x80};
    uint32_t temp;
    uint8_t ADDR = I2C_MCP_SEGA_JOY_ADDR;
    uint8_t reg = MCP_REG_GPIOA;
    uint8_t n = 2;
        for(int i=0; i<4; i++ ){
            mcp23017_write(ADDR, reg, &val[0] ,n);
            mcp23017_read(ADDR,reg,&i2c_joy_data[i*4],n);        
            mcp23017_write(ADDR, reg, &val[2] ,n);
            mcp23017_read(ADDR,reg,&i2c_joy_data[(i*4)+2],n);                  
        }
        mcp23017_write(ADDR, reg, &val[0] ,n);

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

bool init_MCP23017(uint8_t ADDR){ 
    const uint8_t val_iocon[2] = {0x00,0x00};
    const uint8_t val_iodira[6] = {
                                0xff,0xff,              //REG_IODIRA for 16 button
                                0xf3,0xf3,              //REG_IODIRA for 2 NES 
                                0x3f,0x3f               //REG_IODIRA for 2 SEGA
                                };    
    uint8_t reg;
    size_t n = 2;
    if(ADDR == MCP_16BUTTON_ADDR){
        reg = MCP_REG_IODIRA;   
        if(mcp23017_write(ADDR,reg,&val_iodira[0],n)){return true;}else{return false;} 
        }
    if (ADDR == MCP_NES_JOY_ADDR){
        reg = MCP_REG_IOCON;
        mcp23017_write(ADDR,reg,&val_iocon[0],n);        
        reg = MCP_REG_IODIRA;     
        if(mcp23017_write(ADDR,reg,&val_iodira[2],n)){return true;}else{return false;}
    }
    if (ADDR == MCP_SEGA_JOY_ADDR){     
        reg = MCP_REG_IOCON;
        mcp23017_write(ADDR,reg,&val_iocon[0],n);        
        reg = MCP_REG_IODIRA;   
        if(mcp23017_write(ADDR,reg,&val_iodira[4],n)){return true;}else{return false;}
    } 
    return false;     
}