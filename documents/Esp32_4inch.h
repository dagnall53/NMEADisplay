/*******************************************************************************
Pins and defines for GFX - various versions!
 ******************************************************************************/
 
#ifndef _ESPGFDEF_H_
#define _ESPGFDEF_H_
//****  Later  GFX VERSIONS HAVE Refactored  *BUS config and miss the ips (colour inversion??) setup  
#define GFX_BL 38
Arduino_DataBus *bus = new Arduino_SWSPI(
  GFX_NOT_DEFINED /* DC */,
  39 /* CS */,                // Chip Select pin
  48 /* SCK */,               // Clock pin
  47 /* SDA ? */,             // Master Out Slave In pin
  GFX_NOT_DEFINED /* MISO */  // Master In Slave Out pin (not used)
);

Arduino_ESP32RGBPanel *rgbpanel = new Arduino_ESP32RGBPanel(  // MY BOARD modified pin numbers
  18 /* DE */, 17 /* VSYNC */, 16 /* HSYNC */, 21 /* PCLK */,
  11 /* R0 */, 12 /* R1 */, 13 /* R2 */, 14 /* R3 */, 0 /* R4 */,
  8 /* G0/P22 */, 20 /* G1/P23 */, 3 /* G2/P24 */, 46 /* G3/P25 */, 9 /* G4/P26 */, 10 /* G5 */,
  4 /* B0 */, 5 /* B1 */, 6 /* B2 */, 7 /* B3 */, 15 /* B4 */ ,
  1 /* hsync_polarity */,      // Horizontal sync polarity
  10 /* hsync_front_porch */,  // Horizontal front porch duration
  8 /* hsync_pulse_width */,   // Horizontal pulse width
  50 /* hsync_back_porch */,   // Horizontal back porch duration
  1 /* vsync_polarity */,      // Vertical sync polarity
  10 /* vsync_front_porch */,  // Vertical front porch duration
  8 /* vsync_pulse_width */,   // Vertical pulse width
  20 /* vsync_back_porch */    // Vertical back porch duration
                               // ,1, 30000000 // Uncomment for additional parameters if needed
);
// Initialize ST7701 display // comments at end of  https://github.com/Makerfabs/ESP32-S3-Parallel-TFT-with-Touch-4inch
Arduino_RGB_Display *gfx = new Arduino_RGB_Display(
  480 /* width */,  480 /* height */,  rgbpanel,
  0 /* rotation */,  true /* auto_flush */,  bus, // as defined in Arduino_DataBus *bus 
  GFX_NOT_DEFINED /* RST */,  st7701_type9_init_operations,  sizeof(st7701_type9_init_operations));
/*
Note: Use Type1 modified 
WRITE_COMMAND_8, 0x20, // 0x20 normal, 0x21 IPS
WRITE_C8_D8, 0x3A, 0x50, // 0x70 RGB888, 0x60 RGB666, 0x50 RGB565
or type 9

 
*/
//** OTHER PINS

#define TFT_BL GFX_BL
#define I2C_SDA_PIN 19     //19
#define I2C_SCL_PIN 45     //45


#define SD_SCK  48                //48
#define SD_MISO 41                //41
#define SD_MOSI 47                //47
#define SD_CS   42                //42

#define I2S_DOUT      40       //40
#define I2S_BCLK      1       //1
#define I2S_LRCK      2       //2  

//#include "Touch.h"
/* for these pins
#define TOUCH_INT -1          //-1
#define TOUCH_RST 38          // important not -1
#define TOUCH_SDA  19
#define TOUCH_SCL  45
#define TOUCH_WIDTH  480
#define TOUCH_HEIGHT 480
*/

// workss with (https://github.com/moononournation/Arduino_GFX) GFX 1.3.1 where:









#endif // _ESPGFDEF_H_

