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

//params for fractal
static float xcenter;
static float ycenter;
static float xcenter_init;
static float ycenter_init;
static float scale;
static float width;
static float width_div2;
static float xpos;
static float ypos;
static float xstart;
static float ystart;
static uint8_t max_iter;
static uint16_t drawcount;
static uint16_t fadecount;
static float x;
static float y;
static float xx;
static float yy;  
uint8_t n;

static color pc;
static HSL hs;
static color hues_colors[360];
static color rgb;
static float hue;
  
static float sr;
static float loglog2[360];
static uint16_t norm;

bool paused = false;

void mandel_init(void){

  memset(buff0, 0x00, sizeof(uint8_t) * ST7789_WIDTH * DMA_LIN_CHUNK_SIZE * COLOR_BYTES);
  memset(buff1, 0x00, sizeof(uint8_t) * ST7789_WIDTH * DMA_LIN_CHUNK_SIZE * COLOR_BYTES);
  
  //create hues table with constant saturation and lightness levels
  for (uint16_t i = 0; i < 360; i++)
  {
    HSL temp = {i, 0.95f, 0.6f};
    hues_colors[i] = HSLToRGB(temp);
    loglog2[i] = logf(log2f((float)i/5.625f));    
  }
  
  drawcount = 0;
  fadecount = 0;
  //  
  xcenter_init = -0.5f;
  ycenter_init = 	0.0f;
  //xcenter =   -0.91667f;
  //ycenter = 	-0.3150f;
  //xcenter =   (-1.258496193441468865475 - 1.258369659209541685475)/2.0f;
  //ycenter = 	(-0.382327821334196598258 - 0.382422783671997698258)/2.0f;
  xcenter =   (-0.112800637633519726519 - 0.112593751170314544823)/2.0f;
  ycenter = 	(-0.971759311452004927936 - 0.971966197915210109632)/2.0f;

  width   = 5.00f;
  paused = false;
  width_div2 = width/2.0f;
  xstart = xcenter_init - width_div2;
  ystart = ycenter_init - width_div2;
  scale = (float)width/ST7789_WIDTH;
  max_iter = 14 * powf((log10f(240.0f/width)),1.5f);
}

void mandel_zoom(float factor){

  fadecount +=1;
  drawcount +=4;
  drawcount %= 360;
  
  if (paused)
    return;
  
  //max zoom value below 0.000065f lost of float precission
  if (fabsf(width) < 0.0002f)
  {
    width = 0.0002f;
    paused = true;
  } 
  //fade from initial point to feature center (animated position shift at beginning)
  float fadeparam = (float)fadecount/40;
  if (fadeparam > 1.0f){
    fadeparam = 1.0f;
    fadecount = 40;
  }
  xstart = xcenter_init + (xcenter-xcenter_init)*fadeparam - width_div2;
  ystart = ycenter_init + (ycenter-ycenter_init)*fadeparam - width_div2;

  width -= width/24.0f;
  width_div2 = width/2.0f;

  scale = (float)width/ST7789_WIDTH;
  max_iter = 14 * powf((log10f(240/width)),1.5f);
}

void calc_n_draw(void) {
  
  bool init_frame = true;
  
  color c;
  norm = 0;
  hs.H = 0; hs.L =0.95f; hs.S = 0.6f;
  
  uint8_t* b0ptr = buff0;
  uint8_t* b1ptr = buff1;
  uint8_t px;
  uint8_t py;
  
  for (uint8_t yc = 0; yc <DMA_LIN_CHUNKS; yc++)
  {
    while (get_transfer() == 1 || init_frame){
      init_frame = false;
      for (py = 0; py < DMA_LIN_CHUNK_SIZE; py++)
      {
        ypos = ystart + (float)(yc*DMA_LIN_CHUNK_SIZE + py) * scale;
        xpos = xstart;
        for (px = 0; px < ST7789_WIDTH; px++)
        {
          xpos += scale;
          c = process_pixel();
          
#ifdef COLOR_MODE_16
          b1ptr[py *ST7789_WIDTH*COLOR_BYTES+ px*COLOR_BYTES + 0] = (uint8_t)(ST7789_RGBToColor(c.r,c.g,c.b)>>8);
          b1ptr[py *ST7789_WIDTH*COLOR_BYTES+ px*COLOR_BYTES + 1] = (uint8_t)(ST7789_RGBToColor(c.r,c.g,c.b)&0xFF);
#else
          b1ptr[py *ST7789_WIDTH*COLOR_BYTES+ px*COLOR_BYTES + 0] = c.r;
          b1ptr[py *ST7789_WIDTH*COLOR_BYTES+ px*COLOR_BYTES + 1] = c.g;
          b1ptr[py *ST7789_WIDTH*COLOR_BYTES+ px*COLOR_BYTES + 2] = c.b;
#endif
        }
      }
    }
    
    //swap buffer pointers
    uint8_t* tptr = b1ptr; b1ptr = b0ptr; b0ptr = tptr;
    
    ST7789_SetAddressWindow(0,(yc)*DMA_LIN_CHUNK_SIZE-1,ST7789_WIDTH-1,(yc+1)*DMA_LIN_CHUNK_SIZE-1);
    ST7789_Select();
    ST7789_DC_Set();
    dma_start(b0ptr, ST7789_WIDTH * DMA_LIN_CHUNK_SIZE * COLOR_BYTES);
  }
}

color process_pixel(void){
  
  x = xpos;
  y = ypos;

  n = 0;
  sr = 0;
  hs.H = 0;
  pc.r =0; pc.g = 0; pc.b =0;
  
  while (n < max_iter && sr < 6) {
    xx = x * x;
    yy = y * y;

    y = 2.0f * x * y + ypos;
    x = xx - yy + xpos;

    sr = xx+yy;

    n++;
  }


  if (n != (uint16_t)max_iter) {
      //"normalized iteration count" smoother gradients
      //"sr ranges 0...64 and is mapped on 360 values array
      
      //norm = (uint16_t)(((float)n*8)); //simple mode
      norm = (uint16_t)(((float)n + 1.0f - loglog2[(uint16_t)(sr*5.625f)])*8); 
    
      pc = hues_colors[(norm+drawcount)%360];
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
    float v1, v2;
    hue = (float)hsl.H / 360;
		v2 = (hsl.L < 0.5f) ? (hsl.L * (1 + hsl.S)) : ((hsl.L + hsl.S) - (hsl.L * hsl.S));
		v1 = 2 * hsl.L - v2;

		rgb.r = (unsigned char)(255 * HueToRGB(v1, v2, hue + (1.0f / 3)));
		rgb.g = (unsigned char)(255 * HueToRGB(v1, v2, hue));
		rgb.b = (unsigned char)(255 * HueToRGB(v1, v2, hue - (1.0f / 3)));
	}

	return rgb;
}
