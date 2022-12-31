#include "lcd.h"

 #define MEM_TRIM    32  // Not enough memory for whole image buffer, 
                         // so trimming each side by 32 (cant trim less on stm32f401re)
                         // Real resolution is 176 x 176 after trim
 #define CHUNKS_NUM 2    // chunks for dma

/* If you're using DMA, then u need a "framebuffer" to store datas to be displayed.
 * If your MCU don't have enough RAM, please avoid using DMA(or set 5 to 1).
 * And if your MCU have enough RAM(even larger than full-frame size),
 * Then you can specify the framebuffer size to the full resolution below.
 */

#define HOR_LEN 	      (ST7789_WIDTH  - MEM_TRIM*2)
#define VER_LEN 	      (ST7789_HEIGHT - MEM_TRIM*2)
#define RGB_BYTES 	    3   // We need 3 bytes for RGB
#define DMA_MIN_SIZE    (VER_LEN)/CHUNKS_NUM
#define BUFF_SIZE       (HOR_LEN) * (VER_LEN) * RGB_BYTES

//static uint8_t  buffer[BUFF_SIZE];
//static uint32_t dma_chunk_size = (HOR_LEN) * DMA_MIN_SIZE * RGB_BYTES;


//Write command to ST7789 controller
static void ST7789_WriteCommand(uint8_t cmd)
{
	ST7789_Select();
	ST7789_DC_Clr();
	spi_transmit(&cmd, sizeof(cmd));
	ST7789_UnSelect();
}


//Write data to ST7789 controller

static void ST7789_WriteData(uint8_t *data, size_t data_size)
{
	ST7789_Select();
	ST7789_DC_Set();

  spi_transmit(data, (uint16_t)data_size);

	ST7789_UnSelect();
}

//Write data to ST7789 controller, simplify for 8bit data.

static void ST7789_WriteSmallData(uint8_t data)
{
	ST7789_Select();
	ST7789_DC_Set();
	spi_transmit(&data, sizeof(data));
	ST7789_UnSelect();
}

//Set the rotation direction of the display

void ST7789_SetRotation(uint8_t m)
{
	ST7789_WriteCommand(ST7789_MADCTL);	// MADCTL
	switch (m) {
	case 0:
		ST7789_WriteSmallData(ST7789_MADCTL_MX | ST7789_MADCTL_MY | ST7789_MADCTL_RGB);
		break;
	case 1:
		ST7789_WriteSmallData(ST7789_MADCTL_MY | ST7789_MADCTL_MV | ST7789_MADCTL_RGB);
		break;
	case 2:
		ST7789_WriteSmallData(ST7789_MADCTL_RGB);
		break;
	case 3:
		ST7789_WriteSmallData(ST7789_MADCTL_MX | ST7789_MADCTL_MV | ST7789_MADCTL_RGB);
		break;
	default:
		break;
	}
}

//Set address of DisplayWindow
void ST7789_SetAddressWindow(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1)
{
	ST7789_Select();
	uint16_t x_start = x0 + X_SHIFT, x_end = x1 + X_SHIFT;
	uint16_t y_start = y0 + Y_SHIFT, y_end = y1 + Y_SHIFT;
	
	/* Column Address set */
	ST7789_WriteCommand(ST7789_CASET); 
	{
		uint8_t data[] = {x_start >> 8, x_start & 0xFF, x_end >> 8, x_end & 0xFF};
		ST7789_WriteData(data, sizeof(data));
	}

	/* Row Address set */
	ST7789_WriteCommand(ST7789_RASET);
	{
		uint8_t data[] = {y_start >> 8, y_start & 0xFF, y_end >> 8, y_end & 0xFF};
		ST7789_WriteData(data, sizeof(data));
	}
	/* Write to RAM */
	ST7789_WriteCommand(ST7789_RAMWR);
	ST7789_UnSelect();
}


void ST7789_SpiInit(void)
{
  gpio_init(GPIO_PIN_OLED_RST);
  gpio_init(GPIO_PIN_OLED_CS);
  gpio_init(GPIO_PIN_OLED_DC);

	ST7789_Init();
}

//Initialize ST7789 controller
 void ST7789_Init(void)
{
 
    delay_nops(25);
    ST7789_RST_Clr();
    delay_nops(25);
    ST7789_RST_Set();
    delay_nops(50);
		
  	ST7789_WriteCommand(0xB2);				//	Porch control
    {
      uint8_t data[] = {0x0C, 0x0C, 0x00, 0x33, 0x33};
      ST7789_WriteData(data, sizeof(data));
    }
    ST7789_SetRotation(ST7789_ROTATION);	//	MADCTL (Display Rotation)
	
	/* Internal LCD Voltage generator settings */
    ST7789_WriteCommand(0XB7);				//	Gate Control
    ST7789_WriteSmallData(0x35);			//	Default value
    ST7789_WriteCommand(0xBB);				//	VCOM setting
    ST7789_WriteSmallData(0x19);			//	0.725v (default 0.75v for 0x20)
    ST7789_WriteCommand(0xC0);				//	LCMCTRL	
    ST7789_WriteSmallData (0x2C);			//	Default value
    ST7789_WriteCommand (0xC2);				//	VDV and VRH command Enable
    ST7789_WriteSmallData (0x01);			//	Default value
    ST7789_WriteCommand (0xC3);				//	VRH set
    ST7789_WriteSmallData (0x12);			//	+-4.45v (defalut +-4.1v for 0x0B)
    ST7789_WriteCommand (0xC4);				//	VDV set
    ST7789_WriteSmallData (0x20);			//	Default value
    ST7789_WriteCommand (0xC6);				//	Frame rate control in normal mode
    ST7789_WriteSmallData (0x0F);			//	Default value (60HZ)
    ST7789_WriteCommand (0xD0);				//	Power control
    ST7789_WriteSmallData (0xA4);			//	Default value
    ST7789_WriteSmallData (0xA1);			//	Default value
	/**************** Division line ****************/

    ST7789_WriteCommand(ST7789_COLMOD);		//	Set color mode
#ifdef COLOR_MODE_16
    ST7789_WriteSmallData(ST7789_COLOR_MODE_16bit);
#else 
    ST7789_WriteSmallData(ST7789_COLOR_MODE_18bit);
#endif

    ST7789_WriteCommand(0xE0);        //Gamma correction
    {
      uint8_t data[] = {0xd0,0x08,0x11,0x08,0x0c,0x15,0x39,0x33,0x50,0x36,0x13,0x14,0x29,0x2d};
      ST7789_WriteData(data, sizeof(data));
    }

    ST7789_WriteCommand(0xE1);        //Gamma correction
    {
      uint8_t data[] = {0xd0,0x08,0x10,0x08,0x06,0x06,0x39,0x44,0x51,0x0b,0x16,0x14,0x2f,0x31};
      ST7789_WriteData(data, sizeof(data));
    }
    
    ST7789_WriteCommand (ST7789_INVON);		//	Inversion ON
    ST7789_WriteCommand (ST7789_SLPOUT);	//	Out of sleep mode
  	ST7789_WriteCommand (ST7789_NORON);		//	Normal Display on
  	ST7789_WriteCommand (ST7789_DISPON);	//	Main screen turned on	

}


void ST7789_ClearAll(void)
{

	ST7789_SetAddressWindow(0, 0, ST7789_WIDTH - 1, ST7789_HEIGHT - 1);
	ST7789_Select();
  ST7789_DC_Set();  
		uint16_t x, y;
		for (y = 0; y < ST7789_HEIGHT; y++){
        uint8_t data[ST7789_WIDTH*3]={0x00};
        dma_start(data, sizeof(data));
        while (get_transfer() == 1){}
      }
	ST7789_UnSelect();
}

uint16_t ST7789_RGBToColor(uint8_t r, uint8_t g, uint8_t b) {
	return (((uint16_t)r >> 3) << 11) | (((uint16_t)g >> 2) << 5) | ((uint16_t)b >> 3);
}
