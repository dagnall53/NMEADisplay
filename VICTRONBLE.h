
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

// for record type list see line 300 on https://github.com/Fabian-Schmidt/esphome-victron_ble/blob/main/components/victron_ble/victron_ble.h
// for structures see(circa line 600..) https://github.com/Fabian-Schmidt/esphome-victron_ble/blob/main/components/victron_ble/victron_ble.h
// for data types and ranges see https://github.com/Fabian-Schmidt/esphome-victron_ble/blob/main/components/victron_ble/victron_custom_type.h
// types include range data etc, to allow range setting 
// Later edit.. I have added typedef aliases  to allow for (nearly) direct copy transcription.. changing by hand is tedious..! 
/*


enum vic_16bit_0_01_noNAN : int16_t;

// 0.01 kWh, 0 .. 655.34 kWh
// 0.01 V, 0 .. 655.34 V

*/
typedef uint8_t VE_REG_CHR_ERROR_CODE;
typedef uint8_t VE_REG_DEVICE_STATE;
typedef u_int16_t vic_16bit_1_positive; // 1 W, 0 .. 65534 W // 1 min, 0 .. 65534 min (~1092 hours, ~45.5 days)// 1 VA, 0 .. 65534 VA
typedef int16_t vic_16bit_0_1; // 0.1 A, -3276.8 .. 3276.6 A
typedef int16_t vic_16bit_0_01;// 0.01 V, -327.68 .. 327.66 V
typedef u_int16_t vic_16bit_0_01_positive; // 0.01 V, 0 .. 655.34 V //// 0.01 kWh, 0 .. 655.34 kWh
typedef u_int16_t vic_13bit_0_01_positive;  //// 0.01 V, 0 .. 81.90 V
typedef u_int16_t vic_11bit_0_1_positive; // // 0.1 A, 0 .. 204.6 A
typedef u_int16_t vic_temperature_7bit;   /// 1 °C, -40 .. 86 °C - Temperature = Record value - 40
typedef u_int16_t vic_9bit_0_1_positive;  //// 0.1 A, 0 .. 51.0 A
typedef u_int16_t vic_9bit_0_1_negative; //// 0.1 A, 0 .. 51.0 A 

//https://stackoverflow.com/questions/44990068/using-c-typedef-using-type-alias
//using vic_9bit_0_1_negative = u_int16_t;

typedef struct  {  // NOLINT(readability-identifier-naming,altera-struct-pack-align)
  uint8_t device_state;
  uint8_t charger_error;;
  vic_13bit_0_01_positive battery_voltage_1 : 13;
  vic_11bit_0_1_positive battery_current_1 : 11;
  vic_13bit_0_01_positive battery_voltage_2 : 13;
  vic_11bit_0_1_positive battery_current_2 : 11;
  vic_13bit_0_01_positive battery_voltage_3 : 13;
  vic_11bit_0_1_positive battery_current_3 : 11;
  vic_temperature_7bit temperature : 7;
  vic_9bit_0_1_positive ac_current : 9;
} __attribute__((packed))VICTRON_BLE_RECORD_AC_CHARGER;

typedef struct  {                      
  u_int16_t time_to_go;
  int16_t battery_voltage;
  int16_t alarm_reason;
  u_int16_t aux_input;   /*union { vic_16bit_0_01 aux_voltage; vic_16bit_0_01_positive mid_voltage; vic_temperature_16bit temperature;} aux_input; */
  u_int8_t aux_input_type : 2;
  int32_t battery_current : 22;
  u_int32_t consumed_ah : 20;
  u_int16_t state_of_charge : 10;
} __attribute__((packed))VICTRON_BLE_RECORD_BATTERY_MONITOR;

typedef struct  {  // struct from Fabian Schmidt NOLINT(readability-identifier-naming,altera-struct-pack-align)
  uint8_t device_state;
  uint8_t charger_error;
  int16_t battery_voltage; // 0.01 V, -327.68 .. 327.66 V enum vic_16bit_0_01 : int16_t;
  int16_t battery_current; // 0.1 A, -3276.8 .. 3276.6 A enum vic_16bit_0_1 : int16_t;
  u_int16_t yield_today;  //// 0.01 kWh, 0 .. 655.34 kWh
  u_int16_t pv_power;
  vic_9bit_0_1_negative load_current : 9; //// 0.1 A, 0 .. 51.0 A // just this one with the original type to test I can use extra typdef!!
} __attribute__((packed)) VICTRON_BLE_RECORD_SOLAR_CHARGER;


typedef struct {
  uint16_t vendorID;                 // vendor ID
  uint8_t beaconType;                // Should be 0x10 (Product Advertisement) for the packets we want
  uint8_t unknownData1[3];           // Unknown data
  uint8_t victronRecordType;         // will be eg 0x01 (Solar Charger) 
  uint16_t nonceDataCounter;         // Nonce
  uint8_t encryptKeyMatch;           // Should match pre-shared encryption key byte 0
  uint8_t victronEncryptedData[21];  // (31 bytes max per BLE spec - size of previous elements)
  uint8_t nullPad;                   // extra byte because toCharArray() adds a \0 byte.
} __attribute__((packed)) victronManufacturerData;


void hexCharStrToByteArray(char * hexCharStr, byte * byteArray); // called in LoadVictronConfiguration to set byte version of the input string 

void BLEsetup();  // called from main void setup();
void BLEloop();   // called from main void loop() 

#endif