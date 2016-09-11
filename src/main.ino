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

void software_Reset(){
  asm volatile ("jmp 0");
}

/************************/
/* ds18b20 Sensors      */
/************************/
// I have some issues if this is not created here
//OneWire oneWire(SENSOR_ONE_WIRE_BUS);
// Pass our oneWire reference to Dallas Temperature.
//DallasTemperature sensors(&oneWire);


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
    Device *device;
    output->
    beginArray("data");

    for (int i = 0; i < m.getTotalDevices(); i++) {
        device = m.getDevice(i);
        output->beginObject()
          .property("name", (char *)device->getName());
        if (device->multipleValues()) {
          int totalValues = device->getValues(multipleValues);
          output->
            property("multiple", true)
            .property("total", totalValues)
            .beginArray("values");

            for (int j = 0; j < totalValues; j++) {
              output->number(multipleValues[j]);
            }
          output->endArray();
        } else {
          output->
            property("multiple", false)
            .property("value", device->getValue());
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
    int units[10];

    for (int i = 0; i < m.getTotalDevices(); i++) {
        device = m.getDevice(i);
        int totalValues = device->getSensorUnits(units);

        output->beginObject()
          .property("name", (char *)device->getName())
          //.property("sensor", (char *)device->getSensorType())
          .beginArray("quantities");

            for(int j = 0; j < totalValues; j++){
              strcpy_P(buffer, (char*)pgm_read_word(&(units_table[units[j]])));
                output->beginObject()
                   .property("id", units[j])
                   .property("name", buffer)
                .endObject();
            }

        output->endArray().endObject();

    }
    output->endArray();
}


#define TOTAL_COMMANDS 4
const struct Command commandList[TOTAL_COMMANDS] = {
        {"read", commandRead},
        {"status", commandStatus},
        {"info", commandInfo},
        {"writeConfig", writeConfig}
};


struct AvailableDevice {
    char *sensorType;
    Device* (*fun)(void *data);
};

const struct AvailableDevice availableDeviceList[] = {
  {DallasTemperatureDevice::getSensorType(), DallasTemperatureDevice::fromConfig}
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
  // put your setup code here, to run once:

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
    software_Reset();
    reset = false;
  }
  /*
  byte buffer[255];
  buffer[0] = 'D';
  buffer[1] = 'S';
  buffer[2] = '1';
  buffer[3] = '8';
  buffer[4] = 'B';
  buffer[5] = '2';
  buffer[6] = '0';
  buffer[7] = 0;
  buffer[8] = 8;
  buffer[9] = 0x28;
  buffer[10] = 0xFF;
  buffer[11] = 0x10;
  buffer[12] = 0x93;
  buffer[13] = 0x6F;
  buffer[14] = 0x14;
  buffer[15] = 0x4;
  buffer[16] = 0x11;

  buffer[17] = 'D';
  buffer[18] = 'S';
  buffer[19] = '1';
  buffer[29] = '8';
  buffer[21] = 'B';
  buffer[22] = '2';
  buffer[23] = '0';
  buffer[24] = 0;
  buffer[25] = 8;
  buffer[26] = 1;
  buffer[27] = 2;
  buffer[28] = 3;
  buffer[29] = 4;
  buffer[30] = 5;
  buffer[31] = 6;
  buffer[32] = 7;
  buffer[33] = 8;

*/

    while(Serial.available()){
        char c = Serial.read();

        if (c == '{') {
            pairingLeft++;
        } else if (c == '}') {
            pairingLeft--;
        }
        readBuffer[bufferPosition] = c;
        bufferPosition++;

        /*if (bufferPosition > JSON_READ_BUFFER){
            Serial.println("Parse error");
        }*/

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
