/*
N2KdataRX.h

Copyright (c) 2015-2018 Timo Lappalainen, Kave Oy, www.kave.fi


Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to use,
copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the
Software, and to permit persons to whom the Software is furnished to do so,
subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF
CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE
OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include <NMEA0183.h>
#include <NMEA2000.h>
#include <N2kMsg.h>
#include <NMEA2000.h>
#include <N2kMessages.h>

//------------------------------------------------------------------------------
/*
 Despite the name,  NOTE FOR the MULTI DISPLAY. We do not convert N2K to 0183, but just get values and place in boatData! 
*/


  
  void HandleHeading(const tN2kMsg &N2kMsg);            // 127250
  void HandleVariation(const tN2kMsg &N2kMsg);          // 127258
  void HandleBoatSpeed(const tN2kMsg &N2kMsg);          // 128259
  void HandleDepth(const tN2kMsg &N2kMsg);              // 128267
  void HandlePosition(const tN2kMsg &N2kMsg);           // 129025

  void HandleCOGSOG(const tN2kMsg &N2kMsg);             // 129026
  void HandleGNSS(const tN2kMsg &N2kMsg);               // 129029
  void HandleGNSSSystemTime(const tN2kMsg &N2kMsg);     // 126992 
  void HandleWind(const tN2kMsg &N2kMsg);               // 130306
  void HandleRudder(const tN2kMsg &N2kMsg);             // 127245
  void HandleWatertemp12(const tN2kMsg &N2kMsg);        //130312
  void HandleWatertemp16(const tN2kMsg &N2kMsg);        //130316
// experimental functions
  void HandleMFRData(const tN2kMsg &N2kMsg) ;           //126996 experimental to serial print  mfr data
  void RequestProductInformation(uint8_t destination) ; // Use 0xFF for broadcast or specific address




  String PGNDecode(int PGN);                              // decode the PGN to a readable name.. Useful for the decodeMode the bus?
