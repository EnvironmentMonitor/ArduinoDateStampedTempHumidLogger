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

Use Arduino UNO or Mini Pro(Use 5V Version For better stability of Sensor Readings) 
Ethernet Shield or SDCARD Breakout Board With 8GB SDCARD (Min=2Gb) --> SPI Pins 10-13 + 4
DS18B20 (As many as the Topology Algorithm Allows) --> Data pin Attached to Arduino Pin 3
DHT22 First Sensor --> Data Pin Attached to Arduino Pin 2
DHT22 Second Sensor --> Data Pin Attached to Arduino Pin 5
The Pattern is repeated per DHT22 Add additional pins, Data Pins 2,3,5 NEED 10K Pullups !!!
The DS1307 RTC gives Time & Date via the I2C Bus, This Also has additional DS18B20 pins
onboard and can be used to Join the Sensors to the Arduino, Timestamps on SDcard Write.
The Files are CSV Comma seperated, Datalog.CSV and Humidlog.CSV on the SDCARD, if the Files
Exist they will be Appended if not they will be Created, A Valid Partition IS NEEDED on SD.
Datalog.CSV   ----   DS18B20 Log
The Sensor ID for the Temperature Currently only LOGS limited fields, to expand use more
Elements from the address array(noted below).
Temperature in Centigrade, there is also the option of Temperatures in Fahrenheit..
Humidlog.CSV  ----   DHT22 Log
The Sensor ID is from the Text Below(S1,S2) and the Temperature in Centigrade along with
the Relative Humidity in Percentages are logged from each sensor,
there is also A Heat Index that can be used along with Temperatures in Fahrenheit.
This is running most of the Arduino Memory but there is enough left to load HTTP
files from SD to process in one routine but this can be Unreliable due to Timing of HTTP.
________________________________________________________________________________________
!*!*!*!*!*!*!* ALL Data is sent to the Serial Console with Time Stamps  *!*!*!*!*!*!*!*!
___________________________________________________________________________
!*!*!*! Please note the 4 Libraries Required in the Includes.......!*!*!*!|
---------------------------------------------------------------------------
*/
#include <Wire.h>
#include <DHT.h>
#include <OneWire.h>
#include <SD.h>
//_________________________________________________________________________________________
//!*!*! The Libraries MUST be installed Before this will Compile^You have been Warned !*!*!
//_________________________________________________________________________________________
#define DS1307_I2C_ADDRESS 0x68
#define DHTPIN 2               // What pin we're connected to for the First Humidity Sensor
#define DHTPIN1 5              // What pin we're connected to for the Second Humidity Sensor
#define DHTTYPE DHT22          // DHT 22  (AM2302)
// Initialize DHT sensor for normal 16mhz Arduino
DHT dht(DHTPIN, DHTTYPE);
DHT dht1(DHTPIN1, DHTTYPE);
// Initialize DHT sensor for 84mhz Arduino DUE
//DHT dht(DHTPIN, DHTTYPE, 30);
OneWire  ds(3);  // DS18B20 OneWire Data Pin on Arduino pin 3 For the Temperature Sensors
const int chipSelect = 4;      // CS for SD card
byte decToBcd(byte val)
{
 return ( (val/10*16) + (val%10) );
}
byte bcdToDec(byte val)
{
 return ( (val/16*10) + (val%16) );
}
// Assumes you're passing in valid numbers
void setDateDs1307(byte second, // 0-59
 byte minute, // 0-59
 byte hour, // 1-23
 byte dayOfWeek, // 1-7
 byte dayOfMonth, // 1-28/29/30/31
 byte month, // 1-12
 byte year) // 0-99
{
 Wire.beginTransmission(DS1307_I2C_ADDRESS);
 Wire.write(0);
 Wire.write(decToBcd(second)); // 0 to bit 7 starts the clock
 Wire.write(decToBcd(minute));
 Wire.write(decToBcd(hour));
 Wire.write(decToBcd(dayOfWeek));
 Wire.write(decToBcd(dayOfMonth));
 Wire.write(decToBcd(month));
 Wire.write(decToBcd(year));
 Wire.endTransmission();
}
// Gets the date and time from the ds1307
void getDateDs1307(byte *second,
 byte *minute,
 byte *hour,
 byte *dayOfWeek,
 byte *dayOfMonth,
 byte *month,
 byte *year)
{
// Reset the register pointer
 Wire.beginTransmission(DS1307_I2C_ADDRESS);
 Wire.write(0);
 Wire.endTransmission();
 Wire.requestFrom(DS1307_I2C_ADDRESS, 7);
// A few of these need masks because certain bits are control bits
 *second = bcdToDec(Wire.read() & 0x7f);
 *minute = bcdToDec(Wire.read());
 *hour = bcdToDec(Wire.read() & 0x3f); // Need to change this if 12 hour am/pm
 *dayOfWeek = bcdToDec(Wire.read());
 *dayOfMonth = bcdToDec(Wire.read());
 *month = bcdToDec(Wire.read());
 *year = bcdToDec(Wire.read());
}
void setup()
{
    Serial.begin(9600);
    Serial.println("Starting The Environmental Monitor ...");
    Serial.println("Logging All Sensor Readings to SD card ...");
    Serial.println("DHT22 Humidity & Temperature Sensors (*2) plus ...");
    Serial.println("DS18B20 Temperature Sensors (Min = 1) with DS1307 RTC ...");  
    Serial.print("Initializing SD card...");
    pinMode(10, OUTPUT);
  if (!SD.begin(chipSelect)) {
    Serial.println("Card failed, or not present");
    return;
  }
  Serial.println("card initialized.");
  dht.begin();
  dht1.begin();
 byte second, minute, hour, dayOfWeek, dayOfMonth, month, year;
 second = 10;
 minute = 01;
 hour = 15;
 dayOfWeek = 1;  // First Day "Saturday"
 dayOfMonth = 2;
 month = 5;
 year = 15;
// Set the correct time and date Above and use the line below to set the time if Needed!
// setDateDs1307(second, minute, hour, dayOfWeek, dayOfMonth, month, year);
}

void loop()
{
 byte second, minute, hour, dayOfWeek, dayOfMonth, month, year;
 getDateDs1307(&second, &minute, &hour, &dayOfWeek, &dayOfMonth, &month, &year);
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
  Serial.println();
  if (OneWire::crc8(addr, 7) != addr[7]) {
      Serial.print("CRC is not valid!");
      getDateDs1307(&second, &minute, &hour, &dayOfWeek, &dayOfMonth, &month, &year);
       Serial.print(hour, DEC);
       Serial.print(":");
       Serial.print(minute, DEC);
       Serial.print(":");
       Serial.print(second, DEC);
       Serial.print(" ");
       Serial.print(month, DEC);
       Serial.print("/");
       Serial.print(dayOfMonth, DEC);
       Serial.print("/");
       Serial.print(year, DEC);
       Serial.print(" Day_of_week:");
       Serial.println(dayOfWeek, DEC);
      return;
  }
  getDateDs1307(&second, &minute, &hour, &dayOfWeek, &dayOfMonth, &month, &year);
 Serial.print(hour, DEC);
 Serial.print(":");
 Serial.print(minute, DEC);
 Serial.print(":");
 Serial.print(second, DEC);
 Serial.print(" ");
 Serial.print(month, DEC);
 Serial.print("/");
 Serial.print(dayOfMonth, DEC);
 Serial.print("/");
 Serial.print(year, DEC);
 Serial.print(" Day_of_week:");
 Serial.println(dayOfWeek, DEC);
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
  ds.write(0x44,1);                       // start conversion, with parasite power on at the end
  delay(1000);                            // maybe 750ms is enough, maybe not
  present = ds.reset();
  ds.select(addr);    
  ds.write(0xBE);                         // Read Scratchpad
  Serial.print("  Data = ");
  Serial.print(present,HEX);
  Serial.print(" ");
  for ( i = 0; i < 9; i++) {              // we need 9 bytes
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
  
 getDateDs1307(&second, &minute, &hour, &dayOfWeek, &dayOfMonth, &month, &year);  
  File dataFile = SD.open("datalog.CSV", FILE_WRITE);
// if the file is available, write to it:
    if (dataFile) {
 dataFile.print("  Sensor ID = ,");
 dataFile.print(addr[7],HEX);  //  <--- insert additonal Address field data here [...6,7]
 dataFile.print(",  Temperature = ,");
 dataFile.print(celsius);
 dataFile.print(", Celsius ");
 dataFile.print(" Date Stamp ,");
 dataFile.print(hour, DEC);
 dataFile.print(":");
 dataFile.print(minute, DEC);
 dataFile.print(":");
 dataFile.print(second, DEC);
 dataFile.print(" ");
 dataFile.print(month, DEC);
 dataFile.print("/");
 dataFile.print(dayOfMonth, DEC);
 dataFile.print("/");
 dataFile.print(year, DEC);
 dataFile.print(" Day_of_week:");
 dataFile.print(dayOfWeek, DEC); 
 dataFile.println(",");
 dataFile.close();
    delay(5000);// Pause here to ensure when power off FILE CLOSED!!
      }         // Wait for Pin 13 LED (SPI Activity) to Flash before power off!
// if the file isn't open, pop up an error:
    else {
return;
    }
  float h = dht.readHumidity();
// Read temperature as Celsius
  float t = dht.readTemperature();
// Read temperature as Fahrenheit
  float f = dht.readTemperature(true);
  if (isnan(h) || isnan(t) || isnan(f)) {
    return;
  }
  float hi = dht.computeHeatIndex(f, h);
  float h1 = dht1.readHumidity();
// Read temperature as Celsius
  float t1 = dht1.readTemperature();
// Read temperature as Fahrenheit
  float f1 = dht1.readTemperature(true);
  if (isnan(h1) || isnan(t1) || isnan(f1)) {
    return;
  }
  float hi1 = dht1.computeHeatIndex(f1, h1);
 getDateDs1307(&second, &minute, &hour, &dayOfWeek, &dayOfMonth, &month, &year);  
      File dataFile1 = SD.open("humidlog.CSV", FILE_WRITE);
// if the file is available, write to it:
    if (dataFile1) {
 dataFile1.print("  S1 Humidity Percentage = ,");
 dataFile1.print(h);
 dataFile1.print(",  S1 Temperature = ,");
 dataFile1.print(t);
 dataFile1.print(", Celsius ");
 dataFile1.print("  S2 Humidity Percentage = ,");
 dataFile1.print(h1);
 dataFile1.print(",  S2 Temperature = ,");
 dataFile1.print(t1);
 dataFile1.print(", Celsius ,");
 dataFile1.print(hour, DEC);
 dataFile1.print(":");
 dataFile1.print(minute, DEC);
 dataFile1.print(":");
 dataFile1.print(second, DEC);
 dataFile1.print(" ");
 dataFile1.print(month, DEC);
 dataFile1.print("/");
 dataFile1.print(dayOfMonth, DEC);
 dataFile1.print("/");
 dataFile1.print(year, DEC);
 dataFile1.print(" Day_of_week:");
 dataFile1.print(dayOfWeek, DEC);
 dataFile1.println(",");
 dataFile1.close();
    delay(5000);// Pause here to ensure when power off FILE CLOSED!!
      }         // Wait for Pin 13 LED (SPI Activity) to Flash before power off!
// if the file isn't open, pop up an error:
    else {
      return;
    }
  Serial.print("First - Humidity: "); 
  Serial.print(h);
  Serial.print(" %\t");
  Serial.print("Temperature: "); 
  Serial.print(t);
  Serial.print(" *C ");
  Serial.print(f);
  Serial.print(" *F\t");
  Serial.print("Heat index: ");
  Serial.print(hi);
  Serial.print(" *F");
  Serial.print("Second - Humidity: "); 
  Serial.print(h1);
  Serial.print(" %\t");
  Serial.print("Temperature: "); 
  Serial.print(t1);
  Serial.print(" *C ");
  Serial.print(f1);
  Serial.print(" *F\t");
  Serial.print("Heat index: ");
  Serial.print(hi1);
  Serial.println(" *F");
 getDateDs1307(&second, &minute, &hour, &dayOfWeek, &dayOfMonth, &month, &year);
 Serial.print(hour, DEC);
 Serial.print(":");
 Serial.print(minute, DEC);
 Serial.print(":");
 Serial.print(second, DEC);
 Serial.print(" ");
 Serial.print(month, DEC);
 Serial.print("/");
 Serial.print(dayOfMonth, DEC);
 Serial.print("/");
 Serial.print(year, DEC);
 Serial.print(" Day_of_week:");
 Serial.println(dayOfWeek, DEC);
 delay(10);
}
