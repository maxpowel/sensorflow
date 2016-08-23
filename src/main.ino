#include "JsonWriter.h"
#include <ArduinoJson.h>


JsonWriter serialJsonWriter(&Serial);

struct Command {
    char  command[20];
    void (*fun)(JsonObject *input, JsonWriter *output);
} command;



void commandInfo(JsonObject *input, JsonWriter *output){
    // Display sensor information
    output->
    beginObject("data")
      .property("status", "ok")
    .endObject();
}

#define TOTAL_COMMANDS 1
struct Command commandList[TOTAL_COMMANDS] = {
        {"info", commandInfo}
};


void setup() {
  // put your setup code here, to run once:

  Serial.begin(9600);


}


/* Simple json message recognizer */
#define JSON_READ_BUFFER 180

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
        }
    }
}
