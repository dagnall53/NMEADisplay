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
// generic button for displays to modify h,v and height from Vconfig file. Will start with settings relative to Vic_Inst_Master :
_sButton DisplayOuterbox;
_sButton Vic_Inst_Master = { 0, 0, 146, 46, 5, WHITE, BLACK, BLUE };  // for Victron battery internal displays, three at 46 V spacing  start 5 down
#define socbar 20
#define greyoutTime 12000
#define AES_KEY_BITS 128

int scanTime = 1;              // BLE scan time (seconds)
#define _BLESCANINTERVAL 3000  // run scan every xx secs, (results in lockout of all other functions  for scanTime)
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
      strcpy(Buff, "EQUALIZE");
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
      strcpy(Buff, "-PSU-");
      break;
    case 244:  //
      strcpy(Buff, "SUSTAIN");
      break;
    case 245:  //
      strcpy(Buff, "START_UP");
      break;
    case 246:  //
      strcpy(Buff, "ABSORPTION");
      break;
    case 247:  //
      strcpy(Buff, "EQUALIZE");
      break;
    case 248:  //
      strcpy(Buff, "BAT_SAFE");
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
      strcpy(Buff, "EXT_CONTROL");
      break;
  }
  return Buff;
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
 // AddTitleBorderBox(0, box, title);
}


_sButton Setup_N_Display(_sButton &box, int height, int shiftH, int shiftV, char *title) { // takes charactersitics of 'box, but changes position and height as required
  _sButton Display4outerbox = box;
  Display4outerbox.height = height;
  Display4outerbox = Shift(shiftH, shiftV, Display4outerbox);
  //Serial.printf(" BOX h%i v%i %i high \n",Display4outerbox.h,Display4outerbox.v,Display4outerbox.height);
  GFXBorderBoxPrintf(Display4outerbox, "");  //Used to blank the previous stuff!
  AddTitleBorderBox(0, Display4outerbox, title);
  return Display4outerbox;
}


void DebugRawVdata(byte * outputData, int datasize) {
  //  work on outputData[16]
  char debugMsg[200];
  snprintf(debugMsg, 120, "Raw Data :");
  strcat(VictronBuffer, debugMsg);
  Serial.print("Raw Data");
  for (int i = 0; i < datasize; i++) {
    Serial.printf("[%i]=%02x ", i, outputData[i]);
    snprintf(debugMsg, 120, "[%i]=%02x ", i, outputData[i]);
    strcat(VictronBuffer, debugMsg);}
    snprintf(debugMsg, 120, "\n");
    strcat(VictronBuffer, debugMsg);
    Serial.println("");
  
 // 
 //  Serial.print(outputData[1]);
  // char debugMsg[200];
  // char outmsg[100];
  // outmsg[0]=0;
  // snprintf(debugMsg, 120, "RAW decrypted data");
  // strcat(outmsg, debugMsg);
  // for (int i = 0; i < 15; i++) {
  //   snprintf(debugMsg, 120, "[%x]= %x", i, outputData[i]);
  //   strcat(outmsg, debugMsg);
  // }
  // snprintf(debugMsg, 120, "\n");
  // strcat(outmsg, debugMsg);
  // strcat(VictronBuffer, outmsg);
  // Serial.println(outmsg);
}




int VictronSimulateIndex;
bool SHOWRAWBLE = false;
int FoundMyDevices;
//#define ManuDataLengthMax 31  // BLE specs say no more than 31 bytes, but... see hoben comments .. may 25' I do not want to change or modify this (yet!)
// read https://github.com/hoberman/Victron_BLE_Scanner_Display/blob/main/BLE_Adv_Callback.ino for comments.
class MyAdvertisedDeviceCallbacks : public BLEAdvertisedDeviceCallbacks {
  void onResult(BLEAdvertisedDevice advertisedDevice) {
    char debugMsg[200];
    if (advertisedDevice.haveManufacturerData() == true) {  // ONLY bother with the message if it has "manufacturerdata" and then look to see if it's coming from a Victron device.
      // get all the data we can.
      std::string manData = advertisedDevice.getManufacturerData();  // lib code returns std::string
      int RSSI = advertisedDevice.getRSSI();
      char deviceName[32];  // 31 characters + \0
      deviceName[0] = 0;
      std::string getName = advertisedDevice.getName().c_str();  //
      if (advertisedDevice.haveName()) { strcpy(deviceName, advertisedDevice.getName().c_str()); }
      char receivedMacStr[18];                                                   // mac with colons!
      strcpy(receivedMacStr, advertisedDevice.getAddress().toString().c_str());  // == toString()(convert a string object into a string) with a null (c_str(): Returns a pointer to a null-terminated C-style string representation)
      uint8_t manCharBuf[100];                                                   // 32 is possibly entirely enough see ble example
      int ManuDataLength = manData.length();
      manData.copy((char *)manCharBuf, ManuDataLength);
      if (SHOWRAWBLE) {
        //https://gist.github.com/angorb/f92f76108b98bb0d81c74f60671e9c67#file-bluetooth-company-identifiers-json
        if ((manCharBuf[0] == 0x4C) && (manCharBuf[1] == 0x00) && (manCharBuf[2] == 0x10)) { Serial.print(" Apple advertized data"); }
        if ((manCharBuf[0] == 0x06) && (manCharBuf[1] == 0x00)) { Serial.print(" Microsoft "); }
        if ((manCharBuf[0] == 0xE1) && (manCharBuf[1] == 0x02) && (manCharBuf[2] == 0x10)) {  // This is a Victron (02E1) Advertizing beacon (0x10)
          Serial.print(" Victron beacon ");
        }
        if (advertisedDevice.haveName()) {
          Serial.print("Device name: ");
          Serial.print(deviceName);
          Serial.print(" ");
        } else {
          Serial.print("No - name: ");
        }
        Serial.printf("datalen %i ", manData.length());
        for (int i = 0; i < manData.length(); i++) { Serial.printf("[%X]", manCharBuf[i]); }
        Serial.printf("\n");
      }

      if ((manCharBuf[0] == 0xE1) && (manCharBuf[1] == 0x02) && (manCharBuf[2] == 0x10)) {  // This is a Victron (02E1) Advertizing beacon (0x10)
        //Serial.print(" Victron beacon ");
        for (int i = 0; i < Num_Victron_Devices; i++) {
          if (CompareString_Mac(receivedMacStr, victronDevices.charMacAddr[i])) {
            FoundMyDevices++;
            victronDevices.displayed[i] = false;
            victronDevices.greyed[i] = false;
            victronDevices.updated[i] = millis();
            strcpy(victronDevices.DeviceVictronName[i], deviceName);
            //  Serial.printf("Recognised as device %x  building data \n", i);
            victronDevices.ManuDataLength[i] = manData.length();
            for (int j = 0; j < manData.length(); j++) {
              //     Serial.printf("[%X]", manCharBuf[j]);
              victronDevices.manCharBuf[i][j] = manCharBuf[j];  // Matches one of our known devices so copy data for decrypt later
            }
          }
        }
      }  //data now safely saved in my _sMyVictronDevices structured array to be worked on later!
      packetReceived = true;
    }
  }
};

void BLEsetup() {
  Serial.print("Setting up BLE..");
  delay(500);
  BLEDevice::init("");
  pBLEScan = BLEDevice::getScan();  //create new scan
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setActiveScan(true);  //active scan uses more power, but gets results faster
  pBLEScan->setInterval(100);
  pBLEScan->setWindow(99);  // less or equal setInterval value
  for (int i = 0; i < Num_Victron_Devices; i++) {
    victronDevices.displayed[i] = false;
    victronDevices.greyed[i] = true; 
    victronDevices.updated[i] = millis();
  }
  Serial.println(F(" BLE setup() complete."));
}

void Deal_With_BLE_Data(int i) { // BLE message will have been saved into structure with index i
  // we know the victronDevices data should be for one of our device MAC, and only victron
  // added greying (outdated data) test
  char debugMsg[200];
  if (victronDevices.greyed[i]) {  // Serial.printf("%i is Greyed\n",i);
    return;
  }
  bool recent = (victronDevices.updated[i] + greyoutTime >= millis() );
  if (!recent && !victronDevices.greyed[i] && victronDevices.displayed[i]) {
    victronDevices.greyed[i] = true;  // set it so we do not repeat!
    victronDevices.displayed[i] = false;
  };                                  //OLD DATA  update Vic_Inst_Master in GREY!!  just pretend we have new data while we grey it
  if (victronDevices.displayed[i]) {  //Serial.printf(" %i displayed already\n",i);
    return;
  }

  snprintf(debugMsg, 120, "\nDevice<%i>,", i);  // not essential, but makes NMEAlog and data display tidier
  strcat(VictronBuffer, debugMsg);

  if (victronDevices.greyed[i]) {snprintf(debugMsg, 120, "-Greying ");  // not essential, but makes NMEAlog and data display tidier
  strcat(VictronBuffer, debugMsg);}
  // Now let's use a struct to get to the data more cleanly we saved the raw data in the BLE callback
  victronManufacturerData *vicData = (victronManufacturerData *)victronDevices.manCharBuf[i];
  // // for record type list see line 300 on https://github.com/Fabian-Schmidt/esphome-victron_ble/blob/main/components/victron_ble/victron_ble.h
  // Product mapping in decimal: https://github.com/keshavdv/victron-ble/blob/main/Victron_ProductId_mapping.txt
  snprintf(debugMsg, 120, " record type<%x>,ProductID(%i),", vicData->victronRecordType, vicData->product_id);
  strcat(VictronBuffer, debugMsg);
  //Serial.println(debugMsg);
  int KnownDataType;
  KnownDataType = -1;
  if (vicData->victronRecordType == 0x01) { KnownDataType = 1; }  //Solar Charger
  if (vicData->victronRecordType == 0x02) { KnownDataType = 2; }  //Battery Monitor / smart shunt
  if (vicData->victronRecordType == 0x08) { KnownDataType = 8; }  //VICTRON_BLE_RECORD_AC_CHARGER
                                                                  //

  if (ColorSettings.Simulate) { // NOTE these simulations work best (least confusingly!) with my default Vconfig that has Solar on 1 smartshunt on 2, Ac on 5 ..
     // this is not a universal solution.
    switch (i){
         case 1:  KnownDataType=1; 
               snprintf(victronDevices.FileCommentName[i], sizeof(victronDevices.FileCommentName[i]),"%iSIM_SOLAR",i); 
      break;
         case 3:  KnownDataType=1; 
               snprintf(victronDevices.FileCommentName[i], sizeof(victronDevices.FileCommentName[i]),"%iSIM_SOLAR",i); 
      break;

      case 5:  KnownDataType=8; 
               snprintf(victronDevices.FileCommentName[i], sizeof(victronDevices.FileCommentName[i]),"%iSIM_AC_CHRG",i); 
      break;
      
     default: // simulate shunts everywhere else!
               KnownDataType=2; 
               snprintf(victronDevices.FileCommentName[i], sizeof(victronDevices.FileCommentName[i]),"%iSIM_BAT_MON",i); 
      break;

    }
    snprintf(debugMsg, 120, " Simulating %s, <%i> at position from index:<%i> \n", victronDevices.FileCommentName[i], KnownDataType, i);
    strcat(VictronBuffer, debugMsg);
    //Serial.print(debugMsg);
  }
  //Decode / decrypt.. 
  byte localbyteKey[17];
  hexCharStrToByteArray(victronDevices.charKey[i], localbyteKey);
  if (!ColorSettings.Simulate) {  //disable for simulate
    if (vicData->encryption_key_0 != localbyteKey[0]) {
      snprintf(debugMsg, 120, "BUT key is MIS-MATCHED %x Localbyte(0)%x\n", localbyteKey[0]);      //Serial.println(debugMsg);
      strcat(VictronBuffer, debugMsg);
      victronDevices.displayed[i] = true;  // stop trying again until we get better data!
      return;
    }
  }
  // Now that the packet received and has met all the criteria for being displayed, let's decrypt and decode the manufacturer data.
  byte inputData[16];
  byte outputData[16] = { 0 };  // 128 bits of data
  // The number of encrypted bytes is given by the number of bytes in the manufacturer
  // data as a while minus the number of bytes (10) in the header part of the data.
  int encrDataSize = victronDevices.ManuDataLength[i] - 10;
  for (int i = 0; i < encrDataSize; i++) {
    inputData[i] = vicData->victronEncryptedData[i];  // copy for our decrypt below while I figure this out.
  }
  esp_aes_context ctx;
  esp_aes_init(&ctx);
  auto status = esp_aes_setkey(&ctx, localbyteKey, AES_KEY_BITS);
  if (!ColorSettings.Simulate) {
    if (status != 0) {
      snprintf(debugMsg, 120, " Error in esp_aes_setkey op (%i)\n", status);
      strcat(VictronBuffer, debugMsg);
      esp_aes_free(&ctx);
      return;
    }
  }
 
  if (!ColorSettings.Simulate) {
  byte data_counter_lsb = (vicData->data_counter_lsb);
  byte data_counter_msb = (vicData->data_counter_msb);
  u_int8_t nonce_counter[16] = { data_counter_lsb, data_counter_msb, 0 };
  u_int8_t stream_block[16] = { 0 };
  size_t nonce_offset = 0;
  status = esp_aes_crypt_ctr(&ctx, encrDataSize, &nonce_offset, nonce_counter, stream_block, inputData, outputData);
  if (status != 0) {
      //Serial.printf("Error during esp_aes_crypt_ctr operation (%i).", status);
      snprintf(debugMsg, 120, " Error esp_aes_crypt_ctr operation (%i).", status);
      strcat(VictronBuffer, debugMsg);
      esp_aes_free(&ctx);
      return;
    }
   esp_aes_free(&ctx);
  }
if (ColorSettings.ShowRawDecryptedDataFor==i) {DebugRawVdata(outputData,encrDataSize);}
  if (KnownDataType == 1) {
    VICTRON_BLE_RECORD_SOLAR_CHARGER *victronData = (VICTRON_BLE_RECORD_SOLAR_CHARGER *)outputData;
    byte device_state = victronData->device_state;  // this is really more like "Charger State"
    byte charger_error = victronData->charger_error;
    float battery_voltage = float(victronData->battery_voltage) * 0.01;
    float battery_current = float(victronData->battery_current) * 0.1;
    float yield_today = float(victronData->yield_today) * 0.01 * 1000;  //we use wattHr not kwHr..
    uint16_t pv_power = victronData->pv_power;
    float load_current = float(victronData->load_current) * 0.1;
    
    if (ColorSettings.Simulate) {
      charger_error = int(random(0, 20));
      device_state = int(random(0, 5));
      battery_voltage = random(10.9, 13.5);
      battery_current = random(-2.2, 20.6);
      yield_today = random(200,100);
      pv_power = random(1, 120);
      load_current = random(1,10);
    }
    
    snprintf(debugMsg, 120, "PVPower<%3.2f> Volts<%3.2f>,AMPS<%3.2f>, load%3.2fA\n ", pv_power, battery_voltage, battery_current, load_current);
    strcat(VictronBuffer, debugMsg);
    if (Display_Page == -87) {
      DisplayOuterbox = Setup_N_Display(Vic_Inst_Master, victronDevices.displayHeight[i], victronDevices.displayH[i], victronDevices.displayV[i], victronDevices.FileCommentName[i]);
      if (victronDevices.greyed[i]) { DisplayOuterbox.TextColor = DARKGREY; }
      DisplayOuterbox.PrintLine = 5;
      if (strstr(victronDevices.DisplayShow[i],"P")) {UpdateTwoSize_MultiLine(1, true, false, 11, 10, DisplayOuterbox, "%3dw", pv_power);}
      if (strstr(victronDevices.DisplayShow[i],"V")) {UpdateTwoSize_MultiLine(1, true, false, 10, 9, DisplayOuterbox, "%2.1FV", battery_voltage);}
      if (strstr(victronDevices.DisplayShow[i],"I")) {UpdateTwoSize_MultiLine(1, true, false, 10, 9, DisplayOuterbox, "%2.1FA", battery_current);}
      if (strstr(victronDevices.DisplayShow[i],"L")) {UpdateTwoSize_MultiLine(1, true, false, 10, 9, DisplayOuterbox, "%2.1FA load", load_current);}
      if (strstr(victronDevices.DisplayShow[i],"E")) {UpdateTwoSize_MultiLine(1, true, false, 8, 8, DisplayOuterbox, "%s", DeviceStateToChar(device_state));
      UpdateTwoSize_MultiLine(1, true, false, 8, 8, DisplayOuterbox, "%s", ErrorCodeToChar(charger_error));}
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
      battery_voltage = random( 10.9, 13.5);
      battery_current = random(-2.2, 20.6);
      state_of_charge = 50;
      auxtype = 2;
      aux_input = 300;
    }
    snprintf(debugMsg, 120, "Volts<%3.2f>,AMPS<%3.2f>, SOC%3.2f, AUX%3.2f \n", battery_voltage, battery_current, state_of_charge, aux_input);
    strcat(VictronBuffer, debugMsg);
    if (Display_Page == -87) {
      DisplayOuterbox = Setup_N_Display(Vic_Inst_Master, victronDevices.displayHeight[i], victronDevices.displayH[i], victronDevices.displayV[i], victronDevices.FileCommentName[i]);
      if (victronDevices.greyed[i]) { DisplayOuterbox.TextColor = DARKGREY; }
      DisplayOuterbox.PrintLine = 5;
      if (strstr(victronDevices.DisplayShow[i],"S"))  {DrawBar(DisplayOuterbox, victronDevices.FileCommentName[i], GREEN, state_of_charge);}
      if (strstr(victronDevices.DisplayShow[i],"V"))  {UpdateTwoSize_MultiLine(1, true, false, 11, 10, DisplayOuterbox, "%2.1FV", battery_voltage);}     
      if (strstr(victronDevices.DisplayShow[i],"I"))  {UpdateTwoSize_MultiLine(1, true, false, 11, 10, DisplayOuterbox, "%2.1FA", battery_current);}
      if (strstr(victronDevices.DisplayShow[i],"A"))  { 
            if (auxtype == 2) {
        UpdateTwoSize_MultiLine(1, true, false, 9, 8, DisplayOuterbox, "");                               //temperature 3rd line // use DisplayShow ?? config.DisplayShow[index]
        UpdateTwoSize_MultiLine(1, true, false, 9, 8, DisplayOuterbox, "%2.0f deg", aux_input - 273.15);  //this for deg C original data  is in kelvin. Fabian says only 1degree, but I think it is 0.1 resolution and only 1 degree resolution !
        }
            if (auxtype == 0) {
        UpdateTwoSize_MultiLine(1, true, false, 9, 8, DisplayOuterbox, "");
        UpdateTwoSize_MultiLine(1, true, false, 9, 8, DisplayOuterbox, "Starter %2.1fV", aux_input);
        } 
      }
    }
  }

  if (KnownDataType == 8) {
    VICTRON_BLE_RECORD_AC_CHARGER *victronData = (VICTRON_BLE_RECORD_AC_CHARGER *)outputData;
    byte device_state = victronData->device_state;
    int charger_error = victronData->charger_error;
    float battery_voltage_1 = victronData->battery_voltage_1 * 0.01;
    float battery_current_1 = victronData->battery_current_1 * 0.1;
    float battery_voltage_2 = victronData->battery_voltage_2 * 0.01;
    float battery_current_2 = victronData->battery_current_2 * 0.1;
    float temperature = victronData->temperature;
    if (ColorSettings.Simulate) {
      charger_error = int(random(0, 20));
      device_state = int(random(0, 5));
      battery_voltage_1= random(10.9, 13.5);
      battery_current_1=random(-2.2, 20.6);
      battery_voltage_2=random(10.9, 13.5);
      battery_current_2 = random(-2.2, 20.6);
      temperature = 300;
    }
    snprintf(debugMsg, 120, "battery_voltage_1<%3.2f> battery_current_1<%3.2f>, load%3.2fA, temp%2.0f deg\n", battery_voltage_1, battery_current_1, temperature - 273.15);
    strcat(VictronBuffer, debugMsg);
    if (Display_Page == -87) {
      DisplayOuterbox = Setup_N_Display(Vic_Inst_Master, victronDevices.displayHeight[i], victronDevices.displayH[i], victronDevices.displayV[i], victronDevices.FileCommentName[i]);
      if (victronDevices.greyed[i]) { DisplayOuterbox.TextColor = DARKGREY; }
      DisplayOuterbox.PrintLine = 5;
      if (strstr(victronDevices.DisplayShow[i],"V")) {UpdateTwoSize_MultiLine(1, true, false, 10, 9, DisplayOuterbox, "%2.1FV1", battery_voltage_1);}
      if (strstr(victronDevices.DisplayShow[i],"I")) {UpdateTwoSize_MultiLine(1, true, false, 10, 9, DisplayOuterbox, "%2.1FA1", battery_current_1);}
      if (strstr(victronDevices.DisplayShow[i],"v")) {UpdateTwoSize_MultiLine(1, true, false, 10, 9, DisplayOuterbox, "%2.1FV2", battery_voltage_2);}
      if (strstr(victronDevices.DisplayShow[i],"i")) {UpdateTwoSize_MultiLine(1, true, false, 10, 9, DisplayOuterbox, "%2.1FA2", battery_current_2);}
      UpdateTwoSize_MultiLine(1, true, false, 8, 8, DisplayOuterbox, " ");
      if (strstr(victronDevices.DisplayShow[i],"E"))  { UpdateTwoSize_MultiLine(1, false, false, 8, 8, DisplayOuterbox, "%s", DeviceStateToChar(device_state));}
      if (strstr(victronDevices.DisplayShow[i],"A"))  {UpdateTwoSize_MultiLine(1, true, false, 9, 8, DisplayOuterbox, "%2.0f deg", temperature - 273.15);}
      if (strstr(victronDevices.DisplayShow[i],"E"))  {  UpdateTwoSize_MultiLine(1, true, false, 8, 8, DisplayOuterbox, "%s", ErrorCodeToChar(charger_error));}
    }
  }
  Serial.println(VictronBuffer);
  victronDevices.displayed[i] = true;
}





void BLEloop() {
  // Serial.print(" BLE Scanning...");
  char debugMsg[121];
  static unsigned long BLESCANINTERVAL;
  if (millis() >= BLESCANINTERVAL) {
    BLESCANINTERVAL = millis() + _BLESCANINTERVAL;  //!
    FoundMyDevices = 0;
    
    snprintf(debugMsg, 120, "BLE Scan Commence");
    strcat(VictronBuffer, debugMsg);
    BLEScanResults foundDevices = pBLEScan->start(scanTime, false);  // what does the iscontinue do? (the true/false is set false in examples. )
   // Serial.printf("Found %i BLE and %i are myVictrons \n", foundDevices.getCount(), FoundMyDevices);
    pBLEScan->clearResults();  // delete results fromBLEScan buffer to release memory
     if (ColorSettings.Simulate) {// pull the simulate trigger on all listed in sequence ! 
        VictronSimulateIndex++;
        if (VictronSimulateIndex >= Num_Victron_Devices){VictronSimulateIndex=0;}
            victronDevices.displayed[VictronSimulateIndex] = false;
            victronDevices.greyed[VictronSimulateIndex] = false;
            victronDevices.updated[VictronSimulateIndex] = millis();
      }    
    snprintf(debugMsg, 120, "BLE Scan Finished \n");
    strcat(VictronBuffer, debugMsg);
  }

  for (int i = 0; i < Num_Victron_Devices; i++) {
      Deal_With_BLE_Data(i);
    }
}





char *ErrorCodeToChar(VE_REG_CHR_ERROR_CODE val) {
  static char Buff[100];
  switch (val) {
    case 0:  //VE_REG_CHR_ERROR_CODE::NO_ERROR:
      strcpy(Buff, "");
      break;
    case 1:  //VE_REG_CHR_ERROR_CODE::TEMPERATURE_BATTERY_HIGH:
      strcpy(Buff, "TEMP HIGH");
      break;
    case 2:  //VE_REG_CHR_ERROR_CODE::VOLTAGE_HIGH:
      strcpy(Buff, "VOLTAGE HIGH");
      break;
    case 3:  //VE_REG_CHR_ERROR_CODE::REMOTE_TEMPERATURE_A:
      strcpy(Buff, "T sensor FAIL");
      break;
    case 4:  // VE_REG_CHR_ERROR_CODE::REMOTE_TEMPERATURE_B:
      strcpy(Buff, "T sensor FAIL");
      break;
    case 5:  //VE_REG_CHR_ERROR_CODE::REMOTE_TEMPERATURE_C:
      strcpy(Buff, "T Conn lost");
      break;
    case 6:  // VE_REG_CHR_ERROR_CODE::REMOTE_BATTERY_A:
      strcpy(Buff, "V sense FAIL");
      break;
    case 7:  //VE_REG_CHR_ERROR_CODE::REMOTE_BATTERY_B:
      strcpy(Buff, "V sense FAIL");
      break;
    case 8:  //VE_REG_CHR_ERROR_CODE::REMOTE_BATTERY_C:
      strcpy(Buff, "conn. lost)");
      break;
    case 11:  //VE_REG_CHR_ERROR_CODE::HIGH_RIPPLE:
      strcpy(Buff, "High ripple V");
      break;
    case 14:
      strcpy(Buff, "LOW TEMP");
      break;

    case 17:  //VE_REG_CHR_ERROR_CODE::TEMPERATURE_CHARGER:
      strcpy(Buff, "OVERHEATED");
      break;
    case 18:  //VE_REG_CHR_ERROR_CODE::OVER_CURRENT:
      strcpy(Buff, "Over Current");
      break;
    case 19:  //VE_REG_CHR_ERROR_CODE::POLARITY:
      strcpy(Buff, "Polarity Reversed");
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
      strcpy(Buff, "Short circuit");
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
