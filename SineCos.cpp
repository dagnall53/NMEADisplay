
#include "SineCos.h"
#include <Arduino.h>

/*

SOME HELPER FUNCTIONS for Compass type displays.
A table of values of Sin and Cos *100 for all values from 0 to 360 degrees 
 POINTERS RELATED to the Center of the Rose position of the center of the compass rose
*/ 
const int ROSE_CENTER_X =  240;
const int ROSE_CENTER_Y = 240;

// For values of ROSE_RADIUS below 127 we can use 'char' to save space.
const char ROSE_RADIUS = 100;
// Putting the table of constants into FLASH/PROGMEM to save SRAM

const char SINE_TABLE[90] PROGMEM =  {
  // Casting the result as 'char' avoids warning messages about narrowing
  char(sin(0 * 2 * PI / 360.0) * ROSE_RADIUS + 0.5),
  char(sin(1 * 2 * PI / 360.0) * ROSE_RADIUS + 0.5),
  char(sin(2 * 2 * PI / 360.0) * ROSE_RADIUS + 0.5),
  char(sin(3 * 2 * PI / 360.0) * ROSE_RADIUS + 0.5),
  char(sin(4 * 2 * PI / 360.0) * ROSE_RADIUS + 0.5),
  char(sin(5 * 2 * PI / 360.0) * ROSE_RADIUS + 0.5),
  char(sin(6 * 2 * PI / 360.0) * ROSE_RADIUS + 0.5),
  char(sin(7 * 2 * PI / 360.0) * ROSE_RADIUS + 0.5),
  char(sin(8 * 2 * PI / 360.0) * ROSE_RADIUS + 0.5),
  char(sin(9 * 2 * PI / 360.0) * ROSE_RADIUS + 0.5),
  char(sin(10 * 2 * PI / 360.0) * ROSE_RADIUS + 0.5),
  char(sin(11 * 2 * PI / 360.0) * ROSE_RADIUS + 0.5),
  char(sin(12 * 2 * PI / 360.0) * ROSE_RADIUS + 0.5),
  char(sin(13 * 2 * PI / 360.0) * ROSE_RADIUS + 0.5),
  char(sin(14 * 2 * PI / 360.0) * ROSE_RADIUS + 0.5),
  char(sin(15 * 2 * PI / 360.0) * ROSE_RADIUS + 0.5),
  char(sin(16 * 2 * PI / 360.0) * ROSE_RADIUS + 0.5),
  char(sin(17 * 2 * PI / 360.0) * ROSE_RADIUS + 0.5),
  char(sin(18 * 2 * PI / 360.0) * ROSE_RADIUS + 0.5),
  char(sin(19 * 2 * PI / 360.0) * ROSE_RADIUS + 0.5),
  char(sin(20 * 2 * PI / 360.0) * ROSE_RADIUS + 0.5),
  char(sin(21 * 2 * PI / 360.0) * ROSE_RADIUS + 0.5),
  char(sin(22 * 2 * PI / 360.0) * ROSE_RADIUS + 0.5),
  char(sin(23 * 2 * PI / 360.0) * ROSE_RADIUS + 0.5),
  char(sin(24 * 2 * PI / 360.0) * ROSE_RADIUS + 0.5),
  char(sin(25 * 2 * PI / 360.0) * ROSE_RADIUS + 0.5),
  char(sin(26 * 2 * PI / 360.0) * ROSE_RADIUS + 0.5),
  char(sin(27 * 2 * PI / 360.0) * ROSE_RADIUS + 0.5),
  char(sin(28 * 2 * PI / 360.0) * ROSE_RADIUS + 0.5),
  char(sin(29 * 2 * PI / 360.0) * ROSE_RADIUS + 0.5),
  char(sin(30 * 2 * PI / 360.0) * ROSE_RADIUS + 0.5),
  char(sin(31 * 2 * PI / 360.0) * ROSE_RADIUS + 0.5),
  char(sin(32 * 2 * PI / 360.0) * ROSE_RADIUS + 0.5),
  char(sin(33 * 2 * PI / 360.0) * ROSE_RADIUS + 0.5),
  char(sin(34 * 2 * PI / 360.0) * ROSE_RADIUS + 0.5),
  char(sin(35 * 2 * PI / 360.0) * ROSE_RADIUS + 0.5),
  char(sin(36 * 2 * PI / 360.0) * ROSE_RADIUS + 0.5),
  char(sin(37 * 2 * PI / 360.0) * ROSE_RADIUS + 0.5),
  char(sin(38 * 2 * PI / 360.0) * ROSE_RADIUS + 0.5),
  char(sin(39 * 2 * PI / 360.0) * ROSE_RADIUS + 0.5),
  char(sin(40 * 2 * PI / 360.0) * ROSE_RADIUS + 0.5),
  char(sin(41 * 2 * PI / 360.0) * ROSE_RADIUS + 0.5),
  char(sin(42 * 2 * PI / 360.0) * ROSE_RADIUS + 0.5),
  char(sin(43 * 2 * PI / 360.0) * ROSE_RADIUS + 0.5),
  char(sin(44 * 2 * PI / 360.0) * ROSE_RADIUS + 0.5),
  char(sin(45 * 2 * PI / 360.0) * ROSE_RADIUS + 0.5),
  char(sin(46 * 2 * PI / 360.0) * ROSE_RADIUS + 0.5),
  char(sin(47 * 2 * PI / 360.0) * ROSE_RADIUS + 0.5),
  char(sin(48 * 2 * PI / 360.0) * ROSE_RADIUS + 0.5),
  char(sin(49 * 2 * PI / 360.0) * ROSE_RADIUS + 0.5),
  char(sin(50 * 2 * PI / 360.0) * ROSE_RADIUS + 0.5),
  char(sin(51 * 2 * PI / 360.0) * ROSE_RADIUS + 0.5),
  char(sin(52 * 2 * PI / 360.0) * ROSE_RADIUS + 0.5),
  char(sin(53 * 2 * PI / 360.0) * ROSE_RADIUS + 0.5),
  char(sin(54 * 2 * PI / 360.0) * ROSE_RADIUS + 0.5),
  char(sin(55 * 2 * PI / 360.0) * ROSE_RADIUS + 0.5),
  char(sin(56 * 2 * PI / 360.0) * ROSE_RADIUS + 0.5),
  char(sin(57 * 2 * PI / 360.0) * ROSE_RADIUS + 0.5),
  char(sin(58 * 2 * PI / 360.0) * ROSE_RADIUS + 0.5),
  char(sin(59 * 2 * PI / 360.0) * ROSE_RADIUS + 0.5),
  char(sin(60 * 2 * PI / 360.0) * ROSE_RADIUS + 0.5),
  char(sin(61 * 2 * PI / 360.0) * ROSE_RADIUS + 0.5),
  char(sin(62 * 2 * PI / 360.0) * ROSE_RADIUS + 0.5),
  char(sin(63 * 2 * PI / 360.0) * ROSE_RADIUS + 0.5),
  char(sin(64 * 2 * PI / 360.0) * ROSE_RADIUS + 0.5),
  char(sin(65 * 2 * PI / 360.0) * ROSE_RADIUS + 0.5),
  char(sin(66 * 2 * PI / 360.0) * ROSE_RADIUS + 0.5),
  char(sin(67 * 2 * PI / 360.0) * ROSE_RADIUS + 0.5),
  char(sin(68 * 2 * PI / 360.0) * ROSE_RADIUS + 0.5),
  char(sin(69 * 2 * PI / 360.0) * ROSE_RADIUS + 0.5),
  char(sin(70 * 2 * PI / 360.0) * ROSE_RADIUS + 0.5),
  char(sin(71 * 2 * PI / 360.0) * ROSE_RADIUS + 0.5),
  char(sin(72 * 2 * PI / 360.0) * ROSE_RADIUS + 0.5),
  char(sin(73 * 2 * PI / 360.0) * ROSE_RADIUS + 0.5),
  char(sin(74 * 2 * PI / 360.0) * ROSE_RADIUS + 0.5),
  char(sin(75 * 2 * PI / 360.0) * ROSE_RADIUS + 0.5),
  char(sin(76 * 2 * PI / 360.0) * ROSE_RADIUS + 0.5),
  char(sin(77 * 2 * PI / 360.0) * ROSE_RADIUS + 0.5),
  char(sin(78 * 2 * PI / 360.0) * ROSE_RADIUS + 0.5),
  char(sin(79 * 2 * PI / 360.0) * ROSE_RADIUS + 0.5),
  char(sin(80 * 2 * PI / 360.0) * ROSE_RADIUS + 0.5),
  char(sin(81 * 2 * PI / 360.0) * ROSE_RADIUS + 0.5),
  char(sin(82 * 2 * PI / 360.0) * ROSE_RADIUS + 0.5),
  char(sin(83 * 2 * PI / 360.0) * ROSE_RADIUS + 0.5),
  char(sin(84 * 2 * PI / 360.0) * ROSE_RADIUS + 0.5),
  char(sin(85 * 2 * PI / 360.0) * ROSE_RADIUS + 0.5),
  char(sin(86 * 2 * PI / 360.0) * ROSE_RADIUS + 0.5),
  char(sin(87 * 2 * PI / 360.0) * ROSE_RADIUS + 0.5),
  char(sin(88 * 2 * PI / 360.0) * ROSE_RADIUS + 0.5),
  char(sin(89 * 2 * PI / 360.0) * ROSE_RADIUS + 0.5)
};


// Draw the compass pointer at an angle in degrees
/*
void drawCompassPointer(int angle) {
  int x_offset, y_offset;  // The resulting offsets from the center point
 // Normalize the angle to the range 0 to 359
  while (angle < 0)
    angle += 360;
    
  while (angle > 359)
    angle -= 360;
    
  x_offset = getSine(angle);
  y_offset = getCosine(angle);
  // The actual drawing command will depend on your display library
  
//  drawLine(ROSE_CENTER_X, ROSE_CENTER_Y, ROSE_CENTER_X + x_offset, ROSE_CENTER_Y + y_offset);
}
*/
int getSine(int angle) {
  // For angles less than 90, use the table directly
  while (angle < 0) angle += 360;
  while (angle > 359) angle -= 360;


  if (angle < 90)
    return pgm_read_byte_near(&SINE_TABLE[angle]);

  if (angle < 180) {
    //  90 to 179: flip direction
    return pgm_read_byte_near(&SINE_TABLE[179 - angle]);
  }

  if (angle < 270) {
    // 180 to 269 flip sign
    return -pgm_read_byte_near(&SINE_TABLE[angle - 180]);
  }
  
  // 270 to 359 flip sign and direction
  return -pgm_read_byte_near(&SINE_TABLE[359 - angle]);
}

// Cosine(angle) == Sine(angle-90)
int getCosine(int angle) {
    while (angle < 0) angle += 360;
  while (angle > 359) angle -= 360;
  // Keep angle-90 between 0 and 359
  if (angle < 90)
    angle += 360;

  return getSine(angle - 90);
}

