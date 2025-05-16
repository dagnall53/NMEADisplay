/*
  from https://github.com/hoberman/Victron_BLE_Advertising_example

  Initial BLE code adapted from Examples->BLE->Beacon_Scanner.
  Victron decryption code snippets from:
  
    https://github.com/Fabian-Schmidt/esphome-victron_ble

  Information on the "extra manufacturer data" that we're picking up from Victron SmartSolar
  BLE advertising beacons can be found at:
  
    https://community.victronenergy.com/storage/attachments/48745-extra-manufacturer-data-2022-12-14.pdf
  
  Thanks, Victron, for providing both the beacon and the documentation on its contents!
*/
#include <Arduino_GFX_Library.h>
// also includes Arduino etc, so variable names are understood
#include "Structures.h"
#include "VICTRONBLE.h"
#include "aux_functions.h"

#include <BLEDevice.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>

// The Espressif people decided to use String instead of std::string in newer versions
// (3.0 and later?) of their ESP32 libraries. Check your BLEAdvertisedDevice.h file to see
// if this is the case for getManufacturerData(); if so, then uncomment this line so we'll
// use String code in the callback.

//#define USE_String  for version 3 compiler!

#include <aes/esp_aes.h>  // AES library for decrypting the Victron manufacturer data.
extern int Num_Victron_Devices;
extern char VictronBuffer[];  // to get the data out as a string char
extern _sDisplay_Config Display_Config;
extern _sWiFi_settings_Config Current_Settings;
extern _sMyVictronDevices victronDevices;
extern _MyColors ColorSettings;
extern int Display_Page;
extern int text_height;
extern void showPictureFrame(_sButton &button, const char *name);
//char ErrorInChars[50];

extern char *ErrorCodeToChar(VE_REG_CHR_ERROR_CODE val);  //Defined later..
BLEScan *pBLEScan;
// int h, v, width, height, bordersize;  uint16_t BackColor, TextColor, BorderColor;

_sButton Inst_Disp = { 0, 5, 146, 46, 5, WHITE, BLACK, BLUE };  // for Victron battery internal displays, three at 46 V spacing  start 5 down
#define socbar 20

#define AES_KEY_BITS 128

int scanTime = 1;              // BLE scan time (seconds)
#define _BLESCANINTERVAL 2000  // run scan every 10 secs, (results in lockout of all other functions  for scanTime)
char savedDeviceName[32];      // cached copy of the device name (31 chars max) + \0

// Victron docs on the manufacturer data in advertisement packets can be found at:
//   https://community.victronenergy.com/storage/attachments/48745-extra-manufacturer-data-2022-12-14.pdf
//

int bestRSSI = -200;
int selectedvictronDeviceIndex = -1;

time_t lastLEDBlinkTime = 0;
time_t lastTick = 0;
int displayRotation = 3;
bool packetReceived = false;

_sButton Shift(int shift_h, int shift_v, _sButton original) {
  _sButton temp;
  temp.h = original.h;
  temp.v = original.v;
  temp.h = temp.h + shift_h;
  temp.v = temp.v + shift_v;

  temp.width = original.width;
  temp.height = original.height;
  temp.bordersize = original.bordersize;
  temp.BackColor = original.BackColor;
  temp.TextColor = original.TextColor;
  temp.BorderColor = original.BorderColor;
  temp.Font = original.Font;
  temp.Keypressed = original.Keypressed;
  temp.LastDetect = original.LastDetect;
  temp.PrintLine = original.PrintLine;
  temp.screenfull = original.screenfull;
  temp.debugpause = original.debugpause;

  return temp;
};
char *DeviceStateToChar(VE_REG_DEVICE_STATE val) {
  static char Buff[100];
  switch (val) {
    case 0:  //
      strcpy(Buff, "OFF");
      break;
    case 1:  //
      strcpy(Buff, "Low Power");
      break;
    case 2:  //
      strcpy(Buff, "FAULT");
      break;
    case 3:  //
      strcpy(Buff, "BULK");
      break;
    case 4:  //
      strcpy(Buff, "ABSORPTION");
      break;
    case 5:  //
      strcpy(Buff, "FLOAT");
      break;
    case 6:  //
      strcpy(Buff, "STORAGE");
      break;
    case 7:  //
      strcpy(Buff, "EQUALIZE-Manual");
      break;
    case 8:  //
      strcpy(Buff, "PASSTHRU");
      break;
    case 9:  //
      strcpy(Buff, "INVERTING");
      break;
    case 10:  //
      strcpy(Buff, "ASSISTING");
      break;
    case 11:  //
      strcpy(Buff, "POWER_SUPPLY");
      break;
    case 244:  //
      strcpy(Buff, "SUSTAIN");
      break;
    case 245:  //
      strcpy(Buff, "STARTING_UP");
      break;
    case 246:  //
      strcpy(Buff, "Repeated_ABSORPTION");
      break;
    case 247:  //
      strcpy(Buff, "AUTO_EQUALIZE");
      break;
    case 248:  //
      strcpy(Buff, "BATTERY_SAFE");
      break;
    case 249:  //
      strcpy(Buff, "LOAD_DETECT");
      break;
    case 250:  //
      strcpy(Buff, "BLOCKED");
      break;
    case 251:  //
      strcpy(Buff, "TEST");
      break;
    case 252:  //
      strcpy(Buff, "EXTERNAL_CONTROL");
      break;
  }
      return Buff;
};

char chargeStateNames[][6] = {
  "  off",
  "   1?",
  "   2?",
  " bulk",
  "  abs",
  "float",
  "   6?",
  "equal"
};

byte hexCharToByte(char hexChar) {
  if (hexChar >= '0' && hexChar <= '9') {  // 0-9
    hexChar = hexChar - '0';
  } else if (hexChar >= 'a' && hexChar <= 'f') {  // a-f
    hexChar = hexChar - 'a' + 10;
  } else if (hexChar >= 'A' && hexChar <= 'F') {  // A-F
    hexChar = hexChar - 'A' + 10;
  } else {
    hexChar = 255;
  }
  return hexChar;
}

void hexCharStrToByteArray(char *hexCharStr, byte *byteArray) {
  bool returnVal = false;
  int hexCharStrLength = strlen(hexCharStr);
  // There are simpler ways of doing this without the fancy nibble-munching,
  // but I do it this way so I parse things like colon-separated MAC addresses.
  // BUT: be aware that this expects digits in pairs and byte values need to be
  // zero-filled. i.e., a MAC address like 8:0:2b:xx:xx:xx won't come out the way
  // you want it.
  int byteArrayIndex = 0;
  bool oddByte = true;
  byte hiNibble;
  byte nibble;
  //  Serial.printf("  Hex convert %s  has length %i  \n",hexCharStr,hexCharStrLength);
  for (int i = 0; i < hexCharStrLength; i++) {
    nibble = hexCharToByte(hexCharStr[i]);
    if (nibble != 255) {  // miss any ":"
      if (oddByte) {
        hiNibble = nibble;
      } else {
        byteArray[byteArrayIndex] = (hiNibble << 4) | nibble;
        byteArrayIndex++;
      }
      oddByte = !oddByte;
    }
  }
  // do we have a leftover nibble? I guess we'll assume it's a low nibble?
  if (!oddByte) {
    byteArray[byteArrayIndex] = hiNibble;
  }
}

bool CompareString_Mac(char *receivedMacStr, char *charMacAddr) {  //compare received.. <ea:9d:f3:eb:c6:25> with string <ea9df3ebc625> held in indexed
  bool result = true;
  // Serial.printf("CompareString_Mac test  <%s>    <%s> \n", receivedMacStr, charMacAddr);
  // nB could probably do with some UPPER case stuff in case mac was stored UC?
  // and generic loop to miss out the ":" (58d) ??
  int j = 0;
  for (int i = 0; i < sizeof(charMacAddr); i++) {
    if (receivedMacStr[j] == 58) { j++; }
    if (receivedMacStr[j] != charMacAddr[i]) { result = false; }
    j++;
  }
  return result;
}
void DrawBar(_sButton box, char *title, uint16_t color, float data) {
  int top;
  top = box.h + box.width - socbar - (box.bordersize);  // aligned with Right hand side minus width
  int printheight;
  printheight = box.height - (box.bordersize * 2);
  float bar = data * printheight / 100;
  gfx->fillRect(top, (box.v + box.bordersize) + printheight - int(bar), socbar,
                int(bar), color);
  AddTitleBorderBox(0, box, title);
}


_sButton Setup_N_Display(_sButton &box, int numlines, int shiftH, int shiftV, char *title) {
  _sButton Display4outerbox = box;
  Display4outerbox.height = 30 + (numlines * box.height);
  Display4outerbox = Shift(shiftH, shiftV, Display4outerbox);
  GFXBorderBoxPrintf(Display4outerbox, "");  //Used to blank the previous stuff!
  AddTitleBorderBox(0, Display4outerbox, title);
  //
  box = Shift(shiftH, shiftV, box);
  box = Shift(0, box.bordersize, box);  // print just below top border..
  return Display4outerbox;
}





// void DebugBLEMessage(BLEAdvertisedDevice advertisedDevice) {
//   char debugMsg[300];
//   char outbuffer[400];
//   snprintf(debugMsg, 120, "\n--Advertised device: MAC(%s)", advertisedDevice.getAddress().toString().c_str());
//   strcat(outbuffer, debugMsg);
//   if (advertisedDevice.haveAppearance() == true) {
//     snprintf(debugMsg, 120, "appearance_int16<%i>\n", advertisedDevice.getAppearance());
//     strcat(outbuffer, debugMsg);
//     Serial.print(debugMsg);
//   }
//   if (advertisedDevice.haveName() == true) {
//     snprintf(debugMsg, 120, " Name<%s>\n", advertisedDevice.getName().c_str());
//     strcat(outbuffer, debugMsg);
//     Serial.print(debugMsg);
//   }

//   if (advertisedDevice.haveRSSI() == true) {
//     snprintf(debugMsg, 120, " RSSI<%i>\n", advertisedDevice.getRSSI());
//     strcat(outbuffer, debugMsg);
//     Serial.print(debugMsg);
//   }
//   if (advertisedDevice.haveServiceData() == true) {
//     snprintf(debugMsg, 120, " service data count(%i)  (%s)", advertisedDevice.getServiceDataCount(), advertisedDevice.getServiceData().c_str());
//     strcat(outbuffer, debugMsg);
//     Serial.print(debugMsg);
//   }
//   if (advertisedDevice.haveServiceUUID() == true) {
//     snprintf(debugMsg, 120, " has ServiceUUID data count (%i)", advertisedDevice.getServiceUUIDCount());
//     strcat(outbuffer, debugMsg);
//     Serial.print(debugMsg);
//   }
//   if (advertisedDevice.haveTXPower() == true) {
//     snprintf(debugMsg, 120, " TXpwr<%i>\n", advertisedDevice.getTXPower());
//     strcat(outbuffer, debugMsg);
//     Serial.print(debugMsg);
//   }
//   snprintf(debugMsg, 120, "-- end AD data ");
//   Serial.println(outbuffer);
//   strcat(VictronBuffer, outbuffer);
// }




// generic for displays to modify will start with setting relative to Inst_Disp :
_sButton display;
_sButton DisplayOuterbox;

int VictronSimulateIndex;

#define manDataSizeMax 31  // BLE specs say no more than 31 bytes, but... see hoben comments I do not want to change or modify this (yet!)
// read https://github.com/hoberman/Victron_BLE_Scanner_Display/blob/main/BLE_Adv_Callback.ino for comments.
class MyAdvertisedDeviceCallbacks : public BLEAdvertisedDeviceCallbacks {
  void onResult(BLEAdvertisedDevice advertisedDevice) {
    char debugMsg[200];
    // commenting out the USE_String and placing on line ends. until and if I move to v3 compiler! to make code easier to follow
    if (advertisedDevice.haveManufacturerData() == true) {  // ONLY bother with the message if it has "manufacturerdata" and then look to see if it's coming from a Victron device.
      //snprintf(debugMsg, 120, " BLE seen "); strcat(VictronBuffer, debugMsg);Serial.print(debugMsg);
      uint8_t manCharBuf[manDataSizeMax + 1];                        // #ifdef USE_String  // String manData = advertisedDevice.getManufacturerData();  // lib code returns String. // #else
      std::string manData = advertisedDevice.getManufacturerData();  // lib code returns std::string  // #endif
      int manDataSize = manData.length();
      if (manDataSize > manDataSizeMax) {  // Limit size just in case we get a malformed packet.
        Serial.printf("  Note: Truncating malformed %2d byte manufacturer data to max %d byte array size\n", manDataSize, manDataSizeMax);
        manDataSize = manDataSizeMax;
      }
      // Now copy the data from the String to a byte array. Must have the +1 so we
      //   #ifdef USE_String      // //manData.toCharArray((char *)manCharBuf,manDataSize+1);// memcpy(manCharBuf, manData.c_str(), manDataSize);//    #else //manData.copy((char *)manCharBuf, manDataSize + 1);
      manData.copy((char *)manCharBuf, manDataSize);  // #endif
      // Get the Basic stuff from the message that we need later:  strength (RSSI) of the beacon so we can show it in debug -if necessary, regardless of device and manufacturer.
      int RSSI = advertisedDevice.getRSSI();
      std::string getName = advertisedDevice.getName().c_str();
      char receivedMacStr[18];
      strcpy(receivedMacStr, advertisedDevice.getAddress().toString().c_str());  // == toString()(convert a string object into a string) with a null (c_str(): Returns a pointer to a null-terminated C-style string representation)
                                                                                 // Now let's use a struct to get to the data more cleanly.
      victronManufacturerData *vicData = (victronManufacturerData *)manCharBuf;
      // NORMALLY: ignore this packet if the Vendor ID isn't Victron. https://gist.github.com/angorb/f92f76108b98bb0d81c74f60671e9c67
      // snprintf(debugMsg, 120, "  id:%#x Rssi:%i", vicData->vendorID, RSSI);strcat(VictronBuffer, debugMsg); Serial.println(debugMsg);
      if (vicData->vendorID != 0x02e1) { return; }
      snprintf(debugMsg, 120, " Victron Device found:MAC(%s) \n", advertisedDevice.getAddress().toString().c_str());
      strcat(VictronBuffer, debugMsg);
      // Serial.println(debugMsg);
      // Get the MAC address "xx:xx...xx:xx"of the device we're hearing, and then use that to look up the encryption key for the device from our table
      int victronDeviceIndex = -1;
      if (ColorSettings.Simulate) {
        VictronSimulateIndex++;
        if (VictronSimulateIndex >= Num_Victron_Devices) { VictronSimulateIndex = 0; }
        victronDeviceIndex = VictronSimulateIndex;
      } else {  // do the compare to our list of devices!!
        for (int i = 0; i < Num_Victron_Devices; i++) {
          if (CompareString_Mac(receivedMacStr, victronDevices.charMacAddr[i])) {
            victronDeviceIndex = i;
            //     Serial.printf(" recognised as device %x ", victronDeviceIndex);
          }
        }
      }
      // // for record type list see line 300 on https://github.com/Fabian-Schmidt/esphome-victron_ble/blob/main/components/victron_ble/victron_ble.h

      // snprintf(debugMsg, 120, " data : vendor{%x} beacon{%x} length{%x} product id{%x}, record type{%x} encryption_key_0{%x}\n", vicData->vendorID, vicData->beaconType,
      //          vicData->manufacturer_record_length, vicData->product_id, vicData->victronRecordType, vicData->encryption_key_0);
      // // strcat(VictronBuffer, debugMsg);
      // Serial.println(debugMsg);

      int KnownDataType;
      KnownDataType = -1;
      if (vicData->victronRecordType == 0x01) { KnownDataType = 1; }  //Solar Charger
      if (vicData->victronRecordType == 0x02) { KnownDataType = 2; }  //Battery Monitor / smart shunt
      if (vicData->victronRecordType == 0x08) { KnownDataType = 3; }  //VICTRON_BLE_RECORD_AC_CHARGER
                                                                      // TBD case 4 etc

      if (ColorSettings.Simulate) {
        KnownDataType = victronDeviceIndex;
        if (KnownDataType == 1) { snprintf(victronDevices.FileCommentName[victronDeviceIndex], sizeof(victronDevices.FileCommentName[victronDeviceIndex]),
                                           "SIM_SOLAR"); }
        if (KnownDataType == 2) { snprintf(victronDevices.FileCommentName[victronDeviceIndex], sizeof(victronDevices.FileCommentName[victronDeviceIndex]),
                                           "SIM_SMARTSHUNT"); }
        if (KnownDataType == 3) { snprintf(victronDevices.FileCommentName[victronDeviceIndex], sizeof(victronDevices.FileCommentName[victronDeviceIndex]),
                                           "SIM_AC CHARGE"); }
        snprintf(debugMsg, 120, "          Simulating %s, <%i> at position from index:<%i> \n", victronDevices.FileCommentName[victronDeviceIndex], KnownDataType, victronDeviceIndex);
        strcat(VictronBuffer, debugMsg);
        Serial.print(debugMsg);
      }
      char deviceName[32];  // 31 characters + \0
      // Get the device name (if there's one for this device in the index
      strcpy(deviceName, victronDevices.FileCommentName[victronDeviceIndex]);
      bool deviceNameFound = false;
      if (advertisedDevice.haveName()) {
        strcpy(deviceName, advertisedDevice.getName().c_str());  // This works the same whether getName() returns String or std::string.
        deviceNameFound = true;
      }
      // The manufacturer data from Victron contains a byte that's supposed to match the first byte
      // of the device's encryption key. If they don't match, thhen we don't have the right key for decrypt
      //   also have HexStringToBytes to be tested...

      // serial prints while working on strKey to byteKey conversions.
      // strkey (xxxxx)is 'copyable' by cut and paste from app , so is preferred. But aes computation needs the byteArray Key version xx:xx:xx
      //      Serial.printf("hex to byte  input %s \n",victronDevices.charKey[victronDeviceIndex]);
      byte localbyteKey[17];
      hexCharStrToByteArray(victronDevices.charKey[victronDeviceIndex], localbyteKey);
      if (!ColorSettings.Simulate) {  //disable for simulate
        if (vicData->encryption_key_0 != localbyteKey[0]) {
          snprintf(debugMsg, 120, "BUT key is MIS-MATCHED\n");
          Serial.println(debugMsg);
          strcat(VictronBuffer, debugMsg);
          return;
        }
      }
      // Now that the packet received and has met all the criteria for being displayed, let's decrypt and decode the manufacturer data.
      byte inputData[16];
      byte outputData[16] = { 0 };
      // The number of encrypted bytes is given by the number of bytes in the manufacturer
      // data as a while minus the number of bytes (10) in the header part of the data.
      int encrDataSize = manDataSize - 10;
      for (int i = 0; i < encrDataSize; i++) {
        inputData[i] = vicData->victronEncryptedData[i];  // copy for our decrypt below while I figure this out.
      }
      esp_aes_context ctx;
      esp_aes_init(&ctx);
      auto status = esp_aes_setkey(&ctx, localbyteKey, AES_KEY_BITS);
      if (!ColorSettings.Simulate) {
        if (status != 0) {
          Serial.printf("  Error during esp_aes_setkey operation (%i).\n", status);
          esp_aes_free(&ctx);
          return;
        }
      }
      byte data_counter_lsb = (vicData->nonceDataCounter) & 0xff;
      byte data_counter_msb = ((vicData->nonceDataCounter) >> 8) & 0xff;
      u_int8_t nonce_counter[16] = { data_counter_lsb, data_counter_msb, 0 };
      u_int8_t stream_block[16] = { 0 };
      size_t nonce_offset = 0;
      status = esp_aes_crypt_ctr(&ctx, encrDataSize, &nonce_offset, nonce_counter, stream_block, inputData, outputData);
      if (!ColorSettings.Simulate) {
        if (status != 0) {
          Serial.printf("Error during esp_aes_crypt_ctr operation (%i).", status);
          esp_aes_free(&ctx);
          return;
        }
      }
      esp_aes_free(&ctx);
      //Now use and display the data
      if (KnownDataType == 1) {
        VICTRON_BLE_RECORD_SOLAR_CHARGER *victronData = (VICTRON_BLE_RECORD_SOLAR_CHARGER *)outputData;
        byte device_state = victronData->device_state;  // this is really more like "Charger State"
        byte charger_error = victronData->charger_error;
        float battery_voltage = float(victronData->battery_voltage) * 0.01;
        float battery_current = float(victronData->battery_current) * 0.1;
        float yield_today = float(victronData->yield_today) * 0.01 * 1000;  //we use wattHr not kwHr..
        uint16_t pv_power = victronData->pv_power;
        float load_current = float(victronData->load_current) * 0.1;
        // char chargeStateName[6];
        // sprintf(chargeStateName, "%4d?", device_state);
        // strcpy(chargeStateName,DeviceStateToChar(device_state));
        //if (device_state >= 0 && device_state <= 7) { strcpy(chargeStateName, chargeStateNames[device_state]); }

        if (ColorSettings.Simulate) {
          charger_error = 3;device_state=3;
          battery_voltage = random(10.9, 13.5);
          battery_current = random(-2.2, 20.6);
          yield_today = random(1, 200);
          pv_power = random(1, 120);
          load_current = random(0, 10);
        }
        if (Display_Page == -87) {
          DisplayOuterbox = Inst_Disp;
          display = Inst_Disp;
          DisplayOuterbox = Setup_N_Display(display, 3, victronDevices.displayH[victronDeviceIndex], victronDevices.displayV[victronDeviceIndex], victronDevices.FileCommentName[victronDeviceIndex]);
          DisplayOuterbox.PrintLine = 15;
          UpdateTwoSize_MultiLine(1, true, false, 11, 10, DisplayOuterbox, "%3dw", pv_power);
          UpdateTwoSize_MultiLine(1, true, false, 10, 9, DisplayOuterbox, "%2.1FA", battery_current);
          UpdateTwoSize_MultiLine(1, true, false, 10, 9, DisplayOuterbox, "%2.1FA load", load_current);
          UpdateTwoSize_MultiLine(1, false, false, 8, 8, DisplayOuterbox, "%s", DeviceStateToChar(device_state) );
          UpdateTwoSize_MultiLine(1, false, false, 8, 8, DisplayOuterbox, "%s", ErrorCodeToChar(charger_error) );
        }
      }

      if (KnownDataType == 2) {
        VICTRON_BLE_RECORD_BATTERY_MONITOR *victronData = (VICTRON_BLE_RECORD_BATTERY_MONITOR *)outputData;
        float battery_voltage = float(victronData->battery_voltage) * 0.01;
        float battery_current = float(victronData->battery_current) * 0.001;  //4194
        int auxtype = victronData->aux_input_type;
        float aux_input = float(victronData->aux_input) * 0.01;  // starter battery for Shunt
        float state_of_charge = float(victronData->state_of_charge) * 0.1;
        if (ColorSettings.Simulate) {
          battery_voltage = random(10.9, 13.5);
          battery_current = random(-2.2, 20.6);
          auxtype = 2;
          aux_input = 300;
        }
        if (Display_Page == -87) {
          _sButton DisplayBox;
          DisplayOuterbox = Inst_Disp;
          display = Inst_Disp;
          DisplayOuterbox = Setup_N_Display(display, 3, victronDevices.displayH[victronDeviceIndex], victronDevices.displayV[victronDeviceIndex], victronDevices.FileCommentName[victronDeviceIndex]);
          DisplayOuterbox.PrintLine = 5;
          if (battery_current <= 1000) {                                                                           //  less than 1000 must be a smart shunt, the monitor always shows about 2000 amps                                                                   // discriminator for Shunt to monitor??                                                                    // the simple battery monitor seems to send current 2098A and has no state of charge
            DrawBar(DisplayOuterbox, victronDevices.FileCommentName[victronDeviceIndex], GREEN, state_of_charge);  // with name !
            UpdateTwoSize_MultiLine(1, true, false, 11, 10, DisplayOuterbox, "%2.1FV", battery_voltage);
            UpdateTwoSize_MultiLine(1, true, false, 11, 10, DisplayOuterbox, "%2.1FA", battery_current);
          } else {
            UpdateTwoSize_MultiLine(1, true, false, 11, 10, DisplayOuterbox, "%2.1FV", battery_voltage);
          }
          if (auxtype == 2) {
            UpdateTwoSize_MultiLine(1, true, false, 9, 8, DisplayOuterbox, "");                               //temperature 3rd line // use identifier ?? config.identifier[index]
            UpdateTwoSize_MultiLine(1, true, false, 9, 8, DisplayOuterbox, "%2.0f deg", aux_input - 273.15);  //this for deg C original data  is in kelvin. Fabian says only 1degree, but I think it is 0.1 resolution and only 1 degree resolution !
          }
          if (auxtype == 0) {
            UpdateTwoSize_MultiLine(1, true, false, 9, 8, DisplayOuterbox, "");
            UpdateTwoSize_MultiLine(1, true, false, 9, 8, DisplayOuterbox, "Starter %2.1fV", aux_input);
          }
         
        }
      }

      if (KnownDataType == 3) {
        VICTRON_BLE_RECORD_AC_CHARGER *victronData = (VICTRON_BLE_RECORD_AC_CHARGER *)outputData;
        byte device_state = victronData->device_state; 
        int ERRORCODE = victronData->charger_error;
        float battery_voltage_1 = victronData->battery_voltage_1 * 0.01;
        float battery_current_1 = victronData->battery_current_1 * 0.1;
        float battery_voltage_2 = victronData->battery_voltage_2 * 0.01;
        float battery_current_2 = victronData->battery_current_2 * 0.1;
        float temperature = victronData->temperature;
        if (ColorSettings.Simulate) {
          ERRORCODE = 2;device_state=4;
          battery_voltage_1 = random(10.9, 13.5);
          battery_current_1 = random(-2.2, 20.6);
          battery_voltage_2 = random(10.9, 13.5);
          battery_current_2 = random(-2.2, 20.6);
          temperature = 300;
        }
        if (Display_Page == -87) {
          DisplayOuterbox = Inst_Disp;
          display = Inst_Disp;
          DisplayOuterbox = Setup_N_Display(display, 5, victronDevices.displayH[victronDeviceIndex], victronDevices.displayV[victronDeviceIndex], victronDevices.FileCommentName[victronDeviceIndex]);
          DisplayOuterbox.PrintLine = 25;
          UpdateTwoSize_MultiLine(1, true, false, 10, 9, DisplayOuterbox, "%2.1FV (1)", battery_voltage_1);
          UpdateTwoSize_MultiLine(1, true, false, 10, 9, DisplayOuterbox, "%2.1FA (1)", battery_current_1);
          UpdateTwoSize_MultiLine(1, true, false, 10, 9, DisplayOuterbox, "%2.1FV (2)", battery_voltage_2);
          UpdateTwoSize_MultiLine(1, true, false, 10, 9, DisplayOuterbox, "%2.1FA (2)", battery_current_2);
          UpdateTwoSize_MultiLine(1, true, false, 8, 8, DisplayOuterbox, "\n");
          UpdateTwoSize_MultiLine(1, false, false, 8, 8, DisplayOuterbox, "%s", DeviceStateToChar(device_state) );
          UpdateTwoSize_MultiLine(1, true, false, 8, 8, DisplayOuterbox, "%2.0f deg", temperature - 273.15);
          UpdateTwoSize_MultiLine(1, false, false, 8, 8, DisplayOuterbox, "%s", ErrorCodeToChar(ERRORCODE));
        }
      }
      packetReceived = true;
    }
  }
};

void BLEsetup() {
  Serial.print("Setting up BLE..");

  //  set up test box size modifications??

  delay(500);
  BLEDevice::init("");
  pBLEScan = BLEDevice::getScan();  //create new scan
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setActiveScan(true);  //active scan uses more power, but gets results faster
  pBLEScan->setInterval(100);
  pBLEScan->setWindow(99);  // less or equal setInterval value
  Serial.println(F(" BLE setup() complete."));
}
void BLEloop() {
  // Serial.print(" BLE Scanning...");
  static unsigned long BLESCANINTERVAL;
  if (millis() >= BLESCANINTERVAL) {
    BLESCANINTERVAL = millis() + _BLESCANINTERVAL;                   //!
    BLEScanResults foundDevices = pBLEScan->start(scanTime, false);  // what does the iscontinue do? (the true/false is set false in examples. )
    pBLEScan->clearResults();                                        // delete results fromBLEScan buffer to release memory
  }
}





char *ErrorCodeToChar(VE_REG_CHR_ERROR_CODE val) {
    static char Buff[100];
    switch (val) {
      case 0:  //VE_REG_CHR_ERROR_CODE::NO_ERROR:
        strcpy(Buff, "");
        break;
      case 1:  //VE_REG_CHR_ERROR_CODE::TEMPERATURE_BATTERY_HIGH:
        strcpy(Buff, "Battery temp high");
        break;
      case 2:  //VE_REG_CHR_ERROR_CODE::VOLTAGE_HIGH:
        strcpy(Buff, "Battery voltage high");
        break;
      case 3:  //VE_REG_CHR_ERROR_CODE::REMOTE_TEMPERATURE_A:
        strcpy(Buff, "Remote temperature A sensor failure (auto-reset)");
        break;
      case 4:  // VE_REG_CHR_ERROR_CODE::REMOTE_TEMPERATURE_B:
        strcpy(Buff, "Remote temperature B sensor failure (auto-reset)");
        break;
      case 5:  //VE_REG_CHR_ERROR_CODE::REMOTE_TEMPERATURE_C:
        strcpy(Buff, "Remote temperature C sensor failure (not auto-reset)");
        break;
      case 6:  // VE_REG_CHR_ERROR_CODE::REMOTE_BATTERY_A:
        strcpy(Buff, "Remote battery voltage sense failure");
        break;
      case 7:  //VE_REG_CHR_ERROR_CODE::REMOTE_BATTERY_B:
        strcpy(Buff, "Remote battery voltage sense failure");
        break;
      case 8:  //VE_REG_CHR_ERROR_CODE::REMOTE_BATTERY_C:
        strcpy(Buff, "Remote battery voltage sense failure");
        break;
      case 11:  //VE_REG_CHR_ERROR_CODE::HIGH_RIPPLE:
        strcpy(Buff, "Battery temperature too low");
        break;
      case 17:  //VE_REG_CHR_ERROR_CODE::TEMPERATURE_CHARGER:
        strcpy(Buff, "Charger temperature too high");
        break;
      case 18:  //VE_REG_CHR_ERROR_CODE::OVER_CURRENT:
        strcpy(Buff, "Charger over current");
        break;
      case 19:  //VE_REG_CHR_ERROR_CODE::POLARITY:
        strcpy(Buff, "Charger current polarity reversed");
        break;
      case 20:  //VE_REG_CHR_ERROR_CODE::BULK_TIME:
        strcpy(Buff, "Bulk time limit exceeded");
        break;
      case 21:  //VE_REG_CHR_ERROR_CODE::CURRENT_SENSOR:
        strcpy(Buff, "Current sensor issue (sensor bias/sensor broken)");
        break;
      case 22:  //VE_REG_CHR_ERROR_CODE::INTERNAL_TEMPERATURE_A:
        strcpy(Buff, "Internal temperature sensor failure");
        break;
      case 23:  //VE_REG_CHR_ERROR_CODE::INTERNAL_TEMPERATURE_B:
        strcpy(Buff, "Internal temperature sensor failure");
        break;
      case 24:  //VE_REG_CHR_ERROR_CODE::FAN:
        strcpy(Buff, "Fan failure");
        break;
      case 26:  //VE_REG_CHR_ERROR_CODE::OVERHEATED:
        strcpy(Buff, "Terminals overheated");
        break;
      case 27:  //VE_REG_CHR_ERROR_CODE::SHORT_CIRCUIT:
        strcpy(Buff, "Charger short circuit");
        break;
      case 28:  //VE_REG_CHR_ERROR_CODE::CONVERTER_ISSUE:
        strcpy(Buff, "Power stage issue");
        break;
      case 29:  //VE_REG_CHR_ERROR_CODE::OVER_CHARGE:
        strcpy(Buff, "Over-Charge protection");
        break;
      case 33:  //VE_REG_CHR_ERROR_CODE::INPUT_VOLTAGE:
        strcpy(Buff, "PV over-voltage");
        break;
      case 34:  //VE_REG_CHR_ERROR_CODE::INPUT_CURRENT:
        strcpy(Buff, "PV over-current");
        break;
      case 35:  //VE_REG_CHR_ERROR_CODE::INPUT_POWER:
        strcpy(Buff, "PV over-power");
        break;
      case 38:  //VE_REG_CHR_ERROR_CODE::INPUT_SHUTDOWN_VOLTAGE:
        strcpy(Buff, "Input shutdown (due to excessive battery voltage)");
        break;
      case 39:  //VE_REG_CHR_ERROR_CODE::INPUT_SHUTDOWN_CURRENT:
        strcpy(Buff, "Input shutdown (due to current flow during off mode)");
        break;
      case 40:  //VE_REG_CHR_ERROR_CODE::INPUT_SHUTDOWN_FAILURE:
        strcpy(Buff, "PV Input failed to shutdown");
        break;
      case 41:  //VE_REG_CHR_ERROR_CODE::INVERTER_SHUTDOWN_41:
        strcpy(Buff, "Inverter shutdown (PV isolation)");
        break;
      case 42:  //VE_REG_CHR_ERROR_CODE::INVERTER_SHUTDOWN_42:
        strcpy(Buff, "Inverter shutdown (PV isolation)");
        break;
      case 43:  //VE_REG_CHR_ERROR_CODE::INVERTER_SHUTDOWN_43:
        strcpy(Buff, "Inverter shutdown (Ground Fault)");
        break;
      case 50:  //VE_REG_CHR_ERROR_CODE::INVERTER_OVERLOAD:
        strcpy(Buff, "Inverter overload");
        break;
      case 51:  //VE_REG_CHR_ERROR_CODE::INVERTER_TEMPERATURE:
        strcpy(Buff, "Inverter temperature too high");
        break;
      case 52:  //VE_REG_CHR_ERROR_CODE::INVERTER_PEAK_CURRENT:
        strcpy(Buff, "Inverter peak current");
        break;
      case 53:  //VE_REG_CHR_ERROR_CODE::INVERTER_OUPUT_VOLTAGE_A:
        strcpy(Buff, "Inverter output voltage");
        break;
      case 54:  //VE_REG_CHR_ERROR_CODE::INVERTER_OUPUT_VOLTAGE_B:
        strcpy(Buff, "Inverter output voltage");
        break;
      case 55:  //VE_REG_CHR_ERROR_CODE::INVERTER_SELF_TEST_A:
        strcpy(Buff, "Inverter self test failed");
        break;
      case 56:  //VE_REG_CHR_ERROR_CODE::INVERTER_SELF_TEST_B:
        strcpy(Buff, "Inverter self test failed");
        break;
      case 57:  //VE_REG_CHR_ERROR_CODE::INVERTER_AC:
        strcpy(Buff, "Inverter ac voltage on output");
        break;
      case 58:  //VE_REG_CHR_ERROR_CODE::INVERTER_SELF_TEST_C:
        strcpy(Buff, "Inverter self test failed");
        break;
      case 65:  //VE_REG_CHR_ERROR_CODE::COMMUNICATION:
        strcpy(Buff, "Information 65 - Communication warning");
        break;
      case 66:  //VE_REG_CHR_ERROR_CODE::SYNCHRONISATION:
        strcpy(Buff, "Information 66 - Incompatible device");
        break;
      case 67:  // VE_REG_CHR_ERROR_CODE::BMS:
        strcpy(Buff, "BMS Connection lost");
        break;
      case 68:  //VE_REG_CHR_ERROR_CODE::NETWORK_A:
        strcpy(Buff, "Network misconfigured");
        break;
      case 69:  //VE_REG_CHR_ERROR_CODE::NETWORK_B:
        strcpy(Buff, "Network misconfigured");
        break;
      case 70:  //VE_REG_CHR_ERROR_CODE::NETWORK_C:
        strcpy(Buff, "Network misconfigured");
        break;
      case 71:  //VE_REG_CHR_ERROR_CODE::NETWORK_D:
        strcpy(Buff, "Network misconfigured");
        break;
      case 80:  //VE_REG_CHR_ERROR_CODE::PV_INPUT_SHUTDOWN_80:
        strcpy(Buff, "PV Input shutdown");
        break;
      case 81:  //VE_REG_CHR_ERROR_CODE::PV_INPUT_SHUTDOWN_81:
        strcpy(Buff, "PV Input shutdown");
        break;
      case 82:  //VE_REG_CHR_ERROR_CODE::PV_INPUT_SHUTDOWN_82:
        strcpy(Buff, "PV Input shutdown");
        break;
      case 83:  //VE_REG_CHR_ERROR_CODE::PV_INPUT_SHUTDOWN_83:
        strcpy(Buff, "PV Input shutdown");
        break;
      case 84:  //VE_REG_CHR_ERROR_CODE::PV_INPUT_SHUTDOWN_84:
        strcpy(Buff, "PV Input shutdown");
        break;
      case 85:  //VE_REG_CHR_ERROR_CODE::PV_INPUT_SHUTDOWN_85:
        strcpy(Buff, "PV Input shutdown");
        break;
      case 86:  //VE_REG_CHR_ERROR_CODE::PV_INPUT_SHUTDOWN_86:
        strcpy(Buff, "PV Input shutdown");
        break;
      case 87:  //VE_REG_CHR_ERROR_CODE::PV_INPUT_SHUTDOWN_87:
        strcpy(Buff, "PV Input shutdown");
        break;
      case 114:  //VE_REG_CHR_ERROR_CODE::CPU_TEMPERATURE:
        strcpy(Buff, "CPU temperature too high");
        break;
      case 116:  //VE_REG_CHR_ERROR_CODE::CALIBRATION_LOST:
        strcpy(Buff, "Factory calibration data lost");
        break;
      case 117:  //VE_REG_CHR_ERROR_CODE::FIRMWARE:
        strcpy(Buff, "Invalid/incompatible firmware");
        break;
      case 119:  //VE_REG_CHR_ERROR_CODE::SETTINGS:
        strcpy(Buff, "Settings data lost");
        break;
      case 121:  //VE_REG_CHR_ERROR_CODE::TESTER_FAIL:
        strcpy(Buff, "Tester fail");
        break;
      case 200:  //VE_REG_CHR_ERROR_CODE::INTERNAL_DC_VOLTAGE_A:
        strcpy(Buff, "Internal DC voltage error");
        break;
      case 201:  //VE_REG_CHR_ERROR_CODE::INTERNAL_DC_VOLTAGE_B:
        strcpy(Buff, "Internal DC voltage error");
        break;
      case 202:  //VE_REG_CHR_ERROR_CODE::SELF_TEST:
        strcpy(Buff, "PV residual current sensor self-test failure");
        break;
      case 203:  //VE_REG_CHR_ERROR_CODE::INTERNAL_SUPPLY_A:
        strcpy(Buff, "Internal supply voltage error");
        break;
      case 205:  //VE_REG_CHR_ERROR_CODE::INTERNAL_SUPPLY_B:
        strcpy(Buff, "Internal supply voltage error");
        break;
      case 212:  //VE_REG_CHR_ERROR_CODE::INTERNAL_SUPPLY_C:
        strcpy(Buff, "Internal supply voltage error");
        break;
      case 215:  //VE_REG_CHR_ERROR_CODE::INTERNAL_SUPPLY_D:
        strcpy(Buff, "Internal supply voltage error");
        break;
      case 255:  //VE_REG_CHR_ERROR_CODE::NOT_AVAILABLE:
        strcpy(Buff, "Not available");
        break;
      default:
        strcpy(Buff, "Unknown");
        break;
    }
    // Serial.println(Buff);
    // Serial.printf(" is %s",Buff);
    return Buff;
};
