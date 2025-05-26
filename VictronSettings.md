# VICTRON BLE MODE

This mode is sufficiently different from the settings for the NMEA MFD mode, that it is worth having a separate file:

The configuration for the Victron display is entirely controlled by JSON files in the SD.

Editing of these files is best done on a PC with a mouse. Trying to do this on a phone with touchscreen is very tedious.

The Encryption key and Mac for each Victron device can be obtained and copied from the Victron app: 
(details on how to do this to be expanded)


### Files are:
vconfig.txt : defines the mac address and encryption key for all the users victron BLE devices, plus: defines what data will be displayed and where:
example:  
"device0.mac": "d044984433d2",
  "device0.key": "20bd18fc6ed74d9b6e40c83817d42fc8",
  "device0.comment": "Instruments",     (will be shown as a title to the display box)
  "device0.VICTRON_BLE_RECORD_TYPE": 1, (selects if the device is Solar control, battery monitor, AC charger etc)
  "device0.DisplayH": 160,               (H position of display box on screen)
  "device0.DisplayV": 160,               (Vertical position (down increases) on screen )
  "device0.DisplayHeight": 90,           (Height of display box) 
  "device0.DisplayShow": "VL",           (What will be shown VL = Volts and Load Current) 

A second file : colortest.txt started life as a test to see if I could add daylight / night colours..(I have not succeeded yet).
It also holds some variables for testing and setting the Victron display boxes: The colour selections here work.
 "TextColor": 0,
  "BackColor": 65535,
  "BorderColor": 31,
  "BoxW": 150, (common width of all victron display boxes )
  
  Plus three variables that are used for demonstrations and debugging:
  "Simulate": "false",                 switches on a Simulate mode that simulates data reception. This only works for Victron data, but I may add internal simulation of NMEA data later.
  "Debug": "false",                    gives debug data that is saved to the NMEA log file if it is on.
  "ShowRawDecryptedDataFor": 35,       Gives detailed data on data recieved for numbered device (used to help decrypt data) 


