#include "mandelbrot.h"

#define  DMA_LIN_CHUNKS 10
#define  DMA_LIN_CHUNK_SIZE (ST7789_HEIGHT/DMA_LIN_CHUNKS)


static uint8_t buff0[ST7789_WIDTH * DMA_LIN_CHUNK_SIZE  * 3];
static uint8_t buff1[ST7789_WIDTH * DMA_LIN_CHUNK_SIZE *  3];


void mandel_init(void){
  
  memset(buff0, 0x00, sizeof(uint8_t) * ST7789_WIDTH * DMA_LIN_CHUNK_SIZE * 3);
  memset(buff1, 0x00, sizeof(uint8_t) * ST7789_WIDTH * DMA_LIN_CHUNK_SIZE * 3);
}
///// Done!
void calc_n_draw(uint16_t time) {
  color c;
  
  for (uint8_t yc = 0; yc <DMA_LIN_CHUNKS+1; yc++)
  {
    if (yc > 0 && get_transfer() == 0){
      ST7789_SetAddressWindow(0,(yc-1)*DMA_LIN_CHUNK_SIZE-1,ST7789_WIDTH-1,(yc)*DMA_LIN_CHUNK_SIZE-1);
      ST7789_Select();
      ST7789_DC_Set();
      dma_start(buff0, ST7789_WIDTH * DMA_LIN_CHUNK_SIZE * 3);
    }
    else { //Just initial contitions to start whole process
//      ST7789_SetAddressWindow(0,y-1,ST7789_WIDTH-1,y-1);
//      ST7789_Select();
//      ST7789_DC_Set();
      dma_start(buff0,  3);
    }

    while (get_transfer() == 1){
    
      for (uint8_t y = 0; y < DMA_LIN_CHUNK_SIZE; y++)
      {
        for (uint8_t x = 0; x < ST7789_WIDTH; x++)
        {
          color c;
          c.r = 255; c.g = 0; c.b = 255;
          buff1[y *ST7789_WIDTH*3+ x*3 + 0] = (time % 5)*51;
          buff1[y *ST7789_WIDTH*3+ x*3 + 1] = (time % 17)*15;
          buff1[y *ST7789_WIDTH*3+ x*3 + 2] = (time % 11)*23;
        }
      }
      memcpy(buff0, buff1,ST7789_WIDTH*DMA_LIN_CHUNK_SIZE * 3);
    }
  }
}
  
