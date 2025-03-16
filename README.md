# NMEADisplay
![Sdisplay](https://github.com/user-attachments/assets/e3a0ba0e-b552-46d3-bceb-dcc11c7a620e)
A Wireless NMEA display
Short video demo : https://youtube.com/shorts/24qs9CJK5vo?si=zCDUuTbXkYfHtEDB
Compiled and tested with ESp32 V2.0.11 
GFX library for Arduino 1.5.5
Select "ESP32-S3 DEV Module"
Select PSRAM "OPI PSRAM"

V1.0 includes OTA via the super simple interface- UserId and password admin
Check the IP address via the wifi screen and connect to that IP to get the OTA started. 


Project to use a CHEAP 4" square ESP32 Lcd module to display wirelessly available NMEA data. 

Using some concepts from AndreasSzep WiFi, https://github.com/AndrasSzep/NMEA0183-WiFi
But mainly results of tests I did with https://github.com/dagnall53/Cheap_ESP32_4-display

I have tried to avoid use of "Strings" as used in the original Keyboard and Andreas Szep concepts, and modified the codes to use only strings as char arrays. 

My "Cheap_ESP32" tests included tests of alternate fonts and demonstrated how to access Jpeg and Audio mp3 files from the SD card. - Mainly just to see what was possible. 
This NMEA display will start with these elements retained, but I will probably aim to make them compile defines.

I will use atoi() and atof() to convert to double where needed.
I have used some of the Timo Lappinen NMEA01983 conversion functions. 

The GFX is based on GFX Library for Arduino and I am using Version 1.5.4 : Getting the ST7701 display to work correctly was part of the initial reaspon for the "cheap display" Github.
I plan to keep the relevant - functional -  cpp and h in /documents with modified :
 st7701_type9_init_operations,  sizeof(st7701_type9_init_operations));
Note: Use Type1 modified 
WRITE_COMMAND_8, 0x20, // 0x20 normal, 0x21 IPS
WRITE_C8_D8, 0x3A, 0x50, // 0x70 RGB888, 0x60 RGB666, 0x50 RGB565
 
- as it seems to change occasionally!

I have built a number of GFX box print functions and button touch features into this project to try (?) and simplify the design of the user interface. 
There is a lot of duplication in these at the start. I may try to tidy up later.
The same goes for fonts. I have lots included while trying to get a good looking display. 

'User' Settings for the display are kept in an Single structure that starts with a "KEY". Changing this key in the compiler will force the code to load the "Default" 
Also, should the EEPROM value of this 'KEY' be misread, then the 'default' will be set. _ this is intended to allow the first time run to set defaults.

Use of SD card:
Apparently, File Names longer than 8 charecters will be truncated by the SD library, so keep filenames shorter 
Ver 1.2 I have moved the music to a subdirectory in anticipation of adding SD logging

