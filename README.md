
# NMEADisplay
This project is a Wireless Instrument Repeater Display for Boats.

<i>STANDARD DISCLAIMER: This instrument is intended as an aid to navigation and should not be relied upon as the sole source of information. 
While every effort has been made to ensure the accuracy of message translations and their display, they are not guaranteed. 
The user is responsible for cross-checking data with other sources, exercising judgment, and maintaining situational awareness. 
No liability for any loss, damage, or injury resulting from the use of this instrument will be accepted. </i>

This project requires the boat to have a wireless NMEA Gateway that sends NMEA 0183 instrument readings on UDP. 
<p align="center"> Version 3 display <img width = 400 src="https://github.com/user-attachments/assets/a6a14548-3c6a-4396-b0af-098bd9176c43" width="200" /></p>

Images of previous versions of the display
<p> <img  height =200 src="https://github.com/user-attachments/assets/7f585734-d98d-4989-88b9-e27b94a2dbbe" />
<img height = 200 src="https://github.com/user-attachments/assets/f7ee5526-b172-4278-a29b-25652bf69c3d" /></p>


## HOW TO INSTALL FIRST TIME

Experimental ESPLaunchpad:
<a href="https://espressif.github.io/esp-launchpad/">
    <img alt="Try it with ESP Launchpad" src="https://espressif.github.io/esp-launchpad/assets/try_with_launchpad.png" width="250" height="70">
</a>



 
First, plug your module into a com port on your PC and record which port it is using. 
If confused, check Device Manager and look for the USB-SERIAL CH340 port. 
Remember the port number!
Download the Zip file of the GitHub Project. (green button -<>CODE and select 'Download Zip')
Save the file and extract all the files using a zip tool to somewhere on your PC. - I suggest /downloads.
You will then have a directory something like: C:\Users\admin\Downloads\NMEADisplay-main\
Open the directory in a file browser and navigate to the subdirectory:
C:\Users\admin\Downloads\NMEADisplay-main\NMEADisplay-main\build\esp32.esp32.esp32s3
In this folder is a batch file called ProgramBoard.bat.
Double click this bat file (see below*) and it will start and a new DOS prompt window will open and ask: 
Enter Com port number: ... Enter your com port number (eg 8) and press return. 
The Esptool program will upload the program onto your board and the display board should restart. 

You will next need to get a new microSD card, ( I used 4Gb), and format it using FAT32.
Then copy the whole of the "SdRoot" folder onto the SD card. 
You should then see it will have two sub directories: edit and logs.
The /edit subdirectory will have two files 'index.htm', and 'ace.js' which are vital for the SD Web viewer/editor!.

The root of the SD should have (at least) these files:
config.txt, ( a json file with user settings) and 
vconfig.txt ( a json with settings for the ble victron mode )
colortest.txt ( a json with settings that will eventually allow global day/night colours and also has some simulation/debug settings for the BLE part of the display)
These txt files may (should!) self initiate if not present, but its better to have defaults present! 
logo4.jpg (the new generic start screen image), 
v3small.jpg (used in the webbrowser start screen).
and loading.jpg, (a picture that appears during OTA updates). 
startsound.mp3 is bells that will play on start if you have modified the board to enable the audio.

Re insert the SD card into the module and restart. 

You should get the colour picture and white text showing the program starting up. 
If you do not see the picture, and just get the white text, the module has not recognised the SD card, and you need to try a different one. 
It can be quite fussy.  The Vitron BLE stuff will only work if you have the SD card. as that is the only place the MAC and encryption keys are stored.
 
(*) Windows may bring up a blue box saying "Windows Protected your PC", as it does not like running unrecognised batch files. 
select "More Info" and click on "Run anyway". 

## Connecting to your Wifi SSID

If you happen to have a boat with a WiFi SSID "GuestBoat" and password 12345678, you will connect instantly as this is the default.
For your SSID, Go to Settings WiFi,  Click on "set SSID", and you will be presented with a scan of available networks.

You can select a network by touching it and it will update in the second box and show (eg) Select<GUESTBOAT>?
if you touch this, it will select that SSID and return you to the main WiFi Settings page. 
Press EEPROM "UPDATE" and then "Save/Reset " to save this new SSID in the eeprom.
Settings made via the screen will be copied into the config.txt file on the SD card and may also be modified wirelessly. (see next).
Settings on the config.txt (from SD card) take priority when starting.

The Display does a scan on startup and will attempt to automatically connect IF it finds the correct SSID on startup. 
it will retry the scan once every 30 seconds if it does not find the SSID. 
This will interrupt the display, but since you are not connected .. there is nothing to display! 

## USING Webbrowser to select settings.
There are three JSON files that control operations:
The "config.txt". controls WiFi settings and major display modes, and is backed up by EEPROM, so that the SD card is not essential for basic operations.
Vconfig.txt and colortest.txt are only stored on the SD and are related mainly to Victron BLE functions.
In the config.txt you can select which 'page' is displayed after startup by changing the number: "Start_Page" 4 is the quad display and the default. 
From version 3.97, you can select what is displayed in the various 'quarters' with the JAVASCRIPT entries: 
e.g.  "FourWayBR": "SOG",
      "FourWayBL": "DEPTH",
  other options are : STW GPS SOGGRAPH STWGRAPH DGRAPH and DGRAPH2 TIME, TIMEL 
on Saving (SDsave), the settings will be implemented on the display.  

To edit any of these, use a webbrowser and open the SD Editor. Click on the file and it can be modified. 
Edit the file and save it . (On touchscreens - just press the 'SDsave' button to save once edited). But it is MUCH easier to navigate the SD Edit with a mouse!

The Display will check files on startup. If the file is not present it will use defaults. 
If you have no SD card, it will revert to using its internal EEPROM to save the WIFI settings, but Victron BLE features will not function.

### Webbrowser:
There is a web interface that can be connected to by pointing a browser at http://nmeadisplay.local/ (default)
If you change the panel name, you will need to point to the new name: eg http://panel2.local (etc).
You can also point directly to the IP address as shown on the WiFi settings page. 

The SD EDITOR uses ace javascript, (which is also saved on the SD card in /EDIT, so that internet connection should not be essential.)
It needs right mouse click to select a popup window for download etc. This 'right mouse click' does not work on touch devices.
But if you can add a mouse, you can use this functionality. Also. the scroll bars for large files refuse to show on IOS.. 
BUT if you have the mouse then the scroll wheel WILL scroll through the text content.
I have added a "SDsave" button to save any edits. This works on touch and allows edits made on screen to be saved.
You should make sure you have ace.js in the SD ( /edit/ace.js), and also (/edit/jquery.min.js) to save these having to be obtained from the internet.

From version 4.30, Editing config.txt, Vconfig.txt or colortest.tx via the editor and pressing  'SDsave' now updates the display settings.
but if you modify the Panelname, or AP / passowrd etc, you should still restart the display as you will need to point the webbrowser to the new name: (and or reconnect to the new AP) 
I.E after  I changed from NMEADISPLAY to "Panel2", I need to point the webbrowser to 'http://panel2.local/'. 
This allows use and control of more than one panel at a time.
The panelname in use is now shown at the bottom of all pages, as is the 'page' number.
You can select your preferred 'start page' in config.txt. 

Because the SD Editor can have difficulties with large files (and limited download without a mouse) I have added a direct View NMEA LOG link on the main page.
This opens the /logs/nmea.log file directly into the browser (without using SD Editor). 
This text can then be copied saved etc using normal webbrowser functionality.

## Navigating the Display touch screen

<b>there is a common 'click for settings' at the bottom of every screen that will take you directly to the list of menu functions.</b>

The module will start with the "Quad" instrument display. Touching each quadrant will select a different display page.
This is a simplified view of the original mapping.
![Screen Navigation](https://github.com/user-attachments/assets/f05d7e21-4c72-45cd-ae81-91a27ed20897)

Here is a short video tour of Version 1 of the software : https://youtube.com/shorts/24qs9CJK5vo?si=zCDUuTbXkYfHtEDB
(Version 2 and 3 hav better graphics and a modified menu, but are essentailly similar).

From version 4, the what instrurment data will be displayed in each of the 'quadrants' of the 'Quad' display can be selected from variables in the config.txt file. 

### NMEA DATA and UDP

Whilst the main way to send instrument data to the Display is via NMEA over UDP, the project also accepts NMEA data over 'ESP-NOW' from suitable multiplexers such as VELA-Naviga types: 
https://www.vela-navega.com/index.php/multiplexers

## MODULE Hardware Requirements

The code is based on the GUITRON 4.0 Inch ESP32-S3 Development Board Smart Display Capacitive Touch Screen LCD.
There are sometimes two versions. This code is for the version with an ST7701 driver.
It can be purchased with or without the Relays or the backing plate used for home automation applications.
The code should be compatible with other development boards that use the ST7701 driver. But be warned, you will need to determine the ESP32 pin configuration as it will differ from the Guitron version. The Esp32_4inch.h file contains the correct pin allocations for the Guitron module. There is an UNTESTED pin file for the MakerFabs ESP32S3 board in the /documents directory. 
The Code WILL accept serial NMEA0183 over USB - but is set default to 115200. It sends out debugging data on serial (at 115200 baud) if you have it connected to a PC with a suitable terminal program.   


## CODE FEATURES:
 These notes are mainly if you wish to modify the c++ code.

### FONT NOTES

The display function will generally wrap text if too large (eg in the debug). But the text advance is defined by the third parameter in the GFXfont variable.
In the Mono fonts this is generally set at twice the character height, so the 'wrapped' line prints immediately under the first.
However, in fonts generated by eg(https://rop.nl/truetype2gfx/) the text advance may be greater than twice the height, resulting in 'wrapped' text that prints TWO lines down. 
(V3.7)I have manually modified my fonts and modified the CommonSub_UpdateLine function to try and make wrapped text wrap only ONE line down. 

for reference, on this device: the main fonts and heights are: 
"FONTS/FreeSansBold6pt7b.h"    //font 7   9 pixels high
"FONTS/FreeSansBold8pt7b.h"    //font 8   11 pixels high 
"FONTS/FreeSansBold12pt7b.h"   //font 9   18 pixels high
"FONTS/FreeSansBold18pt7b.h"   //font 10  27 pixels 
"FONTS/FreeSansBold27pt7b.h"   //font 11  39 pixels
"FONTS/FreeSansBold40pt7b.h"   //font 12  59 pixels 
"FONTS/FreeSansBold60pt7b.h"   //font 13  88 pixels 

Void Display(); 

The "DISPLAY" function in the main ino is a (huge) 'switch' function that defines what happens on each page of the display and what will happen if buttons on that page are pressed. 
There is a 'button' structure that defines the size and shape and colours of all boxes on the screen. 

In simplified form: 
A GFXBorderBoxPrintf(_sButton button, const char* fmt, ...) function will print any data in center of the defined 'button'.
eg: GFXBorderBoxPrintf(BigSingleTopLeft, BoatData.SOG, "%2.1fkt"); will print the Button box "BigSingleTopLeft" with borders and the SOG with one decimal place.

To speed up screen updates I use a second function to just update the text and not redraw the box and borders:
UpdateDataTwoSize(11, 10, BigSingleTopLeft, BoatData.SOG, "%2.1fkt"); 
This displays the SOG (in BigSingleTopLeft), using two font sizes (here 11 and 10) with the smaller font used after the decimal point.

Touch screen 'actions' are controlled by a boolean CheckButton(), and an example would be:  
if (CheckButton(BigSingleTopLeft)) { Display_Page = 8; }

## SD CARD

If a SD card is fitted, the display can show Jpg images, and uses one on startup.
It can then store a 'log' file with instrument data. there are options to turn on LOG files (selected instrument readings at regular intervals)
or NMEA Logs (a record of every NMEA message and Victron BLE message recieved).

The LOG file has a filename of 'date" eg 180325.log where the date is as received from a GPS RMC message, and it is dependant on seeing a valid RMC (GPS) message to get the initial date and time. 
It only works if a RMC message with good data has been seen. 

The NMEA log can get huge rapidly and is really only for debugging and fault finding. I would recommend using it in conjunction with a connected PC, and opening config.txt and setting nmea to "true", press SDSAVE.
wait a bit and then set NMEA = "false", and again press SDSAVE. 

### Webrowser Graphics


![wEBBROWSER](https://github.com/user-attachments/assets/d0582791-e483-49ed-8e1f-06d8a2bc3a83)

This gives remote access to a SD File access page and also to a page to allow OTA of any binary updates to the code.
NOTE it uses keyboard and mouse actions that are not available on Touchscreen (tablets / phones). So this editor interface is best used with a PC

![wEBBROWSER INDEX](https://github.com/user-attachments/assets/686fd86d-3a69-4338-9ca3-57efec9dd92b)
ThIS SD file access allows wireless viewing and modification of the log and other files on the SD.

OTA allows update by downloading an updated NMEADisplay Binary. 

# Victron BLE device display

From version 4, I have been adding the ability to switch the display to show Victron BLE data from suitable Victron devices.
![victrondisplay](https://github.com/user-attachments/assets/a0685f92-06f6-4189-8c27-e6e044bc54d0)
This uses regular, repeated, BLE scanning for Victron devices and a subsequent screen display of parameters obtained. 
This is immensely powerful, but interrupts the main wifi for one second every time it scans for BLE devices.
It cannot therefore be used simultaneously with the standard NMEA display routines. 
The Victron display is accessible via "experimental". I would emphasize that this feature/capability is experimental!

There are two critical files for the display are vconfig.txt. (which has the Victron device Mac, key and names), and colortest.txt which has some settings to allow simulation of victron devices and other inputs useful during development.
The display page uses a jpg (vicback.jpg) as a background for a more colourful display.
The code recognises only Solar chargers , SmartShunt and ( tested and found to be very faulty! ) IP65 AC chargers.
The graphical format of the display is defined in VICTRONBLE.cpp and (vconfig.txt).
Each device has selected data displayed in a box with selectable position and height (but with fixed fonts and width).

There is a Simulate option, selectable in colortest, this initiates a crude simulation, suitable for assisting in code development.
This works most logically with my default Vconfig.txt file. 
I have seen some issues with Simulate crashing the display very badly:(needed reinstall of code). But I think this has been fixed in V4.30 

## AUDIO 

An audio player has been added for experimental use, that plays MP3 from files stored on the SD card /music directory.
It requires that three resistors are moved on the PCB to activate the audio circuit, and is not recommended unless you are comfortable with soldering and desoldering SMD resistors!
The audio works, but is mainly used to explore how much - if any - free computation the ESP32 has. When playing and also logging data, the audio is noticably disrupted.
I have semi hidden the audio player in V2 of the software. 
The audio will drop out when extensive computation is occuring. Such as when the graphics are updated. - or when the BLE is scanning. 
Giving a good audible indication useful for development debugging. 
If you ty to listen to an audio file while the BLE is in operation, all sound stops for a second at about 3 second intervals.


=======

# COMPILE NOTES

Compiled and tested with ESp32 V2.0.17 
GFX library for Arduino 1.5.5
Select "ESP32-S3 DEV Module"
Select PSRAM "OPI PSRAM"

It will not work with Version3 of the ESP32 compiler.

Add NMEA0183 and NMEA2000 libraries from https://github.com/ttlappalainen
I have used some of the Timo Lappinen NMEA01983 conversion functions. so null data returns as NMEA0183DoubleNA (==1e-9)

Based on concepts from AndreasSzep WiFi, https://github.com/AndrasSzep/NMEA0183-WiFi
Note: I have tried to avoid use of "Strings" as used in the original Keyboard and Andreas Szep concepts, and modified the codes to use only strings as char arrays. 

The GFX is based on GFX Library for Arduino and I am using Version 1.5.5 :
Getting the ST7701 display to work correctly was part of the initial reason for my "cheap display" Github.
the author of the GFX code sometimes updates the drivers, so if you use a different driver you may need to make small modifications:
I use: 
 st7701_type9_init_operations,  sizeof(st7701_type9_init_operations));
WRITE_COMMAND_8, 0x20, // 0x20 normal, 0x21 IPS (INVERTS the COLOURS)
Note that although the board is definitely wired 565, it seems to need the GFX to be set to RGB666 to allow the FULL range of colours in jpg to be correctly interpreted. 
WRITE_C8_D8, 0x3A, 0x60, // 0x70 RGB888, 0x60 RGB666, 0x50 RGB565

I started the Victron decode with the https://github.com/hoberman/Victron_BLE_Advertising_example, 
but later moved to using code snippets from   https://github.com/Fabian-Schmidt/esphome-victron_ble  
Other useful sources are noted in the victron cpp and .h.

At time of uploading, I am aware that my IP65 single output AC charger does not appear to have a data format as published. I plan to work on this!
Also, I have a 300A smartshunt, that presents with a product ID of 49208, a value not in any published tables I have seen.
Note that some devices will not have all the instrumentation available for viewing. Eg the battery monitor can only display voltages and not state of charge or current. 

# OTHER NOTES 

The main 'User' Settings for the display are kept in an Single structure that starts with a "KEY". Changing this key in the compiler will force the code to load the "Default" 
Also, should the EEPROM value of this 'KEY' be misread, then the 'default' will be set. _ this is intended to allow the first time run to set defaults.

Use of SD card:
Apparently, File Names longer than 8 characters may be truncated by the SD library, so keep filenames shorter.
I have noted particular issues when uploading  jquery.min.js  as this truncated to jquery.m.js  - but editing the name in the Filename box was succesul and it was stored on the SD with the full name.




