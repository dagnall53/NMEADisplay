#ifndef _AUX_FUN_H_
#define _AUX_FUN_H_
//int parseNMEA0183( String sentence, String data[]);
// void initFS();
// void storeString(String path, String content);
// String retrieveString(String path);
// String convertGPString(String input) ;
// String int2string(int number);
// String readStoredData( char* filename);
// String updateStoredData(char* filename, int newValue);
bool processPacket(const char* buf,  tBoatData &stringBD);

#endif