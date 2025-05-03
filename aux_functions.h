#ifndef _AUX_FUN_H_
#define _AUX_FUN_H_
//*** process NMEA packet into Boat variables for display 
#include <Arduino.h>     //necessary for the String, uint16_t etc variable definitions
#include "Structures.h"  // to tell it about the _sBoatData and button structs.
#include <Arduino_GFX_Library.h>  // for the graphics functions

extern Arduino_RGB_Display  *gfx ; //  change if alternate displays !
bool processPacket(const char* buf,  _sBoatData &stringBD );

//** Graphics display functions ***
void GFXBorderBoxPrintf(_sButton button, const char* fmt, ...) ; // main button draw function, draws box and writes (one!) line of text in center/middle
// update lines of data (sequentially) in a button. -- faster as no clear and redraw of the whole button
void UpdateLinef(int font,_sButton& button, const char* fmt, ...) ; // (sequentially) types lines (in 'font') into the selected button space 
void UpdateLinef(uint16_t color,int font,_sButton& button, const char* fmt, ...) ; // (sequentially) types lines (in 'font') into the selected button space 

// Special 'update' for instruments.. Gives two size display, Digits in big, decimal and efverything after in small font
void UpdateDataTwoSize(bool horizCenter, bool vertCenter,int bigfont, int smallfont,_sButton button, _sInstData &data, const char *fmt);
// for even bigger, magnify the font  
void UpdateDataTwoSize(int magnify,bool horizCenter, bool vertCenter,int bigfont, int smallfont,_sButton button, _sInstData &data, const char *fmt);
// NEW version that will update the box etc on starting Simplifies the Display page settings when selecting by JSON..
void UpdateDataTwoSize(bool reset,const char *msg, const char *units,bool horizCenter, bool vertCenter, int bigfont, int smallfont, _sButton button, _sInstData &data, const char *fmt) ;


// Sub function to update (multiple) lines in a button (uses \n to separate lines )
void CommonCenteredSubUpdateLine(bool horizCenter, bool vertCenter, uint16_t color, int font, _sButton &button, const char *msg);
void CommonCenteredSubUpdateLine(uint16_t color, int font, _sButton &button, const char *msg); // default horizontal center only..

// Adds titles inside or outside button in the border area
void AddTitleBorderBox(int font,_sButton button, const char* fmt, ...);
void AddTitleInsideBox(int font,int pos, _sButton button, const char *fmt, ...);

//Draw triangle and circles etc, using Phv (x,y) structures
void PTriangleFill(Phv P1, Phv P2, Phv P3, uint16_t COLOUR);
void Pdrawline(Phv P1, Phv P2, uint16_t COLOUR) ;
void PfillCircle(Phv P1, int rad, uint16_t COLOUR);

//Note two instances so you can display two simultaneously?
void SCROLLGraph(bool reset,int instance, int dotsize, bool line,_sButton button, _sInstData &DATA, double dmin, double dmax, int font, const char *msg, const char *units);

void DrawGPSPlot(bool reset, _sButton button,_sBoatData BoatData, double magnification );
// Sub for functions, Circular keeps things in range.. eg degrees kept in range 0-359 
int Circular(int x, int min,int max);
int GraphRange( double data, int low, int high, double dmin ,double dmax );

// takes nmea field and places as double in struct data(.data)
// not used outside aux_functions ??
void toNewStruct(char *field, _sInstData &data); 
double Double_sInstDataAdd(_sInstData &data1, _sInstData &data);//returns a double of value data.data +data1.data;

int HexStringToBytes(const char *hexStr,
                     unsigned char *output,
                     unsigned int *outputLen);


#endif