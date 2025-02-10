
#include "ESP_NOW_files.h"
#include <esp_now.h>

//byte peerAddress[6];
//const byte peerAddress_def[] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };  // all receive
esp_now_peer_info_t peerInfo;
//bool EspNowIsRunning = false;
byte peerAddress[6];
const byte peerAddress_def[] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };  // all receive
unsigned long Last_EXT_Sent; // used to time a heartbeat 


bool Start_ESP_EXT() {  // start espnow and run Test_EspNOW() when data is seen / sent
  bool success = false;
  EspNowIsRunning = false;
  memcpy(peerInfo.peer_addr, peerAddress_def, 6);
  peerInfo.encrypt = false;
  peerInfo.channel = 0;
  if (esp_now_init() == ESP_OK) { EspNowIsRunning = true; }
  esp_now_register_recv_cb(Test_EspNOW);
  if (esp_now_add_peer(&peerInfo) == ESP_OK) { success = true; }
  return success;
}


//EXT send functions "esp_now"
void Test_EspNOW(const uint8_t* mac, const uint8_t* incomingData, int len) {
  EspNowIsRunning = true;
  //if (!line_EXT) { // wait until previous capture has been processed?
  memcpy(&nmea_EXT, incomingData, sizeof(nmea_EXT));
  //Serial.printf( "esp_now INTERRUPT :%s",nmea_EXT);
  line_EXT = true;  //}
}
void EXTHeartbeat() {
  if (!EspNowIsRunning) { return; }
  if (Last_EXT_Sent + 10000 <= millis()) {  // 10 sec.
  //Serial.println("Sending heartbeat");
    Last_EXT_Sent = millis();               // but we also update Last_EXT_Sent in the EXT send..
                                            //EXTSENDf("_%s_:ch%d_\r\n", Project, WiFi.channel());  
                                            // BUT We must send with a \r\n!!
    EXTSENDf("_NMEA Disp_ Heartbeat_\r\n");  // if using in library does not have access to wifi project etc  data
  }
}
void EXTSEND(const char* buf) {  // same format as TCP send etc..  83 size for NMEA 250 max for ESP-NOW link  NOT BIG ENOUGH FOR SOME RAW!!
  if (!EspNowIsRunning) { return; }
  char myData[248];                       // max size of esp_now_send messages
  strcpy(myData, buf);                    // to avoid overflowing in this function..?
  strcat(myData, "\r\n");                 // Adding CRLF wass eqivalent of Println.. and we now "print" everywhere
  if (strlen(myData) >= 247) { return; }  // // sendAdvice("Message too big for ESP_EXT link");}but test before going further!
  if (strlen(myData) <= 5) { return; }    // should never be less than 5 !! Just to try to catch extra CRLF's ?
  // sendAdvicef(" esp-now Sending: (%s) Wifich<%i>  peer_ch<%i> ", myData, WiFi.channel(), peerInfo.channel);
  //sendAdvicef(" esp-now Sending: (length %i) Wifich<%i>  peer_ch<%i> ", strlen(myData), WiFi.channel(), peerInfo.channel);
  Last_EXT_Sent = millis();
  esp_err_t result = esp_now_send(peerInfo.peer_addr, (uint8_t*)&myData, sizeof(myData));
  //if (result == ESP_OK) {Serial.println(" esp-now SENT");}// <%s> (%i long) in msg max [%i] long \n",myData,strlen(myData), sizeof(myData));}else{Serial.println(" ESP failed to send");}
  //if (result != ESP_OK) {EspNowIsRunning=false;}   // but then we need something in Loop to re-establish the link ??
}

void EXTSENDf(const char* fmt, ...) {  //complete object type suitable for holding the information needed by the macros va_start, va_copy, va_arg, and va_end.
  if (!EspNowIsRunning) { return; }
  static char msg[300] = { '\0' };  // used in message buildup
  va_list args;
  va_start(args, fmt);
  vsnprintf(msg, 128, fmt, args);
  va_end(args);
  // add checksum?
  int len = strlen(msg);
  EXTSEND(msg);
}
