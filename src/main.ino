#include "JsonWriter.h"
#include <ArduinoJson.h>
#include <MemoryFree.h>
#include <avr/pgmspace.h>

/* Sensors dependencies */
#include "Configuration.h"
#include <OneWire.h>
#include <DallasTemperature.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <DHT_U.h>
#include <VoltMeter.h>
#include <Multiplexer.h>
#include <SensorMonitor.h>
#include "EEPROMConfig.h"


/************************/

/* Simple json message recognizer */
#define JSON_READ_BUFFER 100
JsonWriter serialJsonWriter(&Serial);

SensorMonitor m;
bool reset = false;

void softwareReset(){
  asm volatile ("jmp 0");
}

struct Command {
    char  command[20];
    void (*fun)(JsonObject *input, JsonWriter *output);
} command;



void commandStatus(JsonObject *input, JsonWriter *output){
    // Display sensor information
    output->
    beginObject("data")
      .property("status", "ok")
      .beginObject("memory")
        .property("available", freeMemory())
      .endObject()
    .endObject();
}

void writeConfig(JsonObject *input, JsonWriter *output){
  int dataSize = (*input)["dataSize"];
  int totalSensors = (*input)["totalSensors"];
  EEPROMConfig config;
  struct ConfigurationInfo conf = config.conf;
  conf.totalSensors = totalSensors;
  reset = true;
  config.writeConfig(conf);
  config.writeSensorConfig(&Serial, dataSize);

    // Display sensor information
    output->
    beginObject("data")
      .property("status", "ok")
      .property("read", dataSize)
    .endObject();
}

void commandRead(JsonObject *input, JsonWriter *output){
    float multipleValues[10];
    int quantities[10];
    char quantityName[20];
    Device *device;
    output->
    beginArray("data");
    for (int i = 0; i < m.getTotalDevices(); i++) {
        device = m.getDevice(i);
        device->getSensorQuantities(quantities);
        if (device->multipleValues()) {
          int totalValues = device->getValues(multipleValues);
          for (int j = 0; j < totalValues; j++) {
            strcpy_P(quantityName, (char*)pgm_read_word(&(quantities_table[quantities[j]])));
            output->beginObject()
              .property("name", (char *)device->getName())
              .property("value", multipleValues[j])
              .property("quantity", quantityName)
              .endObject();
          }
        } else {
          strcpy_P(quantityName, (char*)pgm_read_word(&(quantities_table[quantities[0]])));
          output->beginObject()
            .property("name", (char *)device->getName())
            .property("value", device->getValue())
            .property("quantity", quantityName)
            .endObject();
        }

    }
    output->endArray();
}

void commandCompactRead(JsonObject *input, JsonWriter *output){
    float multipleValues[10];
    Device *device;
    output->
    beginArray("data");

    for (int i = 0; i < m.getTotalDevices(); i++) {
        device = m.getDevice(i);
        output->beginObject()
          .property("name", (char *)device->getName());
        if (device->multipleValues()) {
          int totalValues = device->getValues(multipleValues);
          output->beginArray("values");
            for (int j = 0; j < totalValues; j++) {
              output->number(multipleValues[j]);
            }
          output->endArray();
        } else {
          output->beginArray("values")
            .number(device->getValue())
            .endArray();
        }
        output->endObject();
    }
    output->endArray();
}

void commandInfo(JsonObject *input, JsonWriter *output){
    // Display sensor information
    char buffer[30];
    Device *device;
    output->beginArray("data");
    int quantities[10];

    for (int i = 0; i < m.getTotalDevices(); i++) {
        device = m.getDevice(i);
        int totalValues = device->getSensorQuantities(quantities);

        output->beginObject()
          .property("name", (char *)device->getName())
          //.property("sensor", (char *)device->getSensorType())
          .beginArray("quantities");

            for(int j = 0; j < totalValues; j++){
              strcpy_P(buffer, (char*)pgm_read_word(&(quantities_table[quantities[j]])));
                output->beginObject()
                   .property("id", quantities[j])
                   .property("name", buffer)
                .endObject();
            }

        output->endArray().endObject();

    }
    output->endArray();
}


const struct Command commandList[] = {
        {"read", commandRead},
        {"cread", commandCompactRead},
        {"status", commandStatus},
        {"info", commandInfo},
        {"writeConfig", writeConfig}
};
#define TOTAL_COMMANDS sizeof(commandList)/sizeof(Command)


struct AvailableDevice {
    const char *sensorType;
    Device* (*fun)(void *data);
};

const struct AvailableDevice availableDeviceList[] = {
  {DallasTemperatureDevice::getSensorType(), DallasTemperatureDevice::fromConfig},
  {INA219Device::getSensorType(), INA219Device::fromConfig},
  {DHTDevice::getSensorType(), DHTDevice::fromConfig}
};

#define TOTAL_AVAILABLE_DEVICES sizeof(availableDeviceList)/sizeof(AvailableDevice)

Device *dt;



bool cagao(char *name, byte *data, size_t dataSize) {
  bool found=false;
  for(int i = 0; i < TOTAL_AVAILABLE_DEVICES && !found; i++){
    if(strcmp(availableDeviceList[i].sensorType, name) == 0){
      found = true;

      m.addDevice(availableDeviceList[i].fun(data));
    }
  }
  return found;
}

void setup() {

  /*Serial.begin(115200);
  m.addDevice(new INA219Device());
  m.addDevice(new DHTDevice(6, DHT21));
  m.addDevice(new DHTDevice(9, DHT11));
*/
  Serial.begin(115200);
  EEPROMConfig config;

  if(config.loadConfig()) {
    config.loadSensors(cagao);
  }

}




char readBuffer[JSON_READ_BUFFER];
int pairingLeft = 0;
char bufferPosition = 0;
void loop() {
  if(reset) {
    softwareReset();
    reset = false;
  }

    while(Serial.available()){
        char c = Serial.read();

        if (c == '{') {
            pairingLeft++;
        } else if (c == '}') {
            pairingLeft--;
        } else if(pairingLeft == 0){
          //Ping request
          Serial.write(c+1);
          Serial.print("PONG");
          Serial.print('\n');
          return;
        }
        readBuffer[bufferPosition] = c;
        bufferPosition++;

        if(pairingLeft == 0 && bufferPosition > 0){
            //Json message found!
            bufferPosition = 0;
            StaticJsonBuffer<JSON_READ_BUFFER> jsonBuffer;
            JsonObject& root = jsonBuffer.parseObject(readBuffer);

            const char* command = root["command"];
            bool found = false;
            int commandIndex = 0;

            JsonObject& output = jsonBuffer.createObject();
            serialJsonWriter.beginObject();
            while((!found) && (commandIndex < TOTAL_COMMANDS)){
                if(strcmp(commandList[commandIndex].command, command) == 0){
                    found = true;
                    commandList[commandIndex].fun(&root, &serialJsonWriter);
                    serialJsonWriter.property("error", false);
                }
                commandIndex++;
            }

            if (!found) {
                serialJsonWriter.property("error", true);
                serialJsonWriter.property("message", "Comando no encontrado");
            }

            serialJsonWriter.endObject();
            Serial.print("\n");
            Serial.flush();
        }
    }
}
