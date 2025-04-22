
# NMEADisplay
This project is a Wireless Instrument Repeater Display for Boats.

It requires the boat to have a wireless NMEA Gateway that sends NMEA 0183 instrument readings on UDP. 
<p align="center"> Version 3 display <img width = 400 src="https://github.com/user-attachments/assets/a6a14548-3c6a-4396-b0af-098bd9176c43" width="200" /></p>

Images of previous versions of the display
<p> <img  height =200 src="https://github.com/user-attachments/assets/7f585734-d98d-4989-88b9-e27b94a2dbbe" />
<img height = 200 src="https://github.com/user-attachments/assets/f7ee5526-b172-4278-a29b-25652bf69c3d" /></p>


## HOW TO INSTALL FIRST TIME
 
First, plug your module into a com port on your PC and record which port it is using. 
If confused, check device Manager and look for the USB-SERIAL CH340 port. 
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
The edit subdirectory will have one file 'index.htm', which is vital for the Webeditor!.
The root of the SD should have (at least) these files:
config.txt, ( a json file with user settings) and logo3.jpg (the start screen image), v3small.jpg (used in the webbrowser start screen). 
and an 'edit' sub directory with the file 'index.html'. Other files may be included.. but are not essential.

Re insert the SD card into the module and restart. 

You should get the colour picture and white text showing the program starting up. 
If you do not see the picture, and just get the white text, the module has not recognised the SD card, and you need to try a different one. 
It can be quite fussy.
 
(*) Windows may bring up a blue box saying "Windows Protected your PC", as it does not like running unrecognised batch files. 
select "More Info" and click on "Run anyway". 

## Connecting to your Wifi SSID

If you happen to have a boat with a wifi SSID "GuestBoat" and password 12345678, you will connect instantly as this is the default.
For your SSID, Go to Settings WiFi,  Click on "set SSID", and you will be presented with a scan of available networks.

You can select a network by touching it and it will update in the second box and show (eg) Select<GUESTBOAT>?
if you touch this, it will select that SSID and return you to the main WiFi Settings page. Press EEPROM "UPDATE" and then "Save/Reset " to save this new SSID in the eeprom.
Settings made via the screen will be copied into the config.txt file on the SD card and may also be modified wirelessly. (see next).
Settings on the config.txt take priority when starting.

The Display does a scan on startup and will attempt to automatically connect IF it finds the correct SSID on startup. 
it will retry the scan once every 30 seconds if it does not find the SSID. This will interrupt the display, but since you are not connected .. there is nothing to display! 

## USING Webbrowser to select settings.
Use the Webbrowser (see later) or put the SD card in a PC and open (double click on) the file "config.txt".
This is a JSON file with settings for wifi etc.
Edit the file and save it . (On touchscreens - just press SAVE to save once edited).
The Display will check this file on startup and use these settings. If the file is not present it will use defaults. 
If you have no SD card, it will revert to using its internal EEPROM to save the WIFI settings. 
You can select which 'page' displayed after startup by changing the number "Start_Page" 4 is the quad display and the default. 

### Using webbrowser settings: VERSION 3 Update / corrections

Prior to V3, it was not possible to easily access OTA via the webbrowser without the correctly formatted SD card. This has been corrected.
The Config.txt SD file also now includes options to modify the Panelname, start log picture and AP password. 
<b>IF</b> you change to a new panel name, then you will need to point the webbrowser to the new name: I.E after  I changed from NMEADISPLAy to "Panel2", I need to point the webbrowser to 'http://panel2.local/' This allows use and control of more than one panel at a time.
The panelname in use is shown at the bottom of the settings page.

###  EDITOR ISSUE on IOS

The SD editor uses right mouse click to select file download /delete etc. Which can be useful to download the LOGS from the SD card wirelessly. Unfortunately, IOS has not got a useful sensible file save capability, so its not possible on IOS to download.
I will try later to add an alternate "send email" with th log file.  

## Navigating the Menu

The module will start with the "Quad" instrument display. Touching each quadrant will select a different display page.
This is a simplified view of the page map.
![Screen Navigation](https://github.com/user-attachments/assets/f05d7e21-4c72-45cd-ae81-91a27ed20897)

<b>Version 3 now has a common 'click for settings' at the bottom of every screen that will take you directly to the list of menu functions. </b>

Here is a short video tour of Version 1 of the software : https://youtube.com/shorts/24qs9CJK5vo?si=zCDUuTbXkYfHtEDB
(Version 2 and 3 hav better graphics and a modified menu, but are essentailly similar)

# NMEA DATA and UDP

Whilst the main way to send instrument data to the Display is via NMEA over UDP, the project also accepts NMEA data over 'ESP-NOW' from suitable multiplexers such as VELA-Naviga types: 
https://www.vela-navega.com/index.php/multiplexers

## MODULE Requirements

The code is based on the GUITRON 4.0 Inch ESP32-S3 Development Board Smart Display Capacitive Touch Screen LCD.
There are sometimes two versions. This code is for the version with an ST7701 driver.
It can be purchased with or without the Relays or the backing plate used for home automation applications.
The code should be compatible with other development boards that use the ST7701 driver. But be warned, you will need to determine the ESP32 pin configuration as it will differ from the Guitron version. The Esp32_4inch.h file contains the correct pin allocations for the Guitron module. There is an UNTESTED pin file for the MakerFabs ESP32S3 board in the /documents directory.   

## FONT NOTES

The display function will generally wrap text if too large (eg in the debug). But the text advance is defined by the third parameter in the GFXfont variable.
In the Mono fonts this is generally set at twice the character height, so the 'wrapped' line prints immediately under the first.
However, in fonts generated by eg(https://rop.nl/truetype2gfx/) the text advance may be greater than twice the height, resulting in 'wrapped' text that prints TWO lines down. 
(V3.7)I have manually modified my fonts and modified the CommonCenteredSubUpdateLine function to try and make wrapped text wrap only ONE line down. 


## CODE FEATURES:

The "DISPLAY" function in the main ino is a (huge) 'switch' function that defines what happens on each page of the display and what will happen if buttons on that page are pressed. 
There is a 'button' structure that defines the size and shape and colours of all boxes on the screen. 

A GFXBorderBoxPrintf(Button button, const char* fmt, ...) function will print any data in center of the defined 'button'.

eg: GFXBorderBoxPrintf(BigSingleTopLeft, BoatData.SOG, "%2.1fkt"); will print the Button box "BigSingleTopLeft" with borders and the SOG with one decimal place.

To speed up screen updates I use a second function to just update the text and not redraw the box and borders:

UpdateDataTwoSize(11, 10, BigSingleTopLeft, BoatData.SOG, "%2.1fkt"); 

This displays the SOG (in BigSingleTopLeft), using two font sizes (here 11 and 10) with the smaller font used after the decimal point.

Touch screen 'actions' are controlled by a boolean CheckButton(), and an example would be:  
if (CheckButton(BigSingleTopLeft)) { Display_Page = 8; }

## SD CARD

If a SD card is fitted, the display can show Jpg images, and uses one on startup.
It can then store a 'log' file with instrument data.
This log file has a filename of 'date" eg 180325.log where the date is as received from a GPS RMC message It only works if a RMC message has been seen. 

## WEB INTERFACE
There is a web interface that can be connected to by pointing a browser at http://nmeadisplay.local/ (default)
If you change the panel name, you will need to point to the new name: eg http://panel2.local (etc).
You can also point directly to the IP address as shown on the WiFi settings page. 
![wEBBROWSER](https://github.com/user-attachments/assets/d0582791-e483-49ed-8e1f-06d8a2bc3a83)

This gives remote access to a SD File access page and also to a page to allow OTA of any binary updates to the code.
NOTE it uses keyboard and mouse actions that are not available on Touchscreen (tablets / phones). So this editor interface is best used with a PC

![wEBBROWSER INDEX](https://github.com/user-attachments/assets/686fd86d-3a69-4338-9ca3-57efec9dd92b)
ThIS SD file access allows wireless viewing and modification of the log and other files on the SD.

OTA allows update by downloading an updated NMEADisplay Binary. 

# AUDIO 

In Version 1 and 2 an audio player has been added for experimental use, that plays MP3 from files stored on the SD card /music directory.
It requires that three resistors are moved on the PCB to activate the audio circuit, and is not recommended unless you are comfortable with soldering and desoldering SMD resistors!
The audion works, but is mainly used to explore how much - if any - free computation the ESP32 has. When playing and also logging data, the audio is noticably disrupted.
I have semi hidden the audio player in V2 of the software.
The audio will drop out when extensive computation is occuring. Such as when the graphics are updated. 

=======
# NOTES

Compiled and tested with ESp32 V2.0.11 
GFX library for Arduino 1.5.5
Select "ESP32-S3 DEV Module"
Select PSRAM "OPI PSRAM"

Based on concepts from AndreasSzep WiFi, https://github.com/AndrasSzep/NMEA0183-WiFi
Note: I have tried to avoid use of "Strings" as used in the original Keyboard and Andreas Szep concepts, and modified the codes to use only strings as char arrays. 

I have used some of the Timo Lappinen NMEA01983 conversion functions. so null data returns as NMEA0183DoubleNA (==1e-9)

The GFX is based on GFX Library for Arduino and I am using Version 1.5.5 :
Getting the ST7701 display to work correctly was part of the initial reason for my "cheap display" Github.
I may keep the relevant - functional - cpp and h in /documents with modified code in the in st7701_type9_init_operations section, but this may be updated in later versions?:
 st7701_type9_init_operations,  sizeof(st7701_type9_init_operations));
WRITE_COMMAND_8, 0x20, // 0x20 normal, 0x21 IPS (INVERTS the COLOURS)
Note that although the board is definitely wired 565, it seems to need the GFX to be set to RGB666 to allow the FULL range of colours in jpg to be correctly interpreted. 
WRITE_C8_D8, 0x3A, 0x60, // 0x70 RGB888, 0x60 RGB666, 0x50 RGB565

# OTHER NOTES 

'User' Settings for the display are kept in an Single structure that starts with a "KEY". Changing this key in the compiler will force the code to load the "Default" 
Also, should the EEPROM value of this 'KEY' be misread, then the 'default' will be set. _ this is intended to allow the first time run to set defaults.

Use of SD card:
Apparently, File Names longer than 8 charecters will be truncated by the SD library, so keep filenames shorter 
Ver 1.2 I have moved the music to a subdirectory in anticipation of adding SD logging.




