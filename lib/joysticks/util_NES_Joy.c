#include <stdio.h>
#include <string.h>
#include "inttypes.h"
#include <pico/stdlib.h>

#include "util_NES_Joy.h"

// bool joy_pressed 	= false;
// bool joy_connected	= false;
// uint8_t data_joy=0;
// uint8_t old_data_joy=0;
// uint8_t rel_data_joy=0;

uint32_t d_joy_get_data(){
	uint16_t data1=0;					// место сбора данных 1 джойстика
	uint16_t data2=0;					// место сбора данных 2 джойстика
	uint32_t result=0;					
	gpio_put(D_JOY_LATCH_PIN,1);
	//gpio_put(D_JOY_CLK_PIN,1);
	busy_wait_us(40);//12//24
	
	gpio_put(D_JOY_LATCH_PIN,0);
	busy_wait_us(40);//6//12
	for(int i=0;i<QNT_IMP_NES;i++){   
		gpio_put(D_JOY_CLK_PIN,0);  
		busy_wait_us(40);//10//20

		data1<<=1;
		data1|=gpio_get(D_JOY1_DATA_PIN);

		data2<<=1;
		data2|=gpio_get(D_JOY2_DATA_PIN);

		// busy_wait_us(40);//10//20
		gpio_put(D_JOY_CLK_PIN,1); 
		busy_wait_us(40);//10//20
		// tight_loop_contents();
	}

	if((data1>0)||(data2>0)){
		if(data1>0) {result |= data1;} else {result |=((uint32_t)0xFFFF);}
		if(data2>0){result |=((uint32_t)data2<<16);} else{result |=((uint32_t)0xFFFF<<16);}
		//printf("NES>%08X\n",result);
		result = result <<(16-QNT_IMP_NES);
		result = ~result;
		//printf("NES>%08X\n",result);
	}
	return result;
};



void d_joy_init(){
	gpio_init(D_JOY_CLK_PIN);
	gpio_set_dir(D_JOY_CLK_PIN,GPIO_OUT);
	gpio_init(D_JOY_LATCH_PIN);
	gpio_set_dir(D_JOY_LATCH_PIN,GPIO_OUT);
	
	gpio_init(D_JOY1_DATA_PIN);
	gpio_set_dir(D_JOY1_DATA_PIN,GPIO_IN);
	gpio_pull_down(D_JOY1_DATA_PIN);
	//gpio_pull_up(D_JOY1_DATA_PIN);

	gpio_init(D_JOY2_DATA_PIN);
	gpio_set_dir(D_JOY2_DATA_PIN,GPIO_IN);
	gpio_pull_down(D_JOY2_DATA_PIN);
	//gpio_pull_up(D_JOY2_DATA_PIN);

	gpio_put(D_JOY_LATCH_PIN,0);
		
}