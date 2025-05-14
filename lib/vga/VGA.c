#include "VGA.h"
#include "hardware/clocks.h"

#include "hardware/structs/pll.h"
#include "hardware/structs/systick.h"

#include "hardware/dma.h"
#include "hardware/irq.h"
#include <string.h>
#include "hardware/pio.h"
#include "pico/stdlib.h"
#include "stdlib.h"
#include <math.h>
//#include "fnt8x16_keyrus.h"

uint16_t pio_program_VGA_instructions[] = {

	//	 .wrap_target

	//	 .wrap_target
	0x6008, //  0: out	pins, 8
	//	 .wrap
	//	 .wrap
};

const struct pio_program pio_program_VGA = {
	.instructions = pio_program_VGA_instructions,
	.length = 1,
	.origin = -1,
};

//буферы строк
//количество буферов задавать кратно степени двойки
//
#define N_LINE_BUF_log2 (2)


#define N_LINE_BUF (1<<N_LINE_BUF_log2)
#define N_LINE_BUF_DMA (N_LINE_BUF)

//максимальный размер строки
#define LINE_SIZE_MAX (800)
//указатели на буферы строк
//выравнивание нужно для кольцевого буфера
static uint32_t rd_addr_DMA_CTRL[N_LINE_BUF*2]__attribute__ ((aligned (4*N_LINE_BUF_DMA))) ;
//непосредственно буферы строк

static uint32_t lines_buf[N_LINE_BUF][LINE_SIZE_MAX/4];

//графическая палитра
//2 массива палитр для миксования
static uint16_t vga_palette[2][256]__attribute__ ((aligned (4)));


static int ch_dma_ctrl=-1;

typedef struct VGA_MODE{
	int VS_begin;
	int VS_end;
	int V_visible_lines;
	int V_total_lines;

	int H_visible_len;
	int H_visible_begin;
	int HS_len;
	int H_len;

	uint8_t VS_TMPL;
	uint8_t VHS_TMPL;
	uint8_t HS_TMPL;
	uint8_t NO_SYNC_TMPL;
} VGA_MODE;

typedef struct G_BUFFER{
	uint width;
	uint height;
	int shift_x;
	int shift_y;
	uint8_t* data;
}G_BUFFER;

typedef struct TXT_BUFFER{
	uint N_COL;
	uint N_ROW;
	uint fnt_W;
	uint fnt_H;
	uint8_t* ch_data;
	uint8_t* color_data;
	uint *fnt;
}TXT_BUFFER;

static VGA_MODE vga_mode={
	.VS_begin=491,
	.VS_end=492,
	.V_total_lines=525,
	.V_visible_lines=480,
	.HS_len=96,
	.H_len=800,
	.H_visible_begin=144,
	.H_visible_len=640,

	.VS_TMPL=0b01000000,
	.VHS_TMPL=0b00000000,
	.HS_TMPL=0b10000000,
	.NO_SYNC_TMPL=0b11000000
};

static G_BUFFER g_buf={
	.data=NULL,
	.shift_x=0,
	.shift_y=0,
	.height=240,
	.width=320
};

static TXT_BUFFER txt_buf={
	.ch_data=NULL,
	.color_data=NULL,
	.N_COL=0,
	.N_ROW=0,
	.fnt_W=0,
	.fnt_H=0,
	.fnt=NULL//(uint *)fnt8x16
};

static g_mode active_mode;
//основная функция заполнения буферов видеоданных
void __not_in_flash_func(main_video_loop)(){
	static uint dma_inx_out=0;
	static uint lines_buf_inx=0;


	if (ch_dma_ctrl==-1) return;//не определен дма канал

	//получаем индекс выводимой строки
	uint dma_inx=(N_LINE_BUF_DMA-2+((dma_channel_hw_addr(ch_dma_ctrl)->read_addr-(uint32_t)rd_addr_DMA_CTRL)/4))%(N_LINE_BUF_DMA);


	//uint n_loop=(N_LINE_BUF_DMA+dma_inx-dma_inx_out)%N_LINE_BUF_DMA;

	static uint32_t line_active=0;
	static uint8_t* vbuf=NULL;
	static uint32_t frame_i=0;

	//while(n_loop--)
	while(dma_inx_out!=dma_inx){
		//режим VGA
		line_active++;
		if (line_active==vga_mode.V_total_lines) {line_active=0;frame_i++;vbuf=g_buf.data;}
		if (line_active<vga_mode.V_visible_lines){
			//зона изображения
			switch (active_mode){
				case g_mode_320x240x4bpp:
				case g_mode_320x240x8bpp:
				//320x240 графика
				if (line_active&1){
					//повтор шаблона строки
				} else{
					//новая строка
					lines_buf_inx=(lines_buf_inx+1)%N_LINE_BUF;
					uint8_t* out_buf8=(uint8_t*)lines_buf[lines_buf_inx];


					//зона синхры, затираем все шаблоны
					if(line_active<(N_LINE_BUF_DMA*2)){
						memset(out_buf8,vga_mode.HS_TMPL,vga_mode.HS_len);
						memset(out_buf8+vga_mode.HS_len,vga_mode.NO_SYNC_TMPL,vga_mode.H_visible_begin-vga_mode.HS_len);
						memset(out_buf8+vga_mode.H_visible_begin+vga_mode.H_visible_len,vga_mode.NO_SYNC_TMPL,vga_mode.H_len-(vga_mode.H_visible_begin+vga_mode.H_visible_len));

					}

					//формирование картинки
					int line=line_active/2;
					uint16_t* out_buf16=(uint16_t*)lines_buf[lines_buf_inx];
					out_buf16+=vga_mode.H_visible_begin/2;
					uint i_pal=(line+frame_i)&1;
					uint16_t* pallete16=vga_palette[i_pal];

					uint8_t* vbuf8;//=vbuf+(line)*g_buf.width;


					if (active_mode==g_mode_320x240x4bpp){	   //для 4-битного буфера
						vbuf8=vbuf+(line)*g_buf.width/2;
						if (vbuf!=NULL)
						for(int i=vga_mode.H_visible_len/32;i--;){
							*out_buf16++=pallete16[*vbuf8++>>4];
							*out_buf16++=pallete16[(*vbuf8&0x0f)];
							*out_buf16++=pallete16[*vbuf8++>>4];
							*out_buf16++=pallete16[(*vbuf8&0x0f)];
							*out_buf16++=pallete16[*vbuf8++>>4];
							*out_buf16++=pallete16[(*vbuf8&0x0f)];
							*out_buf16++=pallete16[*vbuf8++>>4];
							*out_buf16++=pallete16[(*vbuf8&0x0f)];

							*out_buf16++=pallete16[*vbuf8++>>4];
							*out_buf16++=pallete16[(*vbuf8&0x0f)];
							*out_buf16++=pallete16[*vbuf8++>>4];
							*out_buf16++=pallete16[(*vbuf8&0x0f)];
							*out_buf16++=pallete16[*vbuf8++>>4];
							*out_buf16++=pallete16[(*vbuf8&0x0f)];
							*out_buf16++=pallete16[*vbuf8++>>4];
							*out_buf16++=pallete16[(*vbuf8&0x0f)];
						}
					}
					if (active_mode==g_mode_320x240x8bpp) {	   //для 8-битного буфера
						vbuf8=vbuf+(line)*g_buf.width;
						if (vbuf!=NULL)
						// for(int i=vga_mode.H_visible_len/2;i--;)
						//		 {
						//				 *out_buf16++=pallete16[*vbuf8++];
						//		 }
						for(int i=vga_mode.H_visible_len/32;i--;){
							*out_buf16++=pallete16[*vbuf8++];
							*out_buf16++=pallete16[*vbuf8++];
							*out_buf16++=pallete16[*vbuf8++];
							*out_buf16++=pallete16[*vbuf8++];
							*out_buf16++=pallete16[*vbuf8++];
							*out_buf16++=pallete16[*vbuf8++];
							*out_buf16++=pallete16[*vbuf8++];
							*out_buf16++=pallete16[*vbuf8++];

							*out_buf16++=pallete16[*vbuf8++];
							*out_buf16++=pallete16[*vbuf8++];
							*out_buf16++=pallete16[*vbuf8++];
							*out_buf16++=pallete16[*vbuf8++];
							*out_buf16++=pallete16[*vbuf8++];
							*out_buf16++=pallete16[*vbuf8++];
							*out_buf16++=pallete16[*vbuf8++];
							*out_buf16++=pallete16[*vbuf8++];
						}
					}
				}
				break;
				case g_mode_text_80x30:
				//текстовый режим без чередования пикселей и строк
				//новая строка
				lines_buf_inx=(lines_buf_inx+1)%N_LINE_BUF;
				uint8_t* out_buf8=(uint8_t*)lines_buf[lines_buf_inx];


				//зона синхры, затираем все шаблоны
				if(line_active<(N_LINE_BUF_DMA*2)){
					memset(out_buf8,vga_mode.HS_TMPL,vga_mode.HS_len);
					memset(out_buf8+vga_mode.HS_len,vga_mode.NO_SYNC_TMPL,vga_mode.H_visible_begin-vga_mode.HS_len);
					memset(out_buf8+vga_mode.H_visible_begin+vga_mode.H_visible_len,vga_mode.NO_SYNC_TMPL,vga_mode.H_len-(vga_mode.H_visible_begin+vga_mode.H_visible_len));

				}

				//формирование картинки
				uint i_pal=(line_active+frame_i)&1;
				i_pal=0;
				uint16_t* pallete16=vga_palette[i_pal];

				out_buf8+=vga_mode.H_visible_begin;


				int i_txt=(line_active/txt_buf.fnt_H)*txt_buf.N_COL;
				uint8_t* chs=&txt_buf.ch_data[i_txt];
				uint8_t* chs_color=&txt_buf.color_data[i_txt];

				uint fnt_line=(line_active%txt_buf.fnt_H);
				uint8_t* fnt_buf=((uint8_t*)(txt_buf.fnt));

				uint fntH=txt_buf.fnt_H;
				uint fntW=txt_buf.fnt_W;

				uint16_t c_bg_pix16;
				uint8_t* c_bg_pix8=(uint8_t*)&c_bg_pix16;

				for(int i=vga_mode.H_visible_len/txt_buf.fnt_W;i--;){
					uint8_t ch=*chs++;
					uint8_t ch_color=*chs_color++;

					uint pixs=*(fnt_buf+(ch*fntH)+fnt_line);

					uint8_t c_pix=pallete16[ch_color&0xf];
					uint8_t c_bg=pallete16[(ch_color>>4)];
					if (pixs&0x1) *out_buf8++=c_pix; else *out_buf8++=c_bg; pixs>>=1;
					if (pixs&0x1) *out_buf8++=c_pix; else *out_buf8++=c_bg; pixs>>=1;
					if (pixs&0x1) *out_buf8++=c_pix; else *out_buf8++=c_bg; pixs>>=1;
					if (pixs&0x1) *out_buf8++=c_pix; else *out_buf8++=c_bg; pixs>>=1;
					if (pixs&0x1) *out_buf8++=c_pix; else *out_buf8++=c_bg; pixs>>=1;
					if (pixs&0x1) *out_buf8++=c_pix; else *out_buf8++=c_bg; pixs>>=1;
					if (pixs&0x1) *out_buf8++=c_pix; else *out_buf8++=c_bg; pixs>>=1;
					if (pixs&0x1) *out_buf8++=c_pix; else *out_buf8++=c_bg; pixs>>=1;
				}
				break;
				default:
				break;
			}
		} else {
			if((line_active>=vga_mode.VS_begin)&&(line_active<=vga_mode.VS_end)){//кадровый
				if (line_active==vga_mode.VS_begin){
					//новый шаблон
					lines_buf_inx=(lines_buf_inx+1)%N_LINE_BUF;
					uint8_t* out_buf8=(uint8_t*)lines_buf[lines_buf_inx];

					memset(out_buf8,vga_mode.VHS_TMPL,vga_mode.HS_len);
					memset(out_buf8+vga_mode.HS_len,vga_mode.VS_TMPL,vga_mode.H_len-vga_mode.HS_len);

				} else {
					//повтор шаблона
				}
			} else {
				//строчный
				if((line_active==(vga_mode.VS_end+1))||(line_active==(vga_mode.V_visible_lines))){
					//новый шаблон
					lines_buf_inx=(lines_buf_inx+1)%N_LINE_BUF;
					uint8_t* out_buf8=(uint8_t*)lines_buf[lines_buf_inx];
					memset(out_buf8,vga_mode.HS_TMPL,vga_mode.HS_len);
					memset(out_buf8+vga_mode.HS_len,vga_mode.NO_SYNC_TMPL,vga_mode.H_len-vga_mode.HS_len);
				} else {
					//повтор шаблона
				}
			}
		}
		rd_addr_DMA_CTRL[dma_inx_out]=(uint32_t)&lines_buf[lines_buf_inx];//включаем заполненный буфер в данные для вывода
		dma_inx_out=(dma_inx_out+1)%(N_LINE_BUF_DMA);
		dma_inx=(N_LINE_BUF_DMA-2+((dma_channel_hw_addr(ch_dma_ctrl)->read_addr-(uint32_t)rd_addr_DMA_CTRL)/4))%(N_LINE_BUF_DMA);
	}

}



void graphics_set_buffer(uint8_t *buffer){
	g_buf.data=buffer;
	// g_buf.height=height;
	// g_buf.width=width;
}

void graphics_set_textbuffer(uint8_t *ch_buffer,uint8_t* color_buf){
	txt_buf.ch_data=ch_buffer;
	txt_buf.color_data=color_buf;
};

void graphics_set_mode(g_mode mode){
	switch (mode){
		case g_mode_320x240x8bpp:
		active_mode=g_mode_320x240x8bpp;
		g_buf.height=240;
		g_buf.height=320;
		break;
		case g_mode_320x240x4bpp:
		active_mode=g_mode_320x240x4bpp;
		g_buf.height=240;
		g_buf.height=320;
		break;
		case g_mode_text_80x30:
		active_mode=g_mode_text_80x30;
		txt_buf.fnt_H=16;
		txt_buf.fnt_W=8;
		txt_buf.N_COL=80;
		txt_buf.N_ROW=30;
		break;

	}
};

void graphics_set_palette(uint8_t i, uint32_t color888) {
	uint8_t conv0[] = { 0b00, 0b00, 0b01, 0b10, 0b10, 0b10, 0b11, 0b11 };
	uint8_t conv1[] = { 0b00, 0b01, 0b01, 0b01, 0b10, 0b11, 0b11, 0b11 };
	uint8_t b = ((color888 & 0xff) / 42);
	uint8_t g = (((color888 >> 8) & 0xff) / 42);
	uint8_t r = (((color888 >> 16) & 0xff) / 42);
	uint8_t c_hi = (conv0[r] << 4) | (conv0[g] << 2) | conv0[b];
	uint8_t c_lo = (conv1[r] << 4) | (conv1[g] << 2) | conv1[b];
	uint16_t palette16_mask=(vga_mode.NO_SYNC_TMPL<<8)|(vga_mode.NO_SYNC_TMPL);
	vga_palette[0][i] = (((c_hi << 8) | c_lo) & 0x3f3f) | palette16_mask;
	vga_palette[1][i] = (((c_lo << 8) | c_hi) & 0x3f3f) | palette16_mask;
};

bool __not_in_flash_func (vga_timer_callback(repeating_timer_t *rt)) {
	main_video_loop();
	return true;
}


repeating_timer_t vga_timer;

void graphics_init(){
	//инициализация палитры по умолчанию

	uint8_t conv0[]={0b00,0b00,0b01,0b10,0b10,0b10,0b11,0b11};
	uint8_t conv1[]={0b00,0b01,0b01,0b01,0b10,0b11,0b11,0b11};
	for(int i=0;i<256;i++){
		uint8_t b=(i&0b11);

		uint8_t r=((i>>5)&0b111);
		uint8_t g=((i>>2)&0b111);

		// uint8_t c=0xc0|b|(r<<4)|(g<<2);
		uint8_t c_hi=0xc0|(conv0[r]<<4)|(conv0[g]<<2)|b;
		uint8_t c_lo=0xc0|(conv1[r]<<4)|(conv1[g]<<2)|b;

		//for conv
		// uint8_t c_hi=0xc0|(conv0[r]<<0)|(conv0[g]<<2)|(b<<4);
		// uint8_t c_lo=0xc0|(conv1[r]<<0)|(conv1[g]<<2)|(b<<4);

		vga_palette[0][i]=(c_hi<<8)|c_lo;
		vga_palette[1][i]=(c_lo<<8)|c_hi;
	}

	//инициализация PIO
	//загрузка программы в один из PIO
	uint offset = pio_add_program(PIO_VGA, &pio_program_VGA);
	uint sm=pio_claim_unused_sm(PIO_VGA, true);;

	for(int i=0;i<8;i++){gpio_init(beginVGA_PIN+i);gpio_set_dir(beginVGA_PIN+i,GPIO_OUT);pio_gpio_init(PIO_VGA, beginVGA_PIN+i);};//резервируем под выход PIO

	pio_sm_set_consecutive_pindirs(PIO_VGA, sm, beginVGA_PIN, 8, true);//конфигурация пинов на выход
	//pio_sm_config c = pio_vga_program_get_default_config(offset);

	pio_sm_config c = pio_get_default_sm_config();
	sm_config_set_wrap(&c, offset + 0, offset + (pio_program_VGA.length-1));


	sm_config_set_fifo_join(&c, PIO_FIFO_JOIN_TX);//увеличение буфера TX за счёт RX до 8-ми
	sm_config_set_out_shift(&c, true, true, 32);
	sm_config_set_out_pins(&c, beginVGA_PIN, 8);
	pio_sm_init(PIO_VGA, sm, offset, &c);

	pio_sm_set_enabled(PIO_VGA, sm, true);

	float fdiv=clock_get_hz(clk_sys)/(25175000.0);//частота VGA по умолчанию

	uint32_t div32=(uint32_t) (fdiv * (1 << 16)+0.5);
	PIO_VGA->sm[sm].clkdiv=div32&0xfffff000; //делитель для конкретной sm
	//инициализация DMA

	ch_dma_ctrl = dma_claim_unused_channel(true);
	int dma_chan = dma_claim_unused_channel(true);
	//основной ДМА канал для данных
	dma_channel_config c0 = dma_channel_get_default_config(dma_chan);
	channel_config_set_transfer_data_size(&c0, DMA_SIZE_32);

	channel_config_set_read_increment(&c0, true);
	channel_config_set_write_increment(&c0, false);

	uint dreq=DREQ_PIO1_TX0+sm;
	if (PIO_VGA==pio0) dreq=DREQ_PIO0_TX0+sm;

	channel_config_set_dreq(&c0, dreq);
	channel_config_set_chain_to(&c0, ch_dma_ctrl);						// chain to other channel

	dma_channel_configure(
		dma_chan,
		&c0,
		&PIO_VGA->txf[sm], // Write address
		lines_buf[0],			 // read address
		vga_mode.H_len/4, //
		false			 // Don't start yet
	);
	//канал DMA с данными для рабочего канала
	dma_channel_config c1 = dma_channel_get_default_config(ch_dma_ctrl);
	channel_config_set_transfer_data_size(&c1, DMA_SIZE_32);

	channel_config_set_read_increment(&c1, true);
	channel_config_set_write_increment(&c1, false);
	channel_config_set_ring(&c1,false,2+0+N_LINE_BUF_log2);

	channel_config_set_chain_to(&c1, dma_chan);						 // chain to other channel
	//channel_config_set_dreq(&c1, DREQ_PIO0_TX0);

	dma_channel_configure(
		ch_dma_ctrl,
		&c1,
		&dma_hw->ch[dma_chan].read_addr, // Write address
		rd_addr_DMA_CTRL,			 // read address
		1, //
		true			 // Don't start yet
	);

	int hz=30000;
	if (!add_repeating_timer_us(1000000 / hz, vga_timer_callback, NULL, &vga_timer)) {
		return ;
	}

};