//
// Created by alvaro on 26/07/15.
//

#include "EEPROMConfig.h"

EEPROMConfig::EEPROMConfig(){
    //totalDevices = 0;
}

unsigned long EEPROMConfig::configCrc(size_t size) {

  const unsigned long crc_table[16] = {
    0x00000000, 0x1db71064, 0x3b6e20c8, 0x26d930ac,
    0x76dc4190, 0x6b6b51f4, 0x4db26158, 0x5005713c,
    0xedb88320, 0xf00f9344, 0xd6d6a3e8, 0xcb61b38c,
    0x9b64c2b0, 0x86d3d2d4, 0xa00ae278, 0xbdbdf21c
  };

  unsigned long crc = ~0L;

  for (unsigned int index = 0 ; index < size  ; ++index) {
    crc = crc_table[(crc ^ EEPROM[index]) & 0x0f] ^ (crc >> 4);
    crc = crc_table[(crc ^ (EEPROM[index] >> 4)) & 0x0f] ^ (crc >> 4);
    crc = ~crc;
  }
  return crc;
}

bool EEPROMConfig::loadConfig(){

  // Do not CRC the CRC
  int dataSize = sizeof(ConfigurationInfo)-sizeof(unsigned long);
  // Get the data CRC
  unsigned long crc = configCrc(dataSize);
  // Load the whole object
  for (unsigned int t=0; t<sizeof(ConfigurationInfo); t++)
        *((char*)&conf + t) = EEPROM[t];
  return crc == conf.crc;
}

void EEPROMConfig::writeConfig(struct ConfigurationInfo newConf) {
  // Do not CRC the CRC
  unsigned int dataSize = sizeof(ConfigurationInfo)-sizeof(unsigned long);
  // Write only data
  for (unsigned int t=0; t<dataSize; t++)
    EEPROM[t] = *((char*)&newConf + t);

  // Get data CRC
  newConf.crc = configCrc(dataSize);
  // Write the CRC
  for (unsigned int t=dataSize; t<sizeof(ConfigurationInfo); t++)
    EEPROM[t] = *((char*)&newConf + t);

  //Copy new config to actual config
  for (unsigned int t=0; t<sizeof(ConfigurationInfo); t++)
    *((char*)&conf + t) = *((char*)&newConf + t);
}

int EEPROMConfig::loadSensors(bool (*loader)(char *name, byte *data, size_t dataSize)) {
  char sensorType[50];
  int totalLoaded = 0;
  byte data[255];
  int dataPosition = sizeof(ConfigurationInfo);

  int offset = 0;
  for(unsigned int i = 0; i < conf.totalSensors; i++) {

    //Read sensor type
    while(EEPROM[dataPosition + offset] != 0 && offset < 50) {
        sensorType[offset] = EEPROM[dataPosition + offset];
        offset++;
    }

    if(offset == 50) {
      Serial.println("MAX NAME");
    } else {
      // End sensorType string
      sensorType[offset] = 0;
      offset++;
      // Load the data size which is the next element

      int dataSize = EEPROM[dataPosition + offset];
      offset++;
      // Now read the data for initialize the sensor
      for (int i=0; i <dataSize; i++){
        data[i] = EEPROM[dataPosition + offset + i];
      }
      offset += dataSize;
      // At this point we have the sensor type and the parameters to initialize this sensor
      if(loader(sensorType, data, dataSize)){
        // The loader could load the sensor
        totalLoaded++;
      }
    }
  }

  return totalLoaded;
}

void EEPROMConfig::writeSensorConfig(byte *data, size_t size) {
  int offset = sizeof(ConfigurationInfo);
  for (unsigned int t=0; t<size; t++){
    EEPROM[t + offset] = *(data + t);
  }
}

void EEPROMConfig::writeSensorConfig(Stream *source, size_t size) {
  int offset = sizeof(ConfigurationInfo);
  for(size_t i = 0; i < size; i++){
    while(!source->available());
    EEPROM[i + offset] = source->read();
  }
  /*Serial.println("Escribiendo en:");
  int offset = sizeof(ConfigurationInfo);
  for (unsigned int t=0; t<size; t++){
    EEPROM[t + offset] = *(data + t);
  }*/
}
