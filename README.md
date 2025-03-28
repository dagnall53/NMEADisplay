# NMEADisplay

![Sdisplay](https://github.com/user-attachments/assets/e3a0ba0e-b552-46d3-bceb-dcc11c7a620e)

This project is a Wireless Display for Boats
It requires the boat to have a wireless NMEA multiplexer that repeats NMEA instrumentation on UDP.
But it also accepts NMEA data over ESP-NOW from suitable multiplexers such as VELA-Naviga types: 
https://www.vela-navega.com/index.php/multiplexers

# HOW TO INSTALL FIRST TIME
 
First, plug your module into a com port on your PC and record which port it is using. 
If confused, check device Manager and look for the USB-SERIAL CH340 port. 
Remember the port number!
Download the Zip file of the GitHub Project. (green button -<>CODE and select 'Download Zip')
save the file and extract all the files using a zip tool to somewhere on your PC. - I suggest /downloads.
You will then have a directory something like: C:\Users\admin\Downloads\NMEADisplay-main\
Open the directory in a file browser and navigate to the subdirectory:
C:\Users\admin\Downloads\NMEADisplay-main\NMEADisplay-main\build\esp32.esp32.esp32s3
In this folder is a batch file called ProgramBoard.bat.
Double click this bat file (see below*) and it will start and a new DOS prompt window will open and ask: 
Enter Com port number: ... enter your com port number (eg 8) and press return. 
The Esptool program will upload the program onto your board and it should restart. 
You will next need to get a new microSD card, ( I used 4Gb), and format it using FAT32.
Then copy the whole of the "SdRoot" folder onto the SD card. 
You should then see it will have two sub directories: edit and logs. (If it does not have the logs sub directory, then add it!);
The edit subdirectory wil have one file 'index.htm', which is vital for the Webeditor!.
The root of the SD should have three files, logo.jpg (the start screen image), startws.htm, (a file that runs the webserver), and StartSound.mp3. ( a file that will play on modified boards if they have sound).
Re insert the SD card into the module and restart and you should get the colour picture of a clipper ship and White text showing the program starting up. 
If you do not see the picture, and just get the White text, the module has not recognised the SD card, and you need to try a different one. 
It can be quite fussy. 
(*) Windows may bring up a blue box saying "Windows Protected your PC", as it does not like running unrecognised batch files. 
select "More Info" and click on "Run anyway". 

# Navigating the Menu

The module will start with the "Quad" instrument display. Touching each quadrant will select a different display page.








Short video demo here : 

https://youtube.com/shorts/24qs9CJK5vo?si=zCDUuTbXkYfHtEDB


#FEATURES:

Multiple display options: Main display is a 'quad' with Wind, STW, DEPTH and SOG.
separate additional pages display each of these variables, and can also display a graph of previous values.
Code is (relatively!) simple to modify to create new pages for the display.

The code includes a WIFI setup that allows keyboard or scan selection of SSID and also keyboard setting of password and UDP port.
If a SD card is fitted, the display can show Jpg images, and uses one on startup.
It can then store a 'log' file with instrument data.
This log file has a filename of 'date" eg 180325.log where the date is as received from a GPS RMC message It only works if a RMC message has been seen. 
There is a web interface, that can be connected to by pointing at http://nmeadisplay.local/
This gives remote access to a SD File access page and also to a page to allow OTA of any binary updates to the code.
The SD file access allows viewing and modification of the log and other files on the SD.

An audio player has been added for experimental use, that plays MP3 from files stored on the SD card /music directory.
This works, but is mainly to explore how much if any free computation the ESP32 has. The audio will drop out when extensive computation is occuring. Such as when the graphics are updated. 

=======
#DISPLAY 
The initial work to simply get the display working was done in https://github.com/dagnall53/Cheap_ESP32_4-display
I believe this code should work with Makerspace 4" displays, but it will definitely need the correct GFX settings. 
My code is running on a GUITRON 4" esp32 module that is sold as a 58mm square unit with inbuilt mains relay.
This has an audio chip that i use, but as delivered, the chip is NOT connected, and the audio IO pins are used to switch the mains relays. 
There are three small resistors on the PCB that need to be moved to unlink the IO from the mains relay drives and connect to the AUDIO chip.


This code is entirely experimental and I have used it to explore C++ functionalities. 
It uses Structures and .h and .cpp sub files in an attempt to make the code more accessible, but I had to use 'extern' definitions in places.
I have tried to use a limited set of functions to display stuff on the screen, and would have liked to have more nested functions, but I have not yet found a way to nest functions that use the ,fmt ....); ending and to pass the (fmt,... ) data to what I would have liked to be a common sub program.  


Compiled and tested with ESp32 V2.0.11 
GFX library for Arduino 1.5.5
Select "ESP32-S3 DEV Module"
Select PSRAM "OPI PSRAM"



Based on concepts from AndreasSzep WiFi, https://github.com/AndrasSzep/NMEA0183-WiFi
Note: I have tried to avoid use of "Strings" as used in the original Keyboard and Andreas Szep concepts, and modified the codes to use only strings as char arrays. 

My "Cheap_ESP32" tests included tests of alternate fonts and demonstrated how to access Jpeg and Audio mp3 files from the SD card. - Mainly just to see what was possible. 
This NMEA display will start with these elements retained, but I will probably aim to make them compile defines.

I have used some of the Timo Lappinen NMEA01983 conversion functions. so null data returns as NMEA0183DoubleNA (==1e-9)

The GFX is based on GFX Library for Arduino and I am using Version 1.5.4 : Getting the ST7701 display to work correctly was part of the initial reaspon for the "cheap display" Github.
I plan to keep the relevant - functional -  cpp and h in /documents with modified :
 st7701_type9_init_operations,  sizeof(st7701_type9_init_operations));

WRITE_COMMAND_8, 0x20, // 0x20 normal, 0x21 IPS (INVERTS the COLOURS)
Although the board is definitely wired 565, it seems to need the GFX to be set to RGB66 to allow the FULL range of colours in jpg to be correctly interpreted. 
WRITE_C8_D8, 0x3A, 0x60, // 0x70 RGB888, 0x60 RGB666, 0x50 RGB565

OTHER NOTES 

I have built a number of GFX box print functions and button touch features into this project to try (?) and simplify the design of the user interface. 
There is a lot of duplication in these at the start. I may try to tidy up later.
The same goes for fonts. I have lots included while trying to get a good looking display. 

'User' Settings for the display are kept in an Single structure that starts with a "KEY". Changing this key in the compiler will force the code to load the "Default" 
Also, should the EEPROM value of this 'KEY' be misread, then the 'default' will be set. _ this is intended to allow the first time run to set defaults.

Use of SD card:
Apparently, File Names longer than 8 charecters will be truncated by the SD library, so keep filenames shorter 
Ver 1.2 I have moved the music to a subdirectory in anticipation of adding SD logging

SPIFFS :
I hope to add SPIFFS so that SD is not mandatory.
SPIFFs with Arduino IDE2.x is problematic bu this seems to help: https://electronicstree.com/esp8266-spiffs-uploads-with-arduino-ide-2-x/

Upload BINARY without Arduino!
https://docs.espressif.com/projects/esp-test-tools/en/latest/esp32/production_stage/tools/flash_download_tool.html



