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


/************************/

/* Simple json message recognizer */
#define JSON_READ_BUFFER 100
JsonWriter serialJsonWriter(&Serial);

SensorMonitor m;

/************************/
/* ds18b20 Sensors      */
/************************/
// I have some issues if this is not created here
OneWire oneWire(SENSOR_ONE_WIRE_BUS);
// Pass our oneWire reference to Dallas Temperature.
DallasTemperature sensors(&oneWire);


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
    int quantities[10];

    for (int i = 0; i < m.getTotalDevices(); i++) {
        device = m.getDevice(i);
        int totalValues = device->getSensorQuantities(quantities);

        output->beginObject()
          .property("name", (char *)device->getName())
          .property("sensor", (char *)device->getSensorType())
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


#define TOTAL_COMMANDS 3
const struct Command commandList[TOTAL_COMMANDS] = {
        {"read", commandRead},
        {"status", commandStatus},
        {"info", commandInfo}
};


void setup() {
  // put your setup code here, to run once:

  Serial.begin(115200);
  m.addDevice(new INA219Device());
  m.addDevice(new DHTDevice(6, DHT21));
  m.addDevice(new DHTDevice(9, DHT11));


}




char readBuffer[JSON_READ_BUFFER];
int pairingLeft = 0;
char bufferPosition = 0;
void loop() {

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
        }
    }
}
