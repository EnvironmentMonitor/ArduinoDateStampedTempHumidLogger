/*
 ___/'¬_         /\
{_-_ ^  \_______//
     /           \
     \ _  _____  /
     || ||     |||
     || ||     |||  
BadTail Projects ALL RIGHTS GRANTED this is OPEN SOURCE and 
without any Implied or Otherwise "Gaurantees" of a 
Particular Part in Whole or Function
and free to use in any or all parts.........
Libraries are part of Standard GNU License........
Rod Con ..... The Road to Contentment.............

This is a just one Reliable approach to IOT Logging, put it on the SDCARD then
PUSH The Data to the Remote Server Seperately allowing Local Analyssis and backup.

Use Arduino UNO or Mini Pro --> 
Ethernet Shield or SDCARD Breakout Board (With 8GB SDCARD) --> 
DS18B20 (As many as the Topology Algorithm Allows) --> Data pin Attached to Arduino Pin 3
DHT22 First Sensor --> Data Pin Attached to Arduino Pin 7
DHT22 Second Sensor --> Data Pin Attached to Arduino Pin 8
The Pattern is repeated per DHT22 Add additional pins.......
The DS1307 RTC gives Time & Date via the I2CDev Bus, This Also has an additional DS18B20
Location onboard and can be used to Join the Sensors to the Arduino, Timestamps on SDWrite.
The Files are CSV tab seperated, Datalog.CSV and Humidlog.CSV on the SDCARD, if the Files
Exist they will be Appended if not they will be Created, A Valid Partition IS NEEDED on SD.
Datalog.CSV
The Sensor ID for the Temperature Currently only LOGS limited fields to expand use more
Elements from the address array.
Temperature in Centigrade, there is also the option of Temperatures in Fahrenheit..
Humidlog.CSV
The Sensor ID is from the Text Below and the Temperature in Centigrade along with
the Relative Humidity in Percentage are logged from each sensor,
there is also A Heat Index that can be used along with Temperatures in Fahrenheit.
This is running most of the Arduino Memory but there is enough left to load HTTP
files from SD to process in one routine but this can be Unreliable due to Timing of HTTP.

!*!*! Please note the Libraries Required in the Includes.......

*/

#include "Wire.h"
#include "I2Cdev.h"
#include "DS1307.h"
#include "DHT.h"
#include <OneWire.h>
#include <SD.h>

#define DHTPIN 7     // what pin we're connected to for the Humidity Sensors
#define DHTPIN1 8
#define DHTTYPE DHT22   // DHT 22  (AM2302)
// Initialize DHT sensor for normal 16mhz Arduino
DHT dht(DHTPIN, DHTTYPE);
DHT dht1(DHTPIN1, DHTTYPE);
// Initialize DHT sensor for 84mhz Arduino DUE
//DHT dht(DHTPIN, DHTTYPE, 30);
// Setup Real Time Clock, the time IN DS1307 MUST BE SET CORRECTLY !!!
DS1307 rtc;
uint16_t year;
uint8_t month, day, hours, minutes, seconds;
OneWire  ds(3);  // DS18B20 OneWire Data Pin on Arduino pin 3 For the Temperature Sensors
const int chipSelect = 4;      // CS for SD card

void setup() {
  // read all clock info from device
  rtc.getDateTime24(&year, &month, &day, &hours, &minutes, &seconds);
  Serial.begin(9600); 
  Serial.print("Currently Set Time Is :- ");
  Serial.print(year);
  Serial.print( "/");
  Serial.print(month);
  Serial.print("/");
  Serial.print(day);
  Serial.print("|");
  Serial.print(hours);
  Serial.print(":");
  Serial.print(minutes);
  Serial.print(":");
  Serial.println(seconds);
  Serial.println("DHTxx test!");
  Serial.print("Initializing SD card...");
  pinMode(10, OUTPUT);
  if (!SD.begin(chipSelect)) {
    Serial.println("Card failed, or not present");
    return;
  }
  Serial.println("card initialized.");
  dht.begin();
  dht1.begin();
}

void loop() {
  // Wait a couple of seconds between measurements.
  delay(2000);
 byte i;
  byte present = 0;
  byte type_s;
  byte data[12];
  byte addr[8];
  float celsius, fahrenheit;
  if ( !ds.search(addr)) {
    Serial.println("No more addresses.");
    Serial.println();
    ds.reset_search();
    delay(250);
    return;
  }
  Serial.print("ROM =");
  for( i = 0; i < 8; i++) {
    Serial.write(' ');
    Serial.print(addr[i], HEX);
  }
  if (OneWire::crc8(addr, 7) != addr[7]) {
      Serial.println("CRC is not valid!");
      return;
  }
  Serial.println();
  // the first ROM byte indicates which chip
  switch (addr[0]) {
    case 0x10:
      Serial.println("  Chip = DS18S20");  // or old DS1820
      type_s = 1;
      break;
    case 0x28:
      Serial.println("  Chip = DS18B20");
      type_s = 0;
      break;
    case 0x22:
      Serial.println("  Chip = DS1822");
      type_s = 0;
      break;
    default:
      Serial.println("Device is not a DS18x20 family device.");
      return;
  } 

  ds.reset();
  ds.select(addr);
  ds.write(0x44,1);         // start conversion, with parasite power on at the end
  
  delay(1000);     // maybe 750ms is enough, maybe not
  // we might do a ds.depower() here, but the reset will take care of it.
  
  present = ds.reset();
  ds.select(addr);    
  ds.write(0xBE);         // Read Scratchpad

  Serial.print("  Data = ");
  Serial.print(present,HEX);
  Serial.print(" ");
  for ( i = 0; i < 9; i++) {           // we need 9 bytes
    data[i] = ds.read();
    Serial.print(data[i], HEX);
    Serial.print(" ");
  }
  Serial.print(" CRC=");
  Serial.print(OneWire::crc8(data, 8), HEX);
  Serial.println();

  // convert the DS18B20 Hex data to actual temperature

  unsigned int raw = (data[1] << 8) | data[0];
  if (type_s) {
    raw = raw << 3; // 9 bit resolution default
    if (data[7] == 0x10) {
      // count remain gives full 12 bit resolution
      raw = (raw & 0xFFF0) + 12 - data[6];
    }
  } else {
    byte cfg = (data[4] & 0x60);
    if (cfg == 0x00) raw = raw << 3;  // 9 bit resolution, 93.75 ms
    else if (cfg == 0x20) raw = raw << 2; // 10 bit res, 187.5 ms
    else if (cfg == 0x40) raw = raw << 1; // 11 bit res, 375 ms
    // default is 12 bit resolution, 750 ms conversion time
  }
  celsius = (float)raw / 16.0;
  fahrenheit = celsius * 1.8 + 32.0;
  rtc.getDateTime24(&year, &month, &day, &hours, &minutes, &seconds);
  File dataFile = SD.open("datalog.CSV", FILE_WRITE);
    // if the file is available, write to it:
    if (dataFile) {
      dataFile.print("  Sensor ID = ");
      dataFile.print(addr[7],HEX);  //  <--- insert additonal Address data here
      dataFile.print("  Temperature = ");
      dataFile.print(celsius);
      dataFile.print(" Celsius ");
  dataFile.print(year);
  dataFile.print("/");
  dataFile.print(month);
  dataFile.print("/");
  dataFile.print(day);
  dataFile.print("|");
  dataFile.print(hours);
  dataFile.print(":");
  dataFile.print(minutes);
  dataFile.print(":");
  dataFile.println(seconds);
      dataFile.close();
//    delay(5000);// Pause here to ensure when power off FILE CLOSED!!
      }         // Wait for Pin 13 LED (SPI Activity) to Flash before power off!
    // if the file isn't open, pop up an error:
    else {
      Serial.println("error opening datalog.CSV");
    }
  rtc.getDateTime24(&year, &month, &day, &hours, &minutes, &seconds);
  float h = dht.readHumidity();
  // Read temperature as Celsius
  float t = dht.readTemperature();
  // Read temperature as Fahrenheit
  float f = dht.readTemperature(true);
  if (isnan(h) || isnan(t) || isnan(f)) {
    Serial.println("Failed to read from DHT sensor!");
    return;
  }
  float hi = dht.computeHeatIndex(f, h);
  float h1 = dht1.readHumidity();
  // Read temperature as Celsius
  float t1 = dht1.readTemperature();
  // Read temperature as Fahrenheit
  float f1 = dht1.readTemperature(true);
  if (isnan(h1) || isnan(t1) || isnan(f1)) {
    Serial.println("Failed to read from DHT sensor!");
    return;
  }
  float hi1 = dht1.computeHeatIndex(f1, h1);
      File dataFile1 = SD.open("humidlog.CSV", FILE_WRITE);
    // if the file is available, write to it:
    if (dataFile1) {
      dataFile1.print("  S1 Humidity Percentage = ");
      dataFile1.print(h);
      dataFile1.print("  S1 Temperature = ");
      dataFile1.print(t);
      dataFile1.println(" Celsius ");
      dataFile1.print("  S2 Humidity Percentage = ");
      dataFile1.print(h1);
      dataFile1.print("  S2 Temperature = ");
      dataFile1.print(t1);
      dataFile1.print(" Celsius ");
  dataFile1.print(year);
  dataFile1.print("/");
  dataFile1.print(month);
  dataFile1.print("/");
  dataFile1.print(day);
  dataFile1.print("|");
  dataFile1.print(hours);
  dataFile1.print(":");
  dataFile1.print(minutes);
  dataFile1.print(":");
  dataFile1.println(seconds);
      dataFile1.close();
    delay(5000);// Pause here to ensure when power off FILE CLOSED!!
      }         // Wait for Pin 13 LED (SPI Activity) to Flash before power off!
    // if the file isn't open, pop up an error:
    else {
      Serial.println("error opening humidlog.CSV");
    }
  rtc.getDateTime24(&year, &month, &day, &hours, &minutes, &seconds);
  Serial.print("Humidity: "); 
  Serial.print(h);
  Serial.print(" %\t");
  Serial.print("Temperature: "); 
  Serial.print(t);
  Serial.print(" *C ");
  Serial.print(f);
  Serial.print(" *F\t");
  Serial.print("Heat index: ");
  Serial.print(hi);
  Serial.println(" *F");
  Serial.print("Currently Set Time Is :- ");
  Serial.print(year);
  Serial.print( "/");
  Serial.print(month);
  Serial.print("/");
  Serial.print(day);
  Serial.print("|");
  Serial.print(hours);
  Serial.print(":");
  Serial.print(minutes);
  Serial.print(":");
  Serial.println(seconds);
}
