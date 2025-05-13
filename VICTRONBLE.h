
#ifndef Victronble_h
#define Victronble_h
#include <Arduino.h> // Include Arduino library

#include <BLEDevice.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>
// read https://github.com/hoberman/Victron_BLE_Scanner_Display/blob/main/BLE_Adv_Callback.ino for comments.

// for record type list see line 300 on https://github.com/Fabian-Schmidt/esphome-victron_ble/blob/main/components/victron_ble/victron_ble.h
// for structures see(circa line 600..) https://github.com/Fabian-Schmidt/esphome-victron_ble/blob/main/components/victron_ble/victron_ble.h
// for data types and ranges see https://github.com/Fabian-Schmidt/esphome-victron_ble/blob/main/components/victron_ble/victron_custom_type.h
// types include range data etc, to allow range setting 
// Later edit.. I have added typedef aliases  to allow for (nearly) direct copy transcription.. changing by hand is tedious..! 


// victron ble structures
typedef uint8_t VE_REG_CHR_ERROR_CODE;
typedef uint8_t VE_REG_DEVICE_STATE;
typedef u_int16_t vic_16bit_1_positive; // 1 W, 0 .. 65534 W // 1 min, 0 .. 65534 min (~1092 hours, ~45.5 days)// 1 VA, 0 .. 65534 VA
typedef int16_t vic_16bit_0_1; // 0.1 A, -3276.8 .. 3276.6 A
typedef int16_t vic_16bit_0_01;// 0.01 V, -327.68 .. 327.66 V
typedef u_int16_t vic_16bit_0_01_positive; // 0.01 V, 0 .. 655.34 V //// 0.01 kWh, 0 .. 655.34 kWh
typedef u_int16_t vic_15bit_0_01_positive;
typedef u_int16_t vic_13bit_0_01_positive;  //// 0.01 V, 0 .. 81.90 V
typedef u_int16_t vic_12bit_0_01_positive;
typedef u_int16_t vic_11bit_0_1_positive; // // 0.1 A, 0 .. 204.6 A
typedef u_int16_t vic_9bit_0_1_positive;  //// 0.1 A, 0 .. 51.0 A
typedef u_int16_t vic_9bit_0_1_negative; //// 0.1 A, 0 .. 51.0 A 
typedef uint8_t vic_cell_7bit_0_01; 
typedef u_int16_t vic_temperature_7bit;   /// 1 °C, -40 .. 86 °C - Temperature = Record value - 40
typedef uint8_t VE_REG_BALANCER_STATUS;
typedef int16_t vic_16bit_1;
typedef int16_t vic_16bit_0_01_noNAN;
typedef u_int32_t VE_REG_DEVICE_OFF_REASON_2;
typedef u_int32_t VE_REG_BMS_FLAGs;
typedef u_int16_t VE_REG_ALARM_REASON;

/*
enum class VICTRON_BLE_RECORD_TYPE : u_int8_t {
  // VICTRON_BLE_RECORD_TEST
  TEST_RECORD = 0x00,
  // VICTRON_BLE_RECORD_SOLAR_CHARGER
  SOLAR_CHARGER = 0x01,
  // VICTRON_BLE_RECORD_BATTERY_MONITOR
  BATTERY_MONITOR = 0x02,
  // VICTRON_BLE_RECORD_INVERTER
  INVERTER = 0x03,
  // VICTRON_BLE_RECORD_DCDC_CONVERTER
  DCDC_CONVERTER = 0x04,
  // VICTRON_BLE_RECORD_SMART_LITHIUM
  SMART_LITHIUM = 0x05,
  // VICTRON_BLE_RECORD_INVERTER_RS
  INVERTER_RS = 0x06,
  // Not defined
  GX_DEVICE = 0x07,
  // VICTRON_BLE_RECORD_AC_CHARGER
  AC_CHARGER = 0x08,
  // VICTRON_BLE_RECORD_SMART_BATTERY_PROTECT
  SMART_BATTERY_PROTECT = 0x09,
  // VICTRON_BLE_RECORD_LYNX_SMART_BMS
  LYNX_SMART_BMS = 0x0A,
  // VICTRON_BLE_RECORD_MULTI_RS
  MULTI_RS = 0x0B,
  // VICTRON_BLE_RECORD_VE_BUS
  VE_BUS = 0x0C,
  // VICTRON_BLE_RECORD_DC_ENERGY_METER
  DC_ENERGY_METER = 0x0D,
  // VICTRON_BLE_RECORD_ORION_XS
  ORION_XS = 0x0F,
  
  */




typedef struct  {  // struct from Fabian Schmidt NOLINT(readability-identifier-naming,altera-struct-pack-align)
  uint8_t device_state;
  uint8_t charger_error;
  int16_t battery_voltage; // 0.01 V, -327.68 .. 327.66 V enum vic_16bit_0_01 : int16_t;
  int16_t battery_current; // 0.1 A, -3276.8 .. 3276.6 A enum vic_16bit_0_1 : int16_t;
  u_int16_t yield_today;  //// 0.01 kWh, 0 .. 655.34 kWh
  u_int16_t pv_power;
  vic_9bit_0_1_negative load_current : 9; //// 0.1 A, 0 .. 51.0 A // just this one with the original type to test I can use extra typdef!!
} __attribute__((packed)) VICTRON_BLE_RECORD_SOLAR_CHARGER; //01

typedef struct  {                      
  u_int16_t time_to_go;
  int16_t battery_voltage;
  int16_t alarm_reason;
  u_int16_t aux_input;   /*union { vic_16bit_0_01 aux_voltage; vic_16bit_0_01_positive mid_voltage; vic_temperature_16bit temperature;} aux_input; */
  u_int8_t aux_input_type : 2;
  int32_t battery_current : 22;
  u_int32_t consumed_ah : 20;
  u_int16_t state_of_charge : 10;
} __attribute__((packed))VICTRON_BLE_RECORD_BATTERY_MONITOR; //02

typedef struct  {  // NOLINT(readability-identifier-naming,altera-struct-pack-align)
  VE_REG_DEVICE_STATE device_state;
  VE_REG_ALARM_REASON alarm_reason;
  vic_16bit_0_01 battery_voltage;
  vic_16bit_1_positive ac_apparent_power;
  vic_15bit_0_01_positive ac_voltage : 15;
  vic_11bit_0_1_positive ac_current : 11;
} __attribute__((packed))VICTRON_BLE_RECORD_INVERTER; //03


typedef struct  {  // NOLINT(readability-identifier-naming,altera-struct-pack-align)
  VE_REG_DEVICE_STATE device_state;
  VE_REG_CHR_ERROR_CODE charger_error;
  vic_16bit_0_01_positive input_voltage;
  vic_16bit_0_01_noNAN output_voltage;
  VE_REG_DEVICE_OFF_REASON_2 off_reason;
} __attribute__((packed))VICTRON_BLE_RECORD_DCDC_CONVERTER;   ///04


typedef struct  {  // NOLINT(readability-identifier-naming,altera-struct-pack-align)
  VE_REG_DEVICE_STATE device_state;
  VE_REG_CHR_ERROR_CODE charger_error;
  vic_16bit_0_01 battery_voltage;
  vic_16bit_0_1 battery_current;
  vic_16bit_1_positive pv_power;
  vic_16bit_0_01_positive yield_today;
  vic_16bit_1 ac_out_power;
} __attribute__((packed))VICTRON_BLE_RECORD_INVERTER_RS; //06

// Battery causes compile messages I can do without for now. Not using this yet anyway 
// typedef struct  {  // NOLINT(readability-identifier-naming,altera-struct-pack-align)
//   VE_REG_BMS_FLAGs bms_flags;
//   u_int16_t SmartLithium_error;
//   vic_cell_7bit_0_01 cell1 : 7;
//   vic_cell_7bit_0_01 cell2 : 7;
//   vic_cell_7bit_0_01 cell3 : 7;
//   vic_cell_7bit_0_01 cell4 : 7;
//   vic_cell_7bit_0_01 cell5 : 7;
//   vic_cell_7bit_0_01 cell6 : 7;
//   vic_cell_7bit_0_01 cell7 : 7;
//   vic_cell_7bit_0_01 cell8 : 7;
//   vic_12bit_0_01_positive battery_voltage : 12;
//   // 0 .. 15
//   VE_REG_BALANCER_STATUS balancer_status : 4;
//   vic_temperature_7bit battery_temperature : 7;
// } __attribute__((packed))VICTRON_BLE_RECORD_SMART_LITHIUM; // 05

// See also <https://github.com/Fabian-Schmidt/esphome-victron_ble/issues/62>
typedef struct  {  // NOLINT(readability-identifier-naming,altera-struct-pack-align)
  VE_REG_DEVICE_STATE device_state;
  VE_REG_CHR_ERROR_CODE charger_error;
  vic_13bit_0_01_positive battery_voltage_1 : 13;
  vic_11bit_0_1_positive battery_current_1 : 11;
  vic_13bit_0_01_positive battery_voltage_2 : 13;
  vic_11bit_0_1_positive battery_current_2 : 11;
  vic_13bit_0_01_positive battery_voltage_3 : 13;
  vic_11bit_0_1_positive battery_current_3 : 11;
  vic_temperature_7bit temperature : 7;
  vic_9bit_0_1_positive ac_current : 9;
} __attribute__((packed))VICTRON_BLE_RECORD_AC_CHARGER; //VICTRON_BLE_RECORD_AC_CHARGER= 08



/*  trying to rebuild the victronManufacturerData based on Fabian Schmittdata:

HObermann original was 

              // Must use the "packed" attribute to make sure the compiler doesn't add any padding to deal with
              // word alignment.
typedef struct {
  uint16_t vendorID;                    // vendor ID
  uint8_t beaconType;                   // Should be 0x10 (Product Advertisement) for the packets we want
  uint8_t unknownData1[3];              // (24 bits of ?)  3 elements of Unknown 8 bit data 
  uint8_t victronRecordType;            // Should be 0x01 (Solar Charger) for the packets we want
  uint16_t nonceDataCounter;            // Nonce
  uint8_t encryptKeyMatch;              // Should match pre-shared encryption key byte 0
  uint8_t victronEncryptedData[21];     // (31 bytes max per BLE spec - size of previous elements)
  uint8_t nullPad;                      // extra byte because toCharArray() adds a \0 byte.
} __attribute__((packed)) victronManufacturerData;
*/

// fabian/schmitt derived version:  


typedef struct {
 uint16_t  vendorID; // properly manufacturer_record_type? ; //VICTRON_MANUFACTURER_RECORD_TYPE  
 uint8_t beaconType;  //PRODUCT_ADVERTISEMENT = 0x10,
 // ok to here..
 u_int8_t manufacturer_record_length;
 uint16_t product_id; // ??? device. VICTRON_PRODUCT_ID ?? the actual device type = BlueSolar. A042..A04f  SmartSolar A050..A065 
 // and ok from here! 
 uint8_t victronRecordType;//aka record_type VICTRON_BLE_RECORD_TYPE 00 = solar charger 01 = Smartshunt and battery monitor .. etc ()
 uint16_t nonceDataCounter; // deal with this later, it seems to work and Hoben reconverts to lsb/msb??
                            // u_int8_t data_counter_lsb;
                            //u_int8_t data_counter_msb;
 uint8_t encryption_key_0; // Byte 0 of the encryption key (bindkey)
 uint8_t victronEncryptedData[21]; // not modifying as proven to work,
 uint8_t nullPad;
} __attribute__((packed)) victronManufacturerData;

void hexCharStrToByteArray(char * hexCharStr, byte * byteArray); // called in LoadVictronConfiguration to set byte version of the input string 

void BLEsetup();  // called from main void setup();
void BLEloop();   // called from main void loop() 

#endif