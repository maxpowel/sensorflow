Sensorflow Arduino
==================

This sketch will convert your Arduino into a sensorflow device

About Sensorflow
----------------
This library is an sensor plug and play for Arduino.
There are a lot of sensors and every sensor has it own library. If you have a lot of sensors,
you will have tons of initizalization and configuration code. And every of these sensors has
a different way to use so you need write different code for every one of these sensors. That
is a lot of code that is unstructured and a pain of maintain.

Sensorflow tries to solve this problem. There is a Device interface that provides a few methods
(for configuration and read values). Then you only have to instantiate this object device with
this minimal configuration data and thats all, in one line you have your sensor working!.

Now you can use you sensors or register in a sensor respository (like a set of sensors) and apply
bulk task (like read all sensors) or request stored sensors by id. In this step you reduced the
complexity of multiple object interface (one per sensor) to only one, the Device interface.

Why sensorflow
--------------
Imagine that I have a DS18B20, a DHT and a BH1750 sensor. The sketch will look something like this:

```c
#include <OneWire.h>
#include <DallasTemperature.h>
#include "DHT.h"
#include <Wire.h>
#include <BH1750.h>

#define ONE_WIRE_BUS 2
#define DHTPIN 3
#define DHTTYPE DHT22

DHT dht(DHTPIN, DHTTYPE);
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

BH1750 lightMeter;

void setup(void)
{
  Serial.begin(9600);
  Serial.println("Dallas Temperature IC Control Library Demo");

  dht.begin();
  sensors.begin();
  lightMeter.begin();

  sensors.requestTemperatures();
  DeviceAddress address =  { 0x28, 0x1D, 0x39, 0x31, 0x2, 0x0, 0x0, 0xF0 };
  Serial.println(sensors.getTempCByAddress(address));  
  float h = dht.readHumidity();
  float t = dht.readTemperature();
  uint16_t lux = lightMeter.readLightLevel();
}

void loop(void)
{

}
```
I only want to read the sensors (same action) but those sensor works completely different. You have to deal with
multiple object types and use every one in a different way. Add any other sensor type will change completely
your program. This is a problem. Now compare with the sensorflow code.

```c
#include <OneWire.h>
#include <DallasTemperature.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <DHT_U.h>
#include <SensorMonitor.h>

SensorMonitor m;

void setup() {
  Serial.begin(9600);
  DeviceAddress address =  { 0x28, 0x1D, 0x39, 0x31, 0x2, 0x0, 0x0, 0xF0 };
  Device *ds = new DallasTemperatureDevice(address);
  Device *dht = DHTDevice(3, DHT22);
  Device *bh = new BH1750Device();

  Serial.println(ds->getValue());
  Serial.println(bh->getValue());
  // DHT returns multiple values, temperature and humidity. For this reason you need the array
  float values[2];
  dht->getValues(values);
  Serial.println(values[0]);
  Serial.println(values[1]);
}

void loop()
{

}
```

This code is now homogeneous. All of those objects implement the same Device class. You
dont have to know how the sensor works neither waste time configuring and refactoring code for a sensor that
you could replace tomorrow. Now your program only have to deal with one interface for all sensors. All this work
is done thanks to the [SensorMonitor library](https://github.com/maxpowel/SensorMonitor). Now imagine that you want
to access to this sensors from other devices (for example a computer). You have to design a communication protocol,
stuff for managing multiple sensors (and remember that every sensor is different), build your client libraries... This is
what sensorflow does. It does all the job and you only have to request data and configure sensors by a friendly way from your client library (from [python](https://github.com/maxpowel/sensorflow-python) for example). The configuration is stored in the EEPROM so you only have to it once.
The installation process is:
* Connect your sensors to arduino
* Upload sensorflow
* Configure sensors using a friendly interface (or from file)
* Ship to your client

If you connect or remove sensors, just connect it to your computer and update the configuration. Forget about writing arduino code when you dont have to!

Now you can focus on your objetive that is fetch sensor data and process/store.
