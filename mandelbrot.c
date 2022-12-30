#include "mandelbrot.h"

#ifdef COLOR_MODE_16
  #define COLOR_BYTES 2
#else
  #define COLOR_BYTES 3
#endif

#define  DMA_LIN_CHUNKS 12 //Screen space will be divided in #num chunks vertically
#define  DMA_LIN_CHUNK_SIZE (ST7789_HEIGHT/DMA_LIN_CHUNKS)

//chunks swap buffers

static uint8_t buff0[ST7789_WIDTH * DMA_LIN_CHUNK_SIZE * COLOR_BYTES];
static uint8_t buff1[ST7789_WIDTH * DMA_LIN_CHUNK_SIZE * COLOR_BYTES];


static bool chunk_completed = false;
//params for fractal
static float xcenter;
static float ycenter;
static float scale;
static float width;
static float width_div2;
static float xpos;
static float ypos;
static float xstart;
static float ystart;
static uint8_t max_iter;
static uint16_t drawcount;
static float zoom_factor_sum=1.0f;
static color pc;
static HSL hs;
static float x;
static float y;
static float xx;
static float yy;  
uint8_t n;

static color rgb;
static float v1, v2;
static float hue;
  
static float sr;
static uint16_t norm;

bool paused = false;

void mandel_init(void){

  memset(buff0, 0x00, sizeof(uint8_t) * ST7789_WIDTH * DMA_LIN_CHUNK_SIZE * COLOR_BYTES);
  memset(buff1, 0x00, sizeof(uint8_t) * ST7789_WIDTH * DMA_LIN_CHUNK_SIZE * COLOR_BYTES);
  
  drawcount = 0;
  xcenter =   -1.263f;
  ycenter = 	-0.389f;
  width   = 4.0f;
  paused = false;
  width_div2 = width/2.0f;
  xstart = xcenter - width_div2;
  ystart = ycenter - width_div2;
  max_iter = 12 * powf((log10f(420.0f/width)),1.5f);
}

void mandel_zoom(float factor){
  
  if (paused)
    return;

  if (fabsf(width) < 0.0068f)
  {
    width = 0.01f;
    paused = true;
    //zoom_factor_sum=1.0f;
  }

    max_iter = 12 * powf((log10f(420/width)),1.5f);
    scale = (float)width/ST7789_WIDTH;
    float zstep = factor/(max_iter/16);
    width -= zstep;
    width_div2 = width/2.0f;
    xstart = xcenter - width_div2;
    ystart = ycenter - width_div2;
    zoom_factor_sum+=factor;

}

void calc_n_draw(void) {
  color c;

  norm = 0;
  hs.H = 0; hs.L =0.5f; hs.S = 0.7f;
  
  drawcount+=10;
  drawcount %=256;
  bool init_frame = true;
  
  for (uint8_t yc = 0; yc <DMA_LIN_CHUNKS; yc++)
  {
    if (get_transfer() == 1 || init_frame){
      init_frame = false;
      for (uint8_t py = 0; py < DMA_LIN_CHUNK_SIZE; py++)
      {
        ypos = ystart + (float)(yc*DMA_LIN_CHUNK_SIZE + py) * scale;
        for (uint8_t px = 0; px < ST7789_WIDTH; px++)
        {
          xpos = xstart + (float)px * scale;
          c = process_pixel();
          
#ifdef COLOR_MODE_16
          buff1[py *ST7789_WIDTH*COLOR_BYTES+ px*COLOR_BYTES + 0] = (uint8_t)(ST7789_RGBToColor(c.r,c.g,c.b)>>8);
          buff1[py *ST7789_WIDTH*COLOR_BYTES+ px*COLOR_BYTES + 1] = (uint8_t)(ST7789_RGBToColor(c.r,c.g,c.b)&0xFF);
#else
          buff1[py *ST7789_WIDTH*COLOR_BYTES+ px*COLOR_BYTES + 0] = c.r;
          buff1[py *ST7789_WIDTH*COLOR_BYTES+ px*COLOR_BYTES + 1] = c.g;
          buff1[py *ST7789_WIDTH*COLOR_BYTES+ px*COLOR_BYTES + 2] = c.b;
#endif
        }
      }
      memcpy(buff0, buff1,ST7789_WIDTH*DMA_LIN_CHUNK_SIZE * COLOR_BYTES);
    }
    
    if (get_transfer() == 0){
      ST7789_SetAddressWindow(0,(yc)*DMA_LIN_CHUNK_SIZE-1,ST7789_WIDTH-1,(yc+1)*DMA_LIN_CHUNK_SIZE-1);
      ST7789_Select();
      ST7789_DC_Set();
      dma_start(buff0, ST7789_WIDTH * DMA_LIN_CHUNK_SIZE * COLOR_BYTES);
    }
//    else if (get_transfer() == 0) { //Just initial contitions to start whole process
//      dma_start(buff0, 3);
//    }

  }
}

color process_pixel(void){
  
  x = xpos;
  y = ypos;

  n = 0;
  sr = 0;
  hs.H = 0;
  pc.r =0; pc.g = 0; pc.b =0;
  
  while (n < max_iter && sr < 8) {
    xx = x * x;
    yy = y * y;

    y = 2.0f * x * y + ypos;
    x = xx - yy + xpos;

    sr = xx+yy;

    n++;
  }


  if (n != (uint16_t)max_iter) {
    
//    norm = (uint16_t)(((float)n/max_iter)*256);
//    pc.r = (uint8_t)((norm))%256;
//    pc.g = (uint8_t)(255-(norm))%256;
//    pc.b = (uint8_t)((norm*4))%256;
    
    norm = (uint16_t)(((float)n/max_iter*360));
    hs.H = (uint16_t)(norm + drawcount/5)%360;  
    pc = HSLToRGB(hs);
  }
  return pc;
}


float HueToRGB(float v1, float v2, float vH)
{
	if (vH < 0)
		vH += 1;

	if (vH > 1)
		vH -= 1;

	if ((6 * vH) < 1)
		return (v1 + (v2 - v1) * 6 * vH);

	if ((2 * vH) < 1)
		return v2;

	if ((3 * vH) < 2)
		return (v1 + (v2 - v1) * ((2.0f / 3) - vH) * 6);

	return v1;
}

color HSLToRGB( HSL hsl) {
	
	if (hsl.S == 0.0f)
	{
		rgb.r = rgb.g = rgb.b = (unsigned char)(hsl.L * 255);
	}
	else
	{
    hue = (float)hsl.H / 360;

		v2 = (hsl.L < 0.5f) ? (hsl.L * (1 + hsl.S)) : ((hsl.L + hsl.S) - (hsl.L * hsl.S));
		v1 = 2 * hsl.L - v2;

		rgb.r = (unsigned char)(255 * HueToRGB(v1, v2, hue + (1.0f / 3)));
		rgb.g = (unsigned char)(255 * HueToRGB(v1, v2, hue));
		rgb.b = (unsigned char)(255 * HueToRGB(v1, v2, hue - (1.0f / 3)));
	}

	return rgb;
}
