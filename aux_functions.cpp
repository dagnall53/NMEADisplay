/* 
Original version by Dr.András Szép under GNU General Public License (GPL).
but highly modified! 
*/

#include <Arduino.h>  //necessary for the String variables
#include "aux_functions.h"
// defines gfx and has pin details of the display
#include <NMEA0183.h>  // for the TL NMEA0183 library functions
#include <NMEA0183Msg.h>
#include <NMEA0183Messages.h>

extern int text_offset;
extern int MasterFont;
extern void setFont(int);
extern int text_height;

extern double NMEA0183GetDouble(const char *data);  // have to do this as its local to NMEA0183Messagesmessages.cpp!

extern char *pTOKEN;  // const ?global? pointer of type char, used to get the fields in a nmea sentence
                      // when a new sentence is processed we advance this pointer n positions
                      // after the beginning of the sentence such that it points to the 1st field.
                      // used by function void FillToken(char * ptr) in

char Field[20][18];  // for the extracted NMEA fields

#define MAX_NMEA_FIELDS 64
char nmeaLine[MAX_NMEA0183_MSG_BUF_LEN];  //NMEA0183 message buffer
size_t i = 0, j = 1;                      //indexers
uint8_t *pointer_to_int;                  //pointer to void *data (!)
int noOfFields = MAX_NMEA_FIELDS;         //max number of NMEA0183 fields
int Num_Conversions, Num_DataFields;      //May be needed in sub functions?

int TokenCount(char *ptr) {
  // to count number of  commas in the char array (counts number of data fields present)
  char ch = ',';
  int count = 0;
  for (int i = 0; i < strlen(ptr); i++) {
    if (ptr[i] == ch) count++;
  }
  return count;
}

boolean FillToken(char *ptr) {  //
  /* the function receives a pointer to the char array where the field is to be stored.
   * It uses the global pointer pTOKEN that points to the starting of the field that we
   * want to extract. So we search for the next field delimiter "," which will be pointed
   * by p2 and we get the lenght of the field in the variable len. We copy len characters
   * to the destination char array and terminate the array with a 0 (ZERO). So if a field
   * is empty, len = 0 and ptr[0]=0. This can be used, later, to test if the field was empty.
   * Finally as p2 is pointing to a "," we increment it by 1 and copy it to pTOKEN so that
   * pTOKEN will be pointing to the starting of the next field.
   */
  char *p2 = strchr(pTOKEN, ',');
  if (p2 == NULL) { return false; }
  int len = p2 - pTOKEN;
  memcpy(ptr, pTOKEN, len);
  ptr[len] = 0;
  pTOKEN = p2 + 1;
  return true;
}

boolean FillTokenLast(char *ptr) {
  // All NMEA messages should have the checksum, with a * delimeter for the last data field. -- but some coders may forget this and just end with CR..(!)
  // This is the same basic code as the FillToken code Luis wrote, but searches for '*' and if not found, looks for a CR.
  // It is ONLY used to extract the last expected datafield from NMEA messages.  - And we counted the number of data fields using a ", count" in  TokenCount.
  // Therefore this will extract the last datafield from NMEA data messages with either  a * or a CR  delimiting the last datafield.
  // could probably be written in a more elegant way..
  char *p2 = strchr(pTOKEN, '*');
  char *p3 = strchr(pTOKEN, 0x0D);
  if ((p2 == NULL) && (p3 == NULL)) { return false; }
  if (p2 == NULL) {
    int len = p3 - pTOKEN;  // Second choice, "*" was not found/missing so just extract ALL the remaining data before the CR.
    memcpy(ptr, pTOKEN, len);
    ptr[len] = 0;
    pTOKEN = p3 + 1;           // not a lot of point in this as this was the end of the message.. ?
  } else {                     // not yet at end of message! We found a "*"
    int len = p2 - pTOKEN;     // get length from last comma to "*" character
    memcpy(ptr, pTOKEN, len);  // copy whatever there is to ptr
    ptr[len] = 0;              // place the end mark (0)
    pTOKEN = p2 + 1;           // could probably remove this as this subroutine is only called for the last data field. ?
  }
  return true;
}





extern void EventTiming(String input);  // to permit timing functions here during development

bool NeedleinHaystack(char ch1, char ch2, char ch3, char *haystack, int &compareOffset) {
  // Serial.printf("\n Looking for<%c%c%c> in strlen(%i) %s \n", ch1, ch2, ch3, strlen(haystack), haystack);
  compareOffset = 0;
  // if (needle[0] == '\0') { return false; }
  for (compareOffset = 0; (compareOffset <= strlen(haystack)); compareOffset++) {
    if ((ch1 == haystack[compareOffset]) && (ch2 == haystack[compareOffset + 1]) && (ch3 == haystack[compareOffset + 2])) {
      //    Serial.printf("Found at %i\n", compareOffset);
      return true;
    }
  }
  compareOffset = 0;
  return false;
}

//********* Add this if needed in the case statements to help sort bugs!
// Serial.println(" Fields:");
// for (int x = 0; x <= Num_DataFields; x++) {
//   Serial.print(Field[x]);
//   Serial.print(",");
// }
// Serial.println("> ");
/* ref  from TL functions..
double NMEA0183GetDouble(const char *data) {
  double val = NMEA0183DoubleNA;
  if (data == 0) return val;         // null data sets a (detectable but should have no effect) 1e-9 
  for (; *data == ' '; data++);      // Pass spaces
  if (*data != 0 && *data != ',') {  // not empty field
    val = atof(data);
  }
  return val;
}
*/
//revised 18/03 all NULL data should be set "grey"
void toNewStruct(char *field, instData &data) {
  data.greyed = true;
  data.updated = millis();
  data.lastdata = data.data;
  data.data = NMEA0183GetDouble(field);  // if we have included the TL library we do not need the function copy above
  if (data.data != NMEA0183DoubleNA) {
    data.greyed = false;
  }
  data.displayed = false;
  data.graphed = false;
}

void toNewStruct(double field, instData &data) { // allow update of struct with simple double data
  data.greyed = true;
  data.updated = millis();
  data.lastdata = data.data;
  data.data = field;
  if (data.data != NMEA0183DoubleNA) {
    data.greyed = false;
  }
  data.displayed = false;
  data.graphed = false;
}




bool processPacket(const char *buf, tBoatData &BoatData) {  // reads char array buf and places (updates) data if found in stringND
  char *p;
  int Index;
  int Num_Conversions = 0;
  Num_DataFields = TokenCount(pTOKEN);  //  TokenCount reads the number of commas in the pointed nmea_X[] buffer
  pTOKEN = pTOKEN + 1;                  // pTOKEN points advances $/!
  for (int n = 0; n <= Num_DataFields - 1; n++) {
    p = Field[n];
    if (!FillToken(Field[n])) { return false; }  // FillToken looks for "," places the data pointed to into Field[n]
  }
  p = Field[Num_DataFields];  // searches for '*' and if not found, looks for a CR
  if (!FillTokenLast(Field[Num_DataFields])) { return false; }
  //Serial.printf("  Found  <%i> Fields Field0<%s> Field1<%s> Field2<%s> Field3<%s>\n", Num_DataFields, Field[0],Field[1], Field[2], Field[3]);
  //NeedleInHaystack/4/will (should !) identify the command.  Note Nul to prevent zero ! being passed to Switch or Div4
  //                  0   1   2   3   4   5   6   7   8   9   10  11  12  13  14  15  16  17  18  19....
  char nmeafunct[] = "NUL,DBT,DPT,DBK,MWV,VHW,RMC,APB,GLL,HDG,HDM,MTW,MWD,NOP,XS,,AK,,ALK,BWC,WPL,GSV ";  // more can be added to
  // Not using Field[0] as some commands have only two characters. so we can look for (eg) 'XS,' from $IIXS, =13
  if (NeedleinHaystack(buf[3], buf[4], buf[5], nmeafunct, Index) == false) { return false; }
  // Serial.printf(" Using case %i \n", Index / 4);
  // Serial.println(" Fields:");for(int x=0 ;int <Num_DataFields;int++){Serial.print(Field[x]);Serial.print(",");} Serial.println("> ");

  switch (Index / 4) {
    case 1:  //dbt
      toNewStruct(Field[3], BoatData.WaterDepth);
      return true;
      break;
    case 2:  //DPT //dIFFERENT TO DBT/DBK
      toNewStruct(Field[1], BoatData.WaterDepth);
      return true;
      break;
    case 3:  //DBK
      toNewStruct(Field[3], BoatData.WaterDepth);
      return true;
      break;

    case 4:  //mwv
      toNewStruct(Field[1], BoatData.WindAngleApp);
      toNewStruct(Field[3], BoatData.WindSpeedK);
      // also try to compute Ground wind.. Relative to North
      toNewStruct((BoatData.Variation + DoubleInstdataAdd (BoatData.WindAngleApp,BoatData.MagHeading)),BoatData.WindAngleGround);
      

      return true;
      break;

    case 5:  //VHW
      toNewStruct(Field[5], BoatData.STW);
       // other VHW data (directions!) are usually false!
      return true;
      break;
    case 6:  //RMC
      toNewStruct(Field[7], BoatData.SOG);
      toNewStruct(Field[8], BoatData.COG);
    // nmea0183nan (-10million.. so may need extra stuff to prevent silly displays!)

      BoatData.Latitude.data = LatLonToDouble(Field[3], Field[4][0]);   // using TL's functions that return null value
      BoatData.Longitude.data = LatLonToDouble(Field[5], Field[6][0]);  //nb we use +1 on his numbering that omits the command
                                                                   //        Serial.println(BoatData.GPSTime); Serial.println(BoatData.Latitude);  Serial.println(BoatData.Longitude);  Serial.println(BoatData.SOG);
      BoatData.GPSTime = NMEA0183GPTimeToSeconds(Field[1]);
      BoatData.GPSDate = atof(Field[9]);  
      // mag variaion is [10]/[11] (E) But I think this comes from a look up and not the sattelites at least on my cheapo GPS module.

      return true;  //
      break;
    case 7:  //APA  3=xte 8 = bearing to dest (9=M agnetic or  T rue)
             //APB  3= xte 11 = CURRENT BEARING TO DEST  and 12(m/t) same..AS APA

      return true;
      break;
    case 10:  //HDM
      toNewStruct(Field[1], BoatData.MagHeading);
       return true;
      break;

    case 19:  //GSV
      // Serial.printf("\n Debug GSV ? numdatafields<%i>  ", Num_DataFields);
      //          if (Num_DataFields < 10) { return false; }
      //        Serial.println("Fields<");                  // un copy this lot to assist debugging!!
      //        for (int x = 0; x <= Num_DataFields; x++) {
      //          Serial.printf("%i=<%s>,",x,Field[x]);
      //        }
      //        Serial.println(" ");
      //  Not new struct, sats in view is just a double. not an inststruct. toNewStruct(Field[3], BoatData.SatsInView);
      BoatData.SatsInView= NMEA0183GetDouble(Field[3]);
 
      return true;
      break;

    default:
      return false;
      break;
  }


  return false;
}
// chararrayToDouble -- get char*, check it is not null then atof to the double. Use to make a safer atof().



int TopLeftYforthisLine(Button button, int printline) {
  return button.v + button.bordersize + (printline * (text_height + 2));
}


// no option for centered.. is equivalent to old CommonSubUpdateLinef
void CommonCenteredSubUpdateLine(uint16_t color, int font, Button &button, const char *msg) {
CommonCenteredSubUpdateLine(true,false,color,font, button, msg); 
}

// I think vertical centering may not be needed?? 
void CommonCenteredSubUpdateLine(bool horizCenter, bool vertCenter, uint16_t color, int font, Button &button, const char *msg) {
  int LinesOfType;
  int16_t x, y, TBx1, TBy1;
  uint16_t TBw1, TBh1;
  int typingspaceH, typingspaceW;
  int local;
  local = MasterFont;
  // can now change font inside this function
  setFont(font);
  typingspaceH = button.height - (2 * button.bordersize);
  typingspaceW = button.width - (2 * button.bordersize);
  LinesOfType = typingspaceH / (text_height + 2);  //assumes textsize 1
   // is this taken into acount in TBh1?? drops a line if it thinks it would print outside box?
  if(horizCenter ||vertCenter ){ gfx->setTextWrap(false);}
     else{gfx->setTextWrap(true);} 
  // get bounds as would be printed at top of box.. 
  // set text bounds first so that can be taken into account !   
  gfx->setTextBound(button.h + button.bordersize+1, button.v + button.bordersize+1, typingspaceW-2, typingspaceH-2);                               // 
  gfx->getTextBounds(msg, button.h + button.bordersize+1, button.v + button.bordersize+1, &TBx1, &TBy1, &TBw1, &TBh1);  // do not forget '& ! Pointer not value!!!
  // FOR debugging line wrapping: use serial input (shows as RED! ) e.g. from Arduino serial monitor
   int LinesPrinted;
  LinesPrinted=int(0.5+TBh1/text_offset);
  // if(color==RED){
  // Serial.printf("Message is <%s>\n",msg);  
  // Serial.printf(" SPACE is %i wide\n",typingspaceW) ; 
  // int LinesPrinted;
  // LinesPrinted=int(0.5+TBh1/text_offset);
  // Serial.printf(" test: tbx%i  tby%i  tbw%i  tbh%i   toff %i  Linesadded%i", TBx1, TBy1, TBw1, TBh1, text_offset,LinesPrinted );
  // }
  y = button.v + text_offset ;
  x = button.h +button.bordersize;  
  if (horizCenter) { x = x + ((typingspaceW - (TBw1 )) / 2); } 
  y= TopLeftYforthisLine(button, button.PrintLine)+text_offset +1; // // puts y cursor on a specific line 
  if (vertCenter) { y = text_offset+ button.bordersize+ button.v + ((typingspaceH - (TBh1)) / 2); }  // vertical centering
  gfx->setCursor(x, y);  
  gfx->setTextColor(color);
  gfx->print(msg);
  button.PrintLine = button.PrintLine +LinesPrinted; 
  //NOTE TEXT WRAP uses the last variable in the GFXFont setting, which should be roughly twice the character height. 
  //But often seems to be set larger,
  // which gives a text wrap of TWO lines..                               
  if (button.PrintLine >= (LinesOfType + 1)) { //clear full print area  if starting again 
    button.screenfull = true;
    if (!button.debugpause) {
      button.PrintLine = 0;
      gfx->fillRect(button.h + button.bordersize, button.v + button.bordersize, typingspaceW, typingspaceH, button.BackColor);
      button.screenfull = false;
    }
  }  

 //  button.PrintLine = button.PrintLine + 1;  //  FOR NEXT LINE (TBh / (text_height + 2)) + 1;
  gfx->setTextBound(0, 0, 480, 480);        //MUST RESET IT
  setFont(local);
}

void UpdateLinef(uint16_t color, int font, Button &button, const char *fmt, ...) {  // Types sequential lines in the button space '&' for button to store printline?
  if (button.screenfull && button.debugpause) { return; }
  //Serial.printf(" lines  TypingspaceH =%i  number of lines=%i printing line <%i>\n",typingspaceH,LinesOfType,button.PrintLine);
  static char msg[300] = { '\0' };
  va_list args;
  va_start(args, fmt);
  vsnprintf(msg, 300, fmt, args);
  va_end(args);
  int len = strlen(msg);
  CommonCenteredSubUpdateLine(false,false, color, font, button, msg);
  // was CommonSubUpdateLinef(color, font, button, msg);
}



void UpdateLinef(int font, Button &button, const char *fmt, ...) {  // Types sequential lines in the button space '&' for button to store printline?
  if (button.screenfull && button.debugpause) { return; }
  static char msg[300] = { '\0' };
  va_list args;
  va_start(args, fmt);
  vsnprintf(msg, 300, fmt, args);
  va_end(args);
  int len = strlen(msg);
  CommonCenteredSubUpdateLine(false,false, button.TextColor, font, button, msg);
  // was   CommonSubUpdateLinef(button.TextColor, font, button, msg);
}





void Sub_for_UpdateTwoSize(int magnify, bool horizCenter, bool vertCenter, bool erase, int bigfont, int smallfont, Button button, instData &data, const char *fmt, ...) {  // TWO font print. separates at decimal point Centers text in space GREYS if data is OLD
  static char msg[300] = { '\0' };
  char digits[30];
  char decimal[30];
  static char *token;
  const char delimiter[2] = ".";  // Or space, etc.
  //Serial.printf("h %i v %i TEXT %i  Background %i \n",button.h,button.v, button.TextColor,button.BackColor);
  // calculate new offsets to just center on original box - minimum redraw of blank
  int16_t x, y, TBx1, TBy1, TBx2, TBy2;
  uint16_t TBw1, TBh1, TBw2, TBh2;
  int typingspaceH, typingspaceW;
  bool recent = (data.updated >= millis() - 6000);
  //Serial.printf(" ** DEBUG 1 in NEW Update Data %f \n",data.data);
  ////// buttton width and height are for the OVERALL box.
  typingspaceH = button.height - (2 * button.bordersize);
  typingspaceW = button.width - (2 * button.bordersize);
  // SetTextsize now set in used in message buildup
  va_list args;         // extract the fmt..
  va_start(args, fmt);
  vsnprintf(msg, 300, fmt, args);
  va_end(args);
  int len = strlen(msg);
  // split msg at the decimal point .. so  must have decimal point!
  if (strcspn(msg, delimiter) != strlen(msg)) {
    token = strtok(msg, delimiter);
    strcpy(digits, token);
    token = strtok(NULL, delimiter);
    strcpy(decimal, delimiter);
    decimal[1] = 0;  // add dp to the decimal and the critical null so the strcat works !!
    strcat(decimal, token);
  } else {
    strcpy(digits, msg);
    decimal[0] = 0;
  }
  setFont(bigfont);                                              // here so the text_offset is correct for bigger font
  x = button.h;                                                  //+ button.bordersize;
  y = button.v + (magnify* text_offset);      // allow for magnify !! bigger font for front half
  gfx->getTextBounds(digits, x, y, &TBx1, &TBy1, &TBw1, &TBh1);  // do not forget '& ! Pointer not value!!!
  //Serial.printf("  *** DEBUG text_offset %i  and y%i  TBy1 %i \n",text_offset,y, TBy1); //Serial.print(digits);Serial.print(decimal);Serial.println(">");  //debugcheck for font too big - causes crashes?
  setFont(smallfont);                                                    // smaller for second part after decimal point
  gfx->getTextBounds(decimal, x + TBw1, y, &TBx2, &TBy2, &TBw2, &TBh2);  // width of smaller stuff
  gfx->setTextBound(button.h + button.bordersize, button.v + button.bordersize, typingspaceW, typingspaceH);
  if (((TBw1 + TBw2) >= typingspaceW) || ((TBh1) >= typingspaceH)) {
    // Serial.print("***DEBUG <");
    // Serial.print(msg);
    // Serial.print("> became <");
    // Serial.print(digits);
    // Serial.print(decimal);
    // Serial.println("> and was too big to print in box");
    gfx->setTextBound(0, 0, 480, 480);
    data.displayed = true;
    return;
  }
  //Serial.print("will print <");Serial.print(digits);Serial.print(decimal);Serial.println(">");  //debug
  setFont(bigfont);                                                   // here so the text_offset is correct for bigger font
  if (horizCenter) { x = x + ((typingspaceW - (TBw1 + TBw2)) / 2); }  //try for horizontal / vertical centering
  if (vertCenter) { y = y + ((typingspaceH - (TBh1)) / 2); }          // vertical centering
  y = y + button.bordersize;                                          // Down a bit equal to border size.. give a bit of border to print
  //box erase
  //gfx->fillRect(TBx2 , y - (text_offset), TBw2+TBw1 , TBh2, button.BackColor);
  //gfx->fillRect(x , y - (text_offset), (TBw1), TBh1, RED);//button.BackColor);  //RED); visualise in debug by changing colour ! where the text will be plus a bit
  if (erase) {
    gfx->setTextColor(button.BackColor);
  } else {
    gfx->setTextColor(button.TextColor);
  }
  gfx->setCursor(x, y);
  if (!recent) {
    gfx->setTextColor(DARKGREY);
    data.greyed = true;
  }
  gfx->print(digits);
  setFont(smallfont);
  gfx->print(decimal);
  gfx->setTextColor(button.TextColor);
  gfx->setTextBound(0, 0, 480, 480);  //MUST reset it !
}

void UpdateDataTwoSize(bool horizCenter, bool vertCenter, int bigfont, int smallfont, Button button, instData &data, const char *fmt) {
  if (data.data == NMEA0183DoubleNA) { return; }

  bool recent = (data.updated >= millis() - 6000);
  if (data.greyed) { return; }
  if (!data.displayed) {
    gfx->setTextSize(1);
    //  Serial.printf(" in UpdateDataTwoSize bigfont %i  smallfont %i    data %f  format %s",bigfont,smallfont,               data.data,fmt);
    Sub_for_UpdateTwoSize(1,horizCenter, vertCenter, true, bigfont, smallfont, button, data, fmt, data.lastdata);
    Sub_for_UpdateTwoSize(1,horizCenter, vertCenter, false, bigfont, smallfont, button, data, fmt, data.data);
    data.displayed = true;  //reset to false inside toNewStruct
    return;
  }
  if (!recent && !data.greyed) {
    gfx->setTextSize(1);
    Sub_for_UpdateTwoSize(1,horizCenter, vertCenter, true, bigfont, smallfont, button, data, fmt, data.lastdata);
    Sub_for_UpdateTwoSize(1,horizCenter, vertCenter, false, bigfont, smallfont, button, data, fmt, data.data);
    data.displayed = true;  //reset to false inside toNewStruct
  }
}



void UpdateDataTwoSize(int magnify, bool horizCenter, bool vertCenter, int bigfont, int smallfont, Button button, instData &data, const char *fmt) {
  if (data.data == NMEA0183DoubleNA) { return; }

  bool recent = (data.updated >= millis() - 6000);
  if (data.greyed) { return; }
  if (!data.displayed) {
    gfx->setTextSize(magnify);
    //  Serial.printf(" in UpdateDataTwoSize bigfont %i  smallfont %i    data %f  format %s",bigfont,smallfont,               data.data,fmt);
    Sub_for_UpdateTwoSize(magnify,horizCenter, vertCenter, true, bigfont, smallfont, button, data, fmt, data.lastdata);
    Sub_for_UpdateTwoSize(magnify,horizCenter, vertCenter, false, bigfont, smallfont, button, data, fmt, data.data);
    data.displayed = true;  //reset to false inside toNewStruct
    gfx->setTextSize(1);
    return;
  }
  if (!recent && !data.greyed) {
    gfx->setTextSize(magnify);
    Sub_for_UpdateTwoSize(magnify,horizCenter, vertCenter, true, bigfont, smallfont, button, data, fmt, data.lastdata);
    Sub_for_UpdateTwoSize(magnify,horizCenter, vertCenter, false, bigfont, smallfont, button, data, fmt, data.data);
    data.displayed = true;  //reset to false inside toNewStruct
    gfx->setTextSize(1);
  }
}

void GFXBorderBoxPrintf(Button button, const char *fmt, ...) {
  static char msg[300] = { '\0' };  // used in message buildup
  va_list args;
  va_start(args, fmt);
  vsnprintf(msg, 300, fmt, args);
  va_end(args);
  int len = strlen(msg);
  gfx->fillRect(button.h, button.v, button.width, button.height, button.BorderColor);  // width and height are for the OVERALL box.
  gfx->fillRect(button.h + button.bordersize, button.v + button.bordersize, 
                button.width - (2 * button.bordersize), button.height - (2 * button.bordersize), button.BackColor);
  //  int local;
  //local = MasterFont;// DO NOT USE MULTI LINE messages in GFXBorderBoxPrintf!!
  CommonCenteredSubUpdateLine(true, true, button.TextColor, MasterFont, button, msg) ;
 // WAS  WriteinBorderBox(button.h, button.v, button.width, button.height, button.bordersize, button.BackColor, button.TextColor, button.BorderColor, msg);
}

void AddTitleBorderBox(int font, Button button, const char *fmt, ...) {  // add a top left title to the box
  int Font_Before;
  //Serial.println("Font at start is %i",MasterFont);
  Font_Before = MasterFont;
  setFont(font);
  static char Title[300] = { '\0' };
  int16_t x, y, TBx1, TBy1;
  uint16_t TBw, TBh;  // used in message buildup
  va_list args;
  va_start(args, fmt);
  vsnprintf(Title, 300, fmt, args);
  va_end(args);
  int len = strlen(Title);
  gfx->getTextBounds(Title, button.h, button.v, &TBx1, &TBy1, &TBw, &TBh);
  gfx->setTextColor(WHITE, button.BorderColor);
  if ((button.v - TBh) >= 0) {
    gfx->setCursor(button.h, button.v);
  } else {
    gfx->setCursor(button.h, button.v + TBh);
  }
  gfx->print(Title);
  setFont(Font_Before);  //Serial.println("Font selected is %i",MasterFont);
}


void AddTitleInsideBox(int font, int pos, Button button, const char *fmt, ...) {  // Pos 1 2 3 4 for top right, botom right etc. add a top left title to the box
  int Font_Before;
  //Serial.println("Font at start is %i",MasterFont);
  Font_Before = MasterFont;
  setFont(font);
  static char Title[300] = { '\0' };
  int16_t x, y, TBx1, TBy1;
  uint16_t TBw, TBh;  // used in message buildup
  va_list args;
  va_start(args, fmt);
  vsnprintf(Title, 300, fmt, args);
  va_end(args);
  int len = strlen(Title);
  gfx->getTextBounds(Title, button.h, button.v, &TBx1, &TBy1, &TBw, &TBh);
  gfx->setTextColor(WHITE, button.BorderColor);
  //different positions
  //top left- just inside / outside the box- original function
  if ((button.v - TBh) >= 0) {
    gfx->setCursor(button.h, button.v);
  } else {
    gfx->setCursor(button.h, button.v + TBh);
  }
  //
  //top right
  switch (pos) {
    case 1:  // top right
      gfx->setCursor(button.h + button.width - TBw - button.bordersize, button.v + TBh + button.bordersize);
      break;
    case 2:  //bottom right
      gfx->setCursor(button.h + button.width - TBw - button.bordersize, button.v + button.height - TBh);
      break;
    case 3:  //top left
      gfx->setCursor(button.h, button.v + TBh + button.bordersize);
      break;
    case 4:  //bottom left
      gfx->setCursor(button.h, button.v + button.height - TBh);
      break;
    default:  //top right
      gfx->setCursor(button.h + button.width - TBw, button.v - TBh);
      break;
  }
  gfx->print(Title);
  setFont(Font_Before);  //Serial.println("Font selected is %i",MasterFont);
}

int Circular(int x, int min, int max) {  // returns circulated data in range min to max
  // based on compass idea in sincos: Normalize the input to the range min (0) to max (359)
  while (x < min) { x += 1 + max - min; }
  while (x > max) { x -= 1 + max - min; }
  return x;
}

int GraphRange(double data, int _TL, int _BR, double dmin, double dmax) {  // returns int (range TL to _BR) proportionate to position of data relative to dmin  / dmax
  int graphpoint;
  double input_ratio = (data - dmin) / (dmax - dmin);
  graphpoint = (_TL + (input_ratio * (_BR - _TL)));
  if (_TL < _BR) {
  } else {  // Question.. I have only needed to use this (vertical) limit function..
    if (graphpoint >= _TL) { graphpoint = _TL; }
    if (graphpoint <= _BR) { graphpoint = _BR; }
  }
  return graphpoint;
}

void PTriangleFill(Phv P1, Phv P2, Phv P3, uint16_t COLOUR) {
  gfx->fillTriangle(P1.h, P1.v, P2.h, P2.v, P3.h, P3.v, COLOUR);
}
void Pdrawline(Phv P1, Phv P2, uint16_t COLOUR) {  // simple hack for a thicker line
   int wide =1;
  gfx->drawLine(P1.h, P1.v, P2.h, P2.v, COLOUR);
  gfx->drawLine(P1.h+wide, P1.v, P2.h+wide, P2.v, COLOUR);
  gfx->drawLine(P1.h, P1.v+wide, P2.h, P2.v+wide, COLOUR);
  gfx->drawLine(P1.h+wide, P1.v+wide, P2.h+wide, P2.v+wide, COLOUR);
}



void PfillCircle(Phv P1, int rad, uint16_t COLOUR) {
  gfx->fillCircle(P1.h, P1.v, rad, COLOUR);
}

void DrawGPSPlot(bool reset, Button button, tBoatData BoatData, double magnification) {
  static double startposlat, startposlon;
  double LatD, LongD;  //deltas
  int h, v;
  if (BoatData.Latitude.data != NMEA0183DoubleNA) {
    if (reset) {
      startposlat = BoatData.Latitude.data;
      startposlon = BoatData.Longitude.data;
    }

    h = button.h + ((button.width) / 2);
    v = button.v + ((button.height) / 2);
    // magnification 1 degree is roughly 111111 m
    AddTitleInsideBox(1, 1, button, "circle:%4.1fm", float((button.height) / (2 * (magnification / 111111))));
    gfx->drawCircle(h, v, (button.height) / 2, button.BorderColor);
    AddTitleBorderBox(0, button, "Magnification:%4.1f pixel/m", float(magnification) / 111111);
    if (startposlon == 0) {
      startposlat = BoatData.Latitude.data;
      startposlon = BoatData.Longitude.data;
    }
    LongD = h + ((BoatData.Longitude.data - startposlon) * magnification);
    LatD = v - ((BoatData.Latitude.data - startposlat) * magnification);  // negative because display is top left to bottom right!
                                                                     //set limits!! ?
    gfx->fillCircle(LongD, LatD, 4, button.TextColor);
  }
}

void DrawGraph(Button button, instData &DATA, double dmin, double dmax) {
  DrawGraph(button, DATA, dmin, dmax, 8, " ", " ");
}
extern int Display_Page;
void DrawGraph(Button button, instData &DATA, double dmin, double dmax, int font, const char *msg, const char *units) {
  if (DATA.displayed) { return; }
  if (DATA.data == NMEA0183DoubleNA) { return; }
  static int Displaypage =0;      // where were we last called from ??
#define Hmax 200                // how many points fill screen width
  static Phv graph[Hmax + 2];  //phv is a point H,V structure
  static int index, lastindex;
  static bool haslooped;
  int dotsize;
  double data;
  data = DATA.data;
  dotsize = 3;
  if (Displaypage != Display_Page) {
    Displaypage = Display_Page;
    haslooped = false;
    for (int x = 0; x <= Hmax; x++) {  //set all points mid screen (so it does not rub things out!)
      graph[x].v = (button.v + button.bordersize + (button.height / 2));
      graph[x].h = (button.h + button.bordersize + (button.width / 2));
    }
  // Serial.printf(" Debug Graph..'zeroed' to mid point of  h%i  v%i",graph[1].h,graph[1].v);
  //initial fill and border (same as writeinborder box codes )
  gfx->fillRect(button.h, button.v, button.width, button.height, button.BorderColor);  // width and height are for the OVERALL box.
  gfx->fillRect(button.h + button.bordersize, button.v + button.bordersize,
                button.width - (2 * button.bordersize), button.height - (2 * button.bordersize), button.BackColor);
  index = 0;
  lastindex = 0;
  // I think this is better done explicitly when DrawGraph is called
  // AddTitleInsideBox(font,3, button, " %s", msg);
  // AddTitleInsideBox(font,1, button, " %4.0f%s", dmax,units);
  // AddTitleInsideBox(font,2, button, " %4.0f%s", dmin,units);
}
//PfillCircle(graph[lastindex], dotsize, button.BackColor); // blank the last circle (if using 'dot on end of line approach')

if (index == 0) {  // redraw text in case it gets overwritten
  AddTitleInsideBox(font, 3, button, " %s", msg);
  AddTitleInsideBox(font, 1, button, " %4.0f%s", dmax, units);
  AddTitleInsideBox(font, 2, button, " %4.0f%s", dmin, units);
  //AddTitleInsideBox(0,3, button, "Title3");
  //AddTitleInsideBox(0,4, button, "Title4");
}

bool LINE = true;
if (haslooped) {                                                              // only erase if there is something to erase!
  PfillCircle(graph[index], dotsize, button.BackColor);  // blank the  circle that will be overwritten
  //Blank line from Current dot (about to be rewritten!) to next circle but not the 'flyback'
  if (LINE && (index <= Hmax-1 )) {
   if (index >= lastindex) {Pdrawline(graph[lastindex], graph[index], button.BackColor);}
    Pdrawline(graph[index], graph[index+1], button.BackColor);
  }
}
//Serial.printf(" Graph Index %i\n", index);
//get graph point.. V is 'data' horiz is 'index' 
graph[index].v = GraphRange(data, button.v + button.height - button.bordersize - dotsize, button.v + button.bordersize + dotsize, dmin, dmax);
graph[index].h = GraphRange(index, button.h + button.bordersize + dotsize, button.h + button.width - button.bordersize - dotsize, 0, Hmax);

// draw from Last circle to the new one
if (LINE && (index >= 1)) {
  Pdrawline(graph[lastindex], graph[index], button.TextColor);
}
PfillCircle(graph[index], dotsize, button.TextColor);
lastindex = index;
index = Circular(index + 1, 0, Hmax);  //circular increment 

if (lastindex >= index) { haslooped = true; }
DATA.displayed = true;  //reset to false inside toNewStruct   it is this that helps prevent multiple repeat runs of this function, but necessitates using the local copy of you want the data twice on a page

// for (int x = 0; x <= Hmax; x++) {  //draw line from index-1 to index in text col
//                                   // Pdrawline(graph[x], graph[x + 1], button.TextColor);
// }
}

double DoubleInstdataAdd(instData &data1, instData &data){
  double temp;
  temp=0;
  if (data.data != NMEA0183DoubleNA){temp += data.data;}
  if (data1.data != NMEA0183DoubleNA){temp += data1.data;}
  return temp;
}
