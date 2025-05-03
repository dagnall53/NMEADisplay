
#ifndef Victronble_h
#define Victronble_h
#include <Arduino.h> // Include Arduino library

#include <BLEDevice.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>

// onResult includes serial print of results but will be changed once I understand it..
// class MyAdvertisedDeviceCallbacks : public BLEAdvertisedDeviceCallbacks 
//   {public:
//   void onResult(BLEAdvertisedDevice advertisedDevice);
//   };

// victron ble structures

// explore VICTRON_SENSOR_TYPE::BATTERY_VOLTAGE: in C:\Users\admin\Downloads\esphome-victron_ble-main (1)\esphome-victron_ble-main\components\victron_ble\sensor\victron_sensor.cpp
/* 
see: 
C:\Users\admin\Downloads\esphome-victron_ble-main (1)\esphome-victron_ble-main\components\victron_ble\victron_ble.h  
struct VICTRON_BLE_RECORD_BATTERY_MONITOR {  // NOLINT(readability-identifier-naming,altera-struct-pack-align)
  vic_16bit_1_positive time_to_go;
  vic_16bit_0_01 battery_voltage;
  VE_REG_ALARM_REASON alarm_reason;
  union {
    vic_16bit_0_01 aux_voltage;
    vic_16bit_0_01_positive mid_voltage;
    vic_temperature_16bit temperature;
  } aux_input;
  VE_REG_BMV_AUX_INPUT aux_input_type : 2;
  vic_22bit_0_001 battery_current : 22; //
  vic_20bit_0_1_negative consumed_ah : 20;
  vic_10bit_0_1_positive state_of_charge : 10;
} __attribute__((packed));



for enums see C:\Users\admin\Downloads\esphome-victron_ble-main (1)\esphome-victron_ble-main\components\victron_ble\victron_custom_type.h

struct VICTRON_BLE_RECORD_SOLAR_CHARGER {  // NOLINT(readability-identifier-naming,altera-struct-pack-align)
  VE_REG_DEVICE_STATE device_state;
  VE_REG_CHR_ERROR_CODE charger_error;
  vic_16bit_0_01 battery_voltage;  //// 0.01 V, -327.68 .. 327.66 V enum vic_16bit_0_01 : int16_t;
  vic_16bit_0_1 battery_current;   //// 0.1 A, -3276.8 .. 3276.6 A enum vic_16bit_0_1 : int16_t;
  vic_16bit_0_01_positive yield_today;
  vic_16bit_1_positive pv_power;   //u_int16_t;
  vic_9bit_0_1_negative load_current : 9;  //// 0.1 A, 0 .. 51.0 A enum vic_9bit_0_1_negative : u_int16_t;
} __attribute__((packed));
*/
typedef struct {                      
  u_int16_t time_to_go;
  int16_t battery_voltage;
  int16_t alarm_reason;
  u_int16_t aux_input;   /*union { vic_16bit_0_01 aux_voltage; vic_16bit_0_01_positive mid_voltage; vic_temperature_16bit temperature;} aux_input; */
  u_int8_t aux_input_type : 2;
  int32_t battery_current : 22;
  u_int32_t consumed_ah : 20;
  u_int16_t state_of_charge : 10;
} __attribute__((packed)) VICTRON_BLE_RECORD_BATTERY_MONITOR;




typedef struct {                       // see C:\Users\admin\Downloads\esphome-victron_ble-main (1)\esphome-victron_ble-main\components\victron_ble\sensor\victron_sensor.cpp
  uint8_t deviceState;
  uint8_t errorCode;
  int16_t batteryVoltage;
  int16_t batteryCurrent;
  uint16_t todayYield;              // starter battery on Smart shunt
  uint16_t inputPower;              // Load current 2's complement on Smart Shunt
  uint8_t outputCurrentLo;  // Low 8 bits of output current (in 0.1 Amp increments)
  uint8_t outputCurrentHi;  // High 1 bit of ourput current (must mask off unused bits)
  uint8_t unused[4];
} __attribute__((packed)) VICTRON_BLE_RECORD_SOLAR_CHARGER;

typedef struct {
  uint16_t vendorID;                 // vendor ID
  uint8_t beaconType;                // Should be 0x10 (Product Advertisement) for the packets we want
  uint8_t unknownData1[3];           // Unknown data
  uint8_t victronRecordType;         // Should be 0x01 (Solar Charger) for the packets we want
  uint16_t nonceDataCounter;         // Nonce
  uint8_t encryptKeyMatch;           // Should match pre-shared encryption key byte 0
  uint8_t victronEncryptedData[21];  // (31 bytes max per BLE spec - size of previous elements)
  uint8_t nullPad;                   // extra byte because toCharArray() adds a \0 byte.
} __attribute__((packed)) victronManufacturerData;


void hexCharStrToByteArray(char * hexCharStr, byte * byteArray); // called in LoadVictronConfiguration to set byte version of the input string 

void BLEsetup();  // called from main void setup();
void BLEloop();   // called from main void loop() 

#endif