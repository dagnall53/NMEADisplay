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

#include <aes/esp_aes.h>      // AES library for decrypting the Victron manufacturer data.
extern char VictronBuffer[];  // to get the data out as a string char
extern _sWiFi_settings_Config Current_Settings;
extern _sMyVictronDevices victronDevices;

BLEScan *pBLEScan;

_sButton TempButton = { 0, 0, 75, 75, 5, BLUE, WHITE, BLACK };  // will be used when I move button H V offsets
_sButton BatteryDisp = { 0, 0, 75, 75, 5, BLUE, WHITE, BLACK }; // for Victron battery display 


#define AES_KEY_BITS 128

int scanTime = 1;  // BLE scan time (seconds)
#define _BLESCANINTERVAL 10000   // run scan every 10 secs, (results in lockout of all other functions  for scanTime)
char savedDeviceName[32];  // cached copy of the device name (31 chars max) + \0

// Victron docs on the manufacturer data in advertisement packets can be found at:
//   https://community.victronenergy.com/storage/attachments/48745-extra-manufacturer-data-2022-12-14.pdf
//

// extra braces around each "designated initializer" element needed by some compiler versions.
// _sMyVictronDevices victronDevices[] = {
//   { { .charMacAddr = "ea9df3ebc625" }, { .charKey = "e09d8b200c61238c811a621e5964c44e" }, { .comment = "300A" } },
//   { { .charMacAddr = "f944913298e8" }, { .charKey = "40ef2093aa678238147091c7657daa54" }, { .comment = "dummy" } },
//   { { .charMacAddr = "cc5b284e8ae6" }, { .charKey = "2b6d51d4a74c3b83749303d87fa17bd9" }, { .comment = "dummy2" } }
// };
//is now Current_Settings.Num_Victron_Devices int  knownvictronDeviceCount = sizeof(victronDevices) / sizeof(victronDevices[0]);

int bestRSSI = -200;
int selectedvictronDeviceIndex = -1;

time_t lastLEDBlinkTime = 0;
time_t lastTick = 0;
int displayRotation = 3;
bool packetReceived = false;

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
  Serial.printf("  Hex convert %s  has length %i  \n",hexCharStr,hexCharStrLength);
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
// read https://github.com/hoberman/Victron_BLE_Scanner_Display/blob/main/BLE_Adv_Callback.ino for comments.
class MyAdvertisedDeviceCallbacks : public BLEAdvertisedDeviceCallbacks {

  void onResult(BLEAdvertisedDevice advertisedDevice) {
    char debugMsg[200];
#define manDataSizeMax 31  // BLE specs say no more than 31 bytes, but see comments below!
    // See if we have manufacturer data and then look to see if it's coming from a Victron device.
    if (advertisedDevice.haveManufacturerData() == true) {
      uint8_t manCharBuf[manDataSizeMax + 1];

#ifdef USE_String
      String manData = advertisedDevice.getManufacturerData();  // lib code returns String.
#else
      std::string manData = advertisedDevice.getManufacturerData();  // lib code returns std::string
#endif
      int manDataSize = manData.length();  // This does not include a null at the end.
      // Limit size just in case we get a malformed packet.
      if (manDataSize > manDataSizeMax) {
        Serial.printf("  Note: Truncating malformed %2d byte manufacturer data to max %d byte array size\n", manDataSize, manDataSizeMax);
        manDataSize = manDataSizeMax;
      }
// Now copy the data from the String to a byte array. Must have the +1 so we
// don't lose the last character to the null terminator. ?????
#ifdef USE_String
      //manData.toCharArray((char *)manCharBuf,manDataSize+1);
      memcpy(manCharBuf, manData.c_str(), manDataSize);
#else
      //manData.copy((char *)manCharBuf, manDataSize + 1);
      manData.copy((char *)manCharBuf, manDataSize);
#endif
      // Now let's use a struct to get to the data more cleanly.
              // Get the signal strength (RSSI) of the beacon.
        int RSSI = advertisedDevice.getRSSI();

      victronManufacturerData *vicData = (victronManufacturerData *)manCharBuf;
      // ignore this packet if the Vendor ID isn't Victron. https://gist.github.com/angorb/f92f76108b98bb0d81c74f60671e9c67
      if (vicData->vendorID != 0x02e1) {
     //  snprintf(debugMsg, 120, " unknown id:%#x Rssi:%i", vicData->vendorID,RSSI);strcat(VictronBuffer, debugMsg);
     //  Serial.println (debugMsg);
        return;
      }
      // IS mfr by Victrion.. it a KNOWN type? (code was based originally  only on Solar Charger
      // type 0x01 (Solar Charger). smartshunt is 0x10 according to AI on google! int 2 is the actual reported returned value
      char receivedMacStr[18];
      strcpy(receivedMacStr, advertisedDevice.getAddress().toString().c_str());

      
      // Get the MAC address of the device we're hearing, and then use that to look up the encryption key
      // for the device from our table
       snprintf(debugMsg, 120, "Found mac<%s> ",  receivedMacStr); strcat(VictronBuffer, debugMsg);Serial.print(debugMsg);

      int victronDeviceIndex = -1;
      for (int i = 0; i < Current_Settings.Num_Victron_Devices; i++) {
        if (CompareString_Mac(receivedMacStr, victronDevices.charMacAddr[i])) {
          victronDeviceIndex = i;
           }
      }  //loop of our known devices loop to set  victronDeviceIndex
      // // for record type list see line 300 on https://github.com/Fabian-Schmidt/esphome-victron_ble/blob/main/components/victron_ble/victron_ble.h


      int KnownDataType;
      KnownDataType = -1;
      if (vicData->victronRecordType == 0x01) { KnownDataType = 1; }  //Solar Charger
      if (vicData->victronRecordType == 0x02) { KnownDataType = 2; }     //Battery Monitr / smart shunt
       if (vicData->victronRecordType == 0x08) { KnownDataType = 2; }     //smart shunt
        snprintf(debugMsg, 120, "Indexmatched :<%i>type<%i> ", victronDeviceIndex, vicData->victronRecordType);
        strcat(VictronBuffer, debugMsg);
        char deviceName[32];  // 31 characters + \0
        // Get the device name (if there's one for this device in the index
        strcpy(deviceName, victronDevices.FileCommentName[victronDeviceIndex]);
        bool deviceNameFound = false;
        if (advertisedDevice.haveName()) {
          strcpy(deviceName, advertisedDevice.getName().c_str());  // This works the same whether getName() returns String or std::string.
          deviceNameFound = true;
        }
        // The manufacturer data from Victron contains a byte that's supposed to match the first byte
        // of the device's encryption key. If they don't match, when we don't have the right key for
        // }  also have HexStringToBytes to be tested... 

        Serial.printf("hex to byte  input %s \n",victronDevices.charKey[victronDeviceIndex]);
        byte localbyteKey[17];     
        hexCharStrToByteArray(victronDevices.charKey[victronDeviceIndex], localbyteKey);
        
        Serial.printf("    byteKey: ");
         for (int j = 0; j < 16; j++) {
           Serial.printf("%2.2x", localbyteKey[j]);
        }
         Serial.println();

        if (vicData->encryptKeyMatch != localbyteKey[0]) {  //is or was!! working  byteKey
          snprintf(debugMsg, 120, "BUT key is MIS-MATCHED\n" );
          Serial.println(debugMsg);
          strcat(VictronBuffer, debugMsg);
          return;
        }

        // Now that the packet received has met all the criteria for being displayed, let's decrypt and decode the manufacturer data.
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
                                                     // or localbyteKey ?
        auto status = esp_aes_setkey(&ctx, localbyteKey, AES_KEY_BITS);
        if (status != 0) {
          Serial.printf("  Error during esp_aes_setkey operation (%i).\n", status);
          esp_aes_free(&ctx);
          return;
        }

        byte data_counter_lsb = (vicData->nonceDataCounter) & 0xff;
        byte data_counter_msb = ((vicData->nonceDataCounter) >> 8) & 0xff;
        u_int8_t nonce_counter[16] = { data_counter_lsb, data_counter_msb, 0 };
        u_int8_t stream_block[16] = { 0 };

        size_t nonce_offset = 0;
        status = esp_aes_crypt_ctr(&ctx, encrDataSize, &nonce_offset, nonce_counter, stream_block, inputData, outputData);
        if (status != 0) {
          Serial.printf("Error during esp_aes_crypt_ctr operation (%i).", status);
          esp_aes_free(&ctx);
          return;
        }
        esp_aes_free(&ctx);



        if (KnownDataType == 1) {
          VICTRON_BLE_RECORD_SOLAR_CHARGER *victronData = (VICTRON_BLE_RECORD_SOLAR_CHARGER *)outputData;

          byte device_state = victronData->device_state;  // this is really more like "Charger State"
          byte charger_error = victronData->charger_error;
          float battery_voltage = float(victronData->battery_voltage) * 0.01;  
          float battery_current = float(victronData->battery_current) * 0.1;   
          float yield_today = float(victronData->yield_today) * 0.01 * 1000; //we use wattHr not kwHr.. 
          uint16_t pv_power = victronData->pv_power;
          float load_current= float(victronData->load_current)* 0.1;
          char chargeStateName[6];
          sprintf(chargeStateName, "%4d?", device_state);
          if (device_state >= 0 && device_state <= 7) { strcpy(chargeStateName, chargeStateNames[device_state]); }
          snprintf(debugMsg, 120, "<%-31s> <%s> Batt:%6.2f Volts %6.2f Amps\n %6d Watts  Yield:%4.0f Wh  \nLoad: %5.1fA  Charger:%-13s Err: %2d RSSI:%d\n",
                   victronDevices.FileCommentName[victronDeviceIndex],deviceName,
                   battery_voltage, battery_current,
                   pv_power, yield_today,
                   load_current, chargeStateName, charger_error, RSSI);
          Serial.println(debugMsg);
         // strcat(VictronBuffer, debugMsg);
        }
        if (KnownDataType == 2) {
          VICTRON_BLE_RECORD_BATTERY_MONITOR *victronData = (VICTRON_BLE_RECORD_BATTERY_MONITOR *)outputData;
          float battery_voltage = float(victronData->battery_voltage) * 0.01;
          float battery_current = float(victronData->battery_current) * 0.001;  //4194
          int auxtype = victronData->aux_input_type;
          float AuxInput = float(victronData->aux_input) * 0.01;  // starter battery for Shunt
          float stateofcharge = float(victronData->state_of_charge) * 0.1;
          snprintf(debugMsg, 120, "<%s> <%s> %2.3Fv  %2.3FA  Aux<%i> is %2.2Fv SOC:%2.1Fpercent  RSSI:%d\n",
                   victronDevices.FileCommentName[victronDeviceIndex],deviceName, battery_voltage, battery_current, auxtype, AuxInput, stateofcharge, RSSI);
          Serial.println(debugMsg);
          /// test, can I use GFXBorderBoxPrintf directly here? 
          GFXBorderBoxPrintf(BatteryDisp, "BATTERY");

         // strcat(VictronBuffer, debugMsg);
        }
      // any more to add to the debug message in the VictronBuffer?
      strcat(VictronBuffer, debugMsg);

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
  Serial.println(F(" BLE setup() complete."));
}
void BLEloop() {
  // Serial.print(" BLE Scanning...");
  static unsigned long BLESCANINTERVAL;
  if (millis() >= BLESCANINTERVAL) {
    BLESCANINTERVAL = millis() + _BLESCANINTERVAL;                              //!
    BLEScanResults foundDevices = pBLEScan->start(scanTime, false);  // what does the iscontinue do? (the true/false is set false in examples. )
    pBLEScan->clearResults();                                        // delete results fromBLEScan buffer to release memory
  }
}
