#ifndef MANDELBROT_H
#define MANDELBROT_H

#include <stdint.h>
#include <stdio.h>
#include "dma.h"
#include "lcd.h"



typedef struct color {
  uint8_t r;
  uint8_t g;
  uint8_t b;
} color;

void mandel_init(void);
void calc_n_draw(uint16_t time);

#endif

