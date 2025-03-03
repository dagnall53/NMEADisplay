/*******************************************************************************
Pins and defines for Arduino GFX - various versions!
 ******************************************************************************/
 
#ifndef _ESPGFDEF_H_
#define _ESPGFDEF_H_
//****  EARLY GFX VERSIONS HAVE DIFFERENT *BUS config 
#define GFX_BL 38
Arduino_ESP32RGBPanel *bus = new Arduino_ESP32RGBPanel(
  39 /* CS */, 48 /* SCK */, 47 /* SDA */,
  18 /* DE */, 17 /* VSYNC */, 16 /* HSYNC */, 21 /* PCLK */,
  11 /* R0 */, 12 /* R1 */, 13 /* R2 */, 14 /* R3 */, 0 /* R4 */,
  8 /* G0 */, 20 /* G1 */, 3 /* G2 */, 46 /* G3 */, 9 /* G4 */, 10 /* G5 */,
  4 /* B0 */, 5 /* B1 */, 6 /* B2 */, 7 /* B3 */, 15 /* B4 */
);
Arduino_ST7701_RGBPanel *gfx =  new Arduino_ST7701_RGBPanel(
    bus, GFX_NOT_DEFINED /* RST */, 0 /* rotation */,
    false /* IPS */, 480 /* width */, 480 /* height */,
    st7701_type1_init_operations, sizeof(st7701_type1_init_operations),     true /* BGR */,
    10 /* hsync_front_porch */, 8 /* hsync_pulse_width */, 50 /* hsync_back_porch */,
    10 /* vsync_front_porch */, 8 /* vsync_pulse_width */, 20 /* vsync_back_porch */);
    /*  Definition is:  Arduino_ST7701_RGBPanel(
        Arduino_ESP32RGBPanel *bus, int8_t rst = GFX_NOT_DEFINED, uint8_t r = 0,
        bool ips = false, int16_t w = ST7701_TFTWIDTH, int16_t h = ST7701_TFTHEIGHT,
        const uint8_t *init_operations = st7701_type1_init_operations,
        size_t init_operations_len = sizeof(st7701_type1_init_operations),
        bool bgr = true,
        uint16_t hsync_front_porch = 6, uint16_t hsync_pulse_width = 18, uint16_t hsync_back_porch = 24,
        uint16_t vsync_front_porch = 4, uint16_t vsync_pulse_width = 10, uint16_t vsync_back_porch = 16);
        */

//** OTHER PINS

#define TFT_BL GFX_BL

#define SD_SCK  48                //48
#define SD_MISO 41                //41
#define SD_MOSI 47                //47
#define SD_CS   42                //42

#define I2S_DOUT      40       //40
#define I2S_BCLK      1       //1
#define I2S_LRCK      2       //2  


#define TOUCH_INT -1          //-1
#define TOUCH_RST 38          // -1 (just uses power off?)
#define TOUCH_SDA  19
#define TOUCH_SCL  45
#define TOUCH_WIDTH  480
#define TOUCH_HEIGHT 480

// workss with (https://github.com/moononournation/Arduino_GFX) GFX 1.3.1 where:









#endif // _ESPGFDEF_H_
