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
DS18B20 (As many as the Topology Algorithm Allows) --> Data pin Attached to Arduino Pin 2
DHT22 First Sensor --> Data Pin Attached to Arduino Pin 5
DHT22 Second Sensor --> Data Pin Attached to Arduino Pin 6
The Pattern is repeated per DHT22 Add additional pins, Data Pins 2,5,6 NEED 10K Pullups !!!
The DS1307 RTC gives Time & Date via the I2C Bus, This Also has additional DS18B20 pins
onboard and can be used to Join the Sensors to the Arduino, Timestamps on SDcard Write.
The Files are CSV Comma seperated, Datalog.CSV on the SDCARD, if the File exists it
will be Appended if not it will be Created, A Valid Partition IS NEEDED on SD.
Datalog.CSV   ----   DS18B20 DHT22 Log
The Sensor ID for the Temperature Is by Index, to expand use more
Elements from the Number of Sensors.
Temperature in Centigrade, there is also the option of Temperatures in Fahrenheit..
The Sensor ID is from the Text Below(S1,S2) and the Temperature in Centigrade along with
the Relative Humidity in Percentages are logged from each DHT22 sensor,
there is also A Heat Index that can be used along with Temperatures in Fahrenheit.
This is running most of the Arduino Memory but there is enough left to load HTTP
files from SD to process in one routine but this can be Unreliable due to Timing of HTTP.
________________________________________________________________________________________
!*!*!*!*!*!*!* ALL Data is sent to the Serial Console with Time Stamps  *!*!*!*!*!*!*!*!
___________________________________________________________________________
!*!*!*! Please note the Libraries Required in the Includes.......!*!*!*!|
---------------------------------------------------------------------------
*/
#include <Wire.h>
#include "RTClib.h"
#include "DHT.h"
#include <SD.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#define DS18B20_PIN 2
#define DHTPIN 5
#define DHTPIN1 6
#define DHTTYPE DHT22
const int chipSelect = 4;
DHT dht(DHTPIN, DHTTYPE);
DHT dht1(DHTPIN1, DHTTYPE);
RTC_DS1307 RTC;
OneWire oneWire(DS18B20_PIN);       
DallasTemperature sensors(&oneWire);

void setup(void) {
    Serial.begin(9600);
 Serial.println("DHT22 Digital Temperature And Hunidity Logging Starting....");
    Serial.print("Initializing SD card...");
    pinMode(10, OUTPUT);
  if (!SD.begin(chipSelect)) {
    Serial.println("Card failed, or not present");
  return;
  }
  Serial.println("card initialized.");
delay(2000); 
  dht.begin();
  dht1.begin();
  // Initialise I2C  
  Wire.begin();
  
  // Initialise RTC
  RTC.begin();
  if (! RTC.isrunning()) {
    

    RTC.adjust(DateTime(__DATE__, __TIME__));
    
    Serial.println("RTC has been started and is set to system time");
  }
  else Serial.println("RTC is already running.");

  sensors.begin();  // DS18B20 starten
}

void loop(){
  
  DateTime now=RTC.now();  
  show_time_and_date(now);
 delay(2000);
    float h = dht.readHumidity();
  float t = dht.readTemperature();
  float f = dht.readTemperature(true);
  if (isnan(h) || isnan(t) || isnan(f)) {
    Serial.println("Failed to read from DHT sensor 1 !");
  }
  float hi = dht.computeHeatIndex(f, h);
  float h1 = dht1.readHumidity();
  float t1 = dht1.readTemperature();
  float f1 = dht1.readTemperature(true);
  if (isnan(h1) || isnan(t1) || isnan(f1)) {
    Serial.println("Failed to read from DHT sensor 2 !");
    return;
  }
  float hi1 = dht1.computeHeatIndex(f1, h1);
  Serial.print("1st DHT22"); 
  Serial.print("Humidity: "); 
  Serial.print(h);
  Serial.print(" %\t");
  Serial.print("Temperature: "); 
  Serial.print(t);
  Serial.print(" *C   ");
  Serial.print(f);
  Serial.print(" *F\t");
  Serial.print("  Heat index: ");
  Serial.print(hi);
  Serial.println(" *F ");
  Serial.print("2nd DHT22 "); 
  Serial.print("Humidity:"); 
  Serial.print(h1);
  Serial.print(" %\t");
  Serial.print("Temperature: "); 
  Serial.print(t1);
  Serial.print(" *C   ");
  Serial.print(f1);
  Serial.print(" *F\t");
  Serial.print("  Heat index: ");
  Serial.print(hi1);
  Serial.println(" *F    ");
  sensors.requestTemperatures(); 
  File dataFile = SD.open("datalog.CSV", FILE_WRITE);
    if (dataFile) {
      dataFile.print("D1 ,");
      dataFile.print(sensors.getTempCByIndex(0));
      dataFile.print(", D2 ,");
      dataFile.print(sensors.getTempCByIndex(1));
      dataFile.print(", D3 ,");
      dataFile.print(sensors.getTempCByIndex(2));
      dataFile.print(", D4 ,");
      dataFile.print(sensors.getTempCByIndex(3));
      dataFile.print(", DH1H ,");
      dataFile.print(h);
      dataFile.print(", DH1T ,");      
      dataFile.print(t);
      dataFile.print(", DH2H ,");
      dataFile.print(h1);
      dataFile.print(", DH2T ,");
      dataFile.print(t1);  
      dataFile.print(", ");      
  dataFile.print(now.year(), DEC);
  dataFile.print('/');
  dataFile.print(now.month(), DEC);
  dataFile.print('/');
  dataFile.print(now.day(), DEC);
  dataFile.print(' ');
  dataFile.print(now.hour(), DEC);
  dataFile.print(':');
  dataFile.print(now.minute(), DEC);
  dataFile.print(':');
  dataFile.print(now.second(), DEC);
  dataFile.println(",");
      dataFile.close();
    delay(500);
      }
    else {
      Serial.println("error opening datalog.CSV");
    }  
  show_temperature(sensors.getTempCByIndex(0));
  show_temperature(sensors.getTempCByIndex(1));
  show_temperature(sensors.getTempCByIndex(2));
  show_temperature(sensors.getTempCByIndex(3));  
  delay(1000);
}
String get_day_of_week(uint8_t dow){ 
  
  String dows="  ";
  switch(dow){
   case 0: dows="Su"; break;
   case 1: dows="Mo"; break;
   case 2: dows="Tu"; break;
   case 3: dows="We"; break;
   case 4: dows="Th"; break;
   case 5: dows="Fr"; break;
   case 6: dows="Sa"; break;
  }
  
  return dows;
}
void show_time_and_date(DateTime datetime){
  
  Serial.print(get_day_of_week(datetime.dayOfWeek()));
  Serial.print(", ");
  if(datetime.day()<10)Serial.print(0);
  Serial.print(datetime.day(),DEC);
  Serial.print(".");
  if(datetime.month()<10)Serial.print(0);
  Serial.print(datetime.month(),DEC);
  Serial.print(".");
  Serial.println(datetime.year(),DEC);
  
  
  if(datetime.hour()<10)Serial.print(0);
  Serial.print(datetime.hour(),DEC);
  Serial.print(":");
  if(datetime.minute()<10)Serial.print(0);
  Serial.print(datetime.minute(),DEC);
  Serial.print(":");
  if(datetime.second()<10)Serial.print(0);
  Serial.println(datetime.second(),DEC);
}

void show_temperature(float temp){
  
  Serial.print("Temperatur: ");
  Serial.print(temp);
  Serial.print(" "); 
  Serial.write(176); 
  Serial.println("C");
}
