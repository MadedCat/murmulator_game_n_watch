#include "pico.h"
#include <stdio.h>
#include "util_power.h"

#include "inttypes.h"
#include "hardware/adc.h"
#include "hardware/gpio.h"
#include "time.h"
#include "hardware/i2c.h"
//#include <stdio.h>

#define PIN_ADC_BATTERY_LEVEL       (29)
#define ADC_BATTERY_LEVEL_CHANNEL   (3)
#define PIN_USB_POWER_DETECT        (24)

bool power_MAX17043 = false;

// ADC Timer & frequency for Battery monitor
// Battery voltage
#define FULL_BATTERY_MV        (4200)
#define EMPTY_BATTERY_MV        (2500)
const uint16_t LOW_BATTERY_THRESHOLD = 2680;
static uint16_t battery_power_mv = 4200;
uint8_t battery_power_percent = 0;
bool battery_power_charge = false;

uint16_t battery_power_readings[10];
static uint8_t battery_power_pos =0;

uint8_t power_data[POWER_BYTE_COUNT];

bool power_init(){
	i2c_init(POWER_PORT, POWER_CLOCK);
	gpio_set_function(POWER_SDA_PIN, GPIO_FUNC_I2C);
	gpio_set_function(POWER_SCL_PIN, GPIO_FUNC_I2C);
	gpio_pull_up(POWER_SDA_PIN);
	gpio_pull_up(POWER_SCL_PIN);

	if(power_reset()){
		if(power_quickStart()){
			return true;
		}
	}
    i2c_deinit(POWER_PORT);
	gpio_set_function(POWER_SDA_PIN, GPIO_FUNC_NULL);
	gpio_set_function(POWER_SCL_PIN, GPIO_FUNC_NULL);
	gpio_deinit(POWER_SDA_PIN);
	gpio_deinit(POWER_SCL_PIN);
	return false;
}

uint16_t power_getVCell() {
	power_readRegister(VCELL_REGISTER);
	uint16_t vCell = (power_data[0]<<8)|power_data[1];
	vCell = (vCell) >> 4;
    //printf("Voltage: %dmV\n",(vCell/8)*10);
	return (uint16_t)((vCell / 8)*10);	
}

uint8_t power_getSoC() {
  	float percent;
	power_readRegister(SOC_REGISTER);
	uint16_t soc = (power_data[0]<<8)|power_data[1];
	percent = (soc & 0xFF00) >> 8;
  	percent += (float) (((uint8_t) soc) / 256.0);
    //printf("Percentage: %f%%\n",percent);
	return (uint8_t)percent;
}

int power_getVersion() {
	power_readRegister(VERSION_REGISTER);
	uint8_t MSB = power_data[1];
	uint8_t LSB = power_data[0];
	return (MSB << 8) | LSB;
}

uint8_t power_getCompensateValue() {
	power_readConfigRegister();
	uint8_t MSB = power_data[1];
	uint8_t LSB = power_data[0];
	return MSB;
}

uint8_t power_getAlertThreshold() {
	power_readConfigRegister();
	uint8_t MSB = power_data[1];
	uint8_t LSB = power_data[0];
	return 32 - (LSB & 0x1F);
}

void power_setAlertThreshold(uint8_t threshold) {
	power_readConfigRegister();
	uint8_t MSB = power_data[1];
	uint8_t LSB = power_data[0];
	if(threshold > 32) threshold = 32;
	threshold = 32 - threshold;
	power_writeRegister(CONFIG_REGISTER, MSB, ((LSB & 0xE0) | threshold));
}

bool power_inAlert() {
	power_readConfigRegister();	
	uint8_t MSB = power_data[1];
	uint8_t LSB = power_data[0];
	return LSB & 0x20;
}

void power_clearAlert() {
	power_readConfigRegister();
	uint8_t MSB = power_data[1];
	uint8_t LSB = power_data[0];
}



bool power_reset() {
	return power_writeRegister(COMMAND_REGISTER, 0x00, 0x54);
}

bool power_quickStart() {
	return power_writeRegister(MODE_REGISTER, 0x40, 0x00);
}


bool power_readConfigRegister() {
	return power_readRegister(CONFIG_REGISTER);
}


bool power_readRegister(uint8_t startAddress) {
	uint8_t result;
	busy_wait_us(200);
	result=i2c_write_blocking(POWER_PORT, POWER_ADDRESS, &startAddress, 1, false);    
	busy_wait_us(200);
	if(result==1){
		//printf("WR>%d\n",result);
		result = i2c_read_blocking(POWER_PORT, POWER_ADDRESS, &power_data[0], 2, false);
		//printf("RD>%d\n",result);
		//printf("[%02X][%02X]\n",power_data[0],power_data[1]);
		if(result==2){
			//printf("[%02X]>[%02X][%02X]\n",startAddress,power_data[0],power_data[1]);
			return true;
		}
	}
	return false;
}

bool power_writeRegister(uint8_t address, uint8_t MSB, uint8_t LSB) {
	uint8_t result;
	busy_wait_us(200);
	power_data[0]=address;
	power_data[1]=MSB;
	power_data[2]=LSB;
	busy_wait_us(200);
	result = i2c_write_blocking(POWER_PORT, POWER_ADDRESS, &power_data[0], 3, false);
	//printf("WR>%d\n",result);
	//printf("[%02X][%02X][%02X]\n",power_data[0],power_data[1],power_data[2]);
	if(result==3){
		return true;
	}
	return false;
}


long map(long x, long in_min, long in_max, long out_min, long out_max){
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

bool batt_management_usb_power_detected(){
    //return false;
    return gpio_get(PIN_USB_POWER_DETECT);
}

volatile int16_t result;

void monitor_battery_voltage(){
    int16_t delta=0;
    if(power_MAX17043){
        battery_power_mv = power_getVCell();
        battery_power_percent = power_getSoC();
        battery_power_charge = batt_management_usb_power_detected();        
    } else {
        // ADC Calibration Coefficients
        // ADC3 pin is connected to middle point of voltage divider 200Kohm + 100Kohm
        const int16_t coef_a = 9875;
        const int16_t coef_b = -20;
        adc_select_input(ADC_BATTERY_LEVEL_CHANNEL);
        //uint32_t result =0;
        /*for(uint8_t checks=0;checks<2;checks++){
            result += adc_read();
            busy_wait_ms(50);
        }
        result=round(result/2);
        */

       /*
        uint16_t result=0;
        //uint8_t cntr=0;
        do{
            result=adc_read();
            battery_power_mv = (result * coef_a) / ((1<<12) + coef_b);
            //cntr++;
        } while((battery_power_mv>=FULL_BATTERY_MV)||(battery_power_mv<EMPTY_BATTERY_MV));

        if ((battery_power_mv>(battery_power_readings[battery_power_pos]-150))&&(battery_power_mv<=(battery_power_readings[battery_power_pos]+50))){
            battery_power_readings[battery_power_pos]=battery_power_mv;
        }
        battery_power_pos++;
        if(battery_power_pos>10)battery_power_pos=0;
        uint32_t totalpower=0;
        for(uint8_t cnt=0;cnt<10;cnt++){
            totalpower+=battery_power_readings[cnt];
        }
        //battery_power_percent = round(((totalpower/10)*100)/FULL_BATTERY_MV);
        battery_power_percent = map((totalpower/10),EMPTY_BATTERY_MV,FULL_BATTERY_MV,0,100);
        battery_power_charge = batt_management_usb_power_detected();
        printf("[%08d]RAW[%06d] BV = %d (mV) %d%% {%04d} [%04d][%04d][%04d][%04d][%04d][%04d][%04d][%04d][%04d][%04d]\n",
            us_to_ms(time_us_32()), 
            result,
            battery_power_mv,
            battery_power_percent,
            (totalpower/10),
            battery_power_readings[0],
            battery_power_readings[1],
            battery_power_readings[2],
            battery_power_readings[3],
            battery_power_readings[4],
            battery_power_readings[5],
            battery_power_readings[6],
            battery_power_readings[7],
            battery_power_readings[8],
            battery_power_readings[9]
        );
        */

        
        delta = adc_read()-result;
        result = result + (delta/10);
        battery_power_mv = (result * coef_a) / ((1<<12) + coef_b);
        if (battery_power_mv<EMPTY_BATTERY_MV) battery_power_mv=EMPTY_BATTERY_MV;
        battery_power_percent = map(battery_power_mv,EMPTY_BATTERY_MV,FULL_BATTERY_MV,0,100);
        battery_power_charge = batt_management_usb_power_detected();
    }
    /*
    printf("[%08d] RAW[%06d]{%04d} BV = %d (mV) %d%% (%d)\n",
        us_to_ms(time_us_32()), 
        result,
        delta,
        battery_power_mv,
        battery_power_percent,
        battery_power_charge
    );
    */
}



bool batt_management_init(){
    gpio_set_dir(PIN_USB_POWER_DETECT, GPIO_IN);

    if(power_init()){
        power_MAX17043 = true;
		printf("Power monitor MAX17043 present\n");
        return true;
	} else {
        adc_init();
        adc_gpio_init(PIN_ADC_BATTERY_LEVEL);
        busy_wait_ms(100);
        result = 1400;
        //battery_power_readings[0]=adc_read();
        /*for(uint8_t cnt=0;cnt<10;cnt++){
            battery_power_readings[cnt]=FULL_BATTERY_MV;
        }*/
        if (battery_power_mv<EMPTY_BATTERY_MV) battery_power_mv=EMPTY_BATTERY_MV;
        printf("Power monitor ADC present\n");
        return true;
    }
    return false;
}

void batt_management_stop(){
    //cancel_repeating_timer(&batt_timer);
}


