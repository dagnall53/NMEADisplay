# NMEADisplay
A Wireless NMEA display

Project to use a CHEAP 4" square ESP32 Lcd module to display wirelessly available NMEA data. 

Using some concepts from AndreasSzep WiFi, https://github.com/AndrasSzep/NMEA0183-WiFi
But mainly results of tests I did with https://github.com/dagnall53/Cheap_ESP32_4-display

I have tried to avoid use of "Strings" as used in the original Keyboard and Andreas Szep concepts, and modified the codes to use only strings as char arrays. 

My "Cheap_ESP32" tests included tests of alternate fonts and demonstrated how to access Jpeg and Audio mp3 files from the SD card. - Mainly just to see what was possible. 
This NMEA display will start with these elements retained, but I will probably aim to make them compile defines.

From Andreas Szep I have appopriated the idea of having a String  boatdata (sBoatData) structure using character arrays. 
This is sensible here, as the NMEA data is read and stored as Char Arrays, and I am (mostly) not planning on doing any arithmetic with the data. 
I will use atoi() and atof() to convert to double where needed.
I have used some of the Timo Lappinen NMEA01983 conversion functions. 

The GFX is based on GFX Library for Arduino and I am using Version 1.5.4 : Getting the ST7701 display to work correctly was part of the initial reaspon for the "cheap display" Github.

I have built a number of GFX box print functions and button touch features into this project to try (?) and simplify the design of the user interface. 
There is a lot of duplication in these at the start. I may try to tidy up later.
The same goes for fonts. I have lots included while trying to get a good looking display. 

'User' Settings for the display are kept in an Single structure that starts with a "KEY". Changing this key in the compiler will force the code to load the "Default" 
Also, should the EEPROM value of this 'KEY' be misread, then the 'default' will be set. _ this is intended to allow the first time run to set defaults.
 
