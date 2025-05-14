#include "i2s.h"

#include <stdio.h>

#include "hardware/pio.h"
#include <hardware/clocks.h>
#include "hardware/dma.h"


// --------- //
// audio_i2s //
// --------- //

#define audio_i2s_wrap_target 0
#define audio_i2s_wrap 7
#define audio_i2s_pio_version 0

#define audio_i2s_offset_entry_point 7u

static const uint16_t audio_i2s_program_instructions[] = {
            //     .wrap_target
    0x7001, //  0: out    pins, 1         side 2     
    0x1840, //  1: jmp    x--, 0          side 3     
    0x6001, //  2: out    pins, 1         side 0     
    0xe82e, //  3: set    x, 14           side 1     
    0x6001, //  4: out    pins, 1         side 0     
    0x0844, //  5: jmp    x--, 4          side 1     
    0x7001, //  6: out    pins, 1         side 2     
    0xf82e, //  7: set    x, 14           side 3     
            //     .wrap
};

#if !PICO_NO_HARDWARE
static const struct pio_program audio_i2s_program = {
    .instructions = audio_i2s_program_instructions,
    .length = 8,
    .origin = -1,
    .pio_version = audio_i2s_pio_version,
#if PICO_PIO_VERSION > 0
    .used_gpio_ranges = 0x0
#endif
};
#endif

static inline pio_sm_config audio_i2s_program_get_default_config(uint offset) {
    pio_sm_config c = pio_get_default_sm_config();
    sm_config_set_wrap(&c, offset + audio_i2s_wrap_target, offset + audio_i2s_wrap);
    sm_config_set_sideset(&c, 2, false, false);
    return c;
};

static int sm_i2s=-1;
static uint32_t i2s_data;
static uint32_t trans_count_DMA=1<<25;


static void i2s_pio_init(){
    PIO pio=PIO_I2S;
    
    sm_i2s  = pio_claim_unused_sm(pio, true);
    
    printf("sm_i2s:%d\n",sm_i2s);

    uint offset = pio_add_program(pio, &audio_i2s_program);

    uint8_t func=(pio==pio0)?GPIO_FUNC_PIO0:GPIO_FUNC_PIO1;    // TODO: GPIO_FUNC_PIO0 for pio0 or GPIO_FUNC_PIO1 for pio1
    gpio_set_function(I2S_DATA_PIN, func);
    gpio_set_function(I2S_CLK_BASE_PIN, func);
    gpio_set_function(I2S_CLK_BASE_PIN+1, func);

    printf("func:%d\n",func);

    pio_sm_config c = pio_get_default_sm_config();
    sm_config_set_wrap(&c, offset + audio_i2s_wrap_target, offset + audio_i2s_wrap);
    sm_config_set_sideset(&c, 2, false, false);

    pio_sm_config sm_config = audio_i2s_program_get_default_config(offset);
    sm_config_set_out_pins(&sm_config, I2S_DATA_PIN, 1);
    sm_config_set_sideset_pins(&sm_config, I2S_CLK_BASE_PIN);
    sm_config_set_out_shift(&sm_config, false, true, 32);
    pio_sm_init(pio, sm_i2s, offset, &sm_config);
    uint pin_mask = (1u << I2S_DATA_PIN) | (3u << I2S_CLK_BASE_PIN);
    pio_sm_set_pindirs_with_mask(pio, sm_i2s, pin_mask, pin_mask);
    pio_sm_set_pins(pio, sm_i2s, 0); // clear pins
    pio_sm_exec(pio, sm_i2s, pio_encode_jmp(offset + audio_i2s_offset_entry_point));


    uint32_t sample_freq=44100;//44100;
    sample_freq*=2;
    uint32_t system_clock_frequency = clock_get_hz(clk_sys);
    uint32_t divider = system_clock_frequency * 4 / sample_freq; // avoid arithmetic overflow

    pio_sm_set_clkdiv_int_frac(pio, sm_i2s , divider >> 8u, divider & 0xffu);
    //printf("pio:%d\tsm:%d\n",pio,sm);
    pio_sm_set_enabled(pio, sm_i2s, true);
    

}


void i2s_init(){


    // uint offset = pio_add_program(PIO_I2S, &audio_i2s_program);
    // audio_i2s_program_init(PIO_I2S, offset, I2S_DATA_PIN , I2S_CLK_BASE_PIN);
    i2s_pio_init();

    int dma_i2s=dma_claim_unused_channel(true);
	int dma_i2s_ctrl=dma_claim_unused_channel(true);

    //printf("dma_i2s:[%d]\tdma_i2s_ctrl:[%d]\n",dma_i2s,dma_i2s_ctrl);

    //основной рабочий канал
	dma_channel_config cfg_dma = dma_channel_get_default_config(dma_i2s);
	channel_config_set_transfer_data_size(&cfg_dma, DMA_SIZE_32);
	channel_config_set_chain_to(&cfg_dma, dma_i2s_ctrl);// chain to other channel

	channel_config_set_read_increment(&cfg_dma, false);
	channel_config_set_write_increment(&cfg_dma, false);



	uint dreq=0;
	if (PIO_I2S==pio0) dreq=DREQ_PIO0_TX0+sm_i2s;
	if (PIO_I2S==pio1) dreq=DREQ_PIO1_TX0+sm_i2s;
    #ifdef PICO_RP2350
	if (PIO_I2S==pio2) dreq=DREQ_PIO2_TX0+sm_i2s;
    #endif
	channel_config_set_dreq(&cfg_dma, dreq);
    //printf("dreq:[%d]\n",dreq);

	dma_channel_configure(
		dma_i2s,
		&cfg_dma,
		&(PIO_I2S->txf[sm_i2s]),		// Write address
		&i2s_data,					// read address
		1<<10,					//
		false			 				// Don't start yet
	);


    //контрольный канал для основного(перезапуск)
    dma_channel_config cfg_dma1 = dma_channel_get_default_config(dma_i2s_ctrl);
	channel_config_set_transfer_data_size(&cfg_dma1, DMA_SIZE_32);
	channel_config_set_chain_to(&cfg_dma1, dma_i2s);// chain to other channel
	
	channel_config_set_read_increment(&cfg_dma1, false);
	channel_config_set_write_increment(&cfg_dma1, false);



	dma_channel_configure(
		dma_i2s_ctrl,
		&cfg_dma1,
		&dma_hw->ch[dma_i2s].al1_transfer_count_trig,	// Write address
		//&dma_hw->ch[dma_i2s].transfer_count,
		&trans_count_DMA,					// read address
		1,									//
		false								// Don't start yet
	);
    dma_start_channel_mask((1u << dma_i2s)) ;
};

void i2s_deinit(){

}

inline void i2s_out(int16_t l_out,int16_t r_out){i2s_data=(((uint16_t)l_out)<<16)|(((uint16_t)r_out));};