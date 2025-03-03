 
#ifndef _KEYBOARD_H_
#define _KEYBOARD_H_
#include <Arduino_GFX_Library.h>

// if v131 extern Arduino_ST7701_RGBPanel *gfx ;  // declare the gfx structure so I can use GFX commands in Keyboard.cpp
extern Arduino_RGB_Display  *gfx ; //  change if alternate displays !

void keyboard(int type);
void Use_Keyboard(char* DATA, int sizeof_data);

void DrawKey(int Keysize, int x, int rows_down, int width, String text );

bool KeyOver(int x, int y, char * Key,int type);
bool XYinBox(int x,int y, int h,int v,int width,int height);

int KEYBOARD_Y(void);
int KEYBOARD_X(void);

String key(int x, int y, int type);



#endif