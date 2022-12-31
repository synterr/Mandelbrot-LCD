#ifndef MANDELBROT_H
#define MANDELBROT_H

#include <stdint.h>
#include <stdio.h>
#include "dma.h"
#include "lcd.h"
#include "tools.h"


typedef struct color {
  uint8_t r;
  uint8_t g;
  uint8_t b;
} color;


typedef struct HSL
{
	int H;
	float S;
	float L;
}HSL;

extern bool paused;

void mandel_init(void);
void mandel_zoom(float factor);
void calc_n_draw(void);
color process_pixel(void);

float HueToRGB(float v1, float v2, float vH);
color HSLToRGB(HSL hsl);

#endif

