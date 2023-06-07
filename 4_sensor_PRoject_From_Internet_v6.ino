#include <OneWire.h>
#include <DallasTemperature.h>
#include <LiquidCrystal_I2C.h>
#include <Wire.h>  //for i2c scanner
#include "Arduino.h"
#include <I2C_RTC.h>
#include <SPI.h>
#include "SdFat.h" //for SD card module
SdFat sd;    //object for sd card
SdFile file; //file system
//
#define I2C_Dis1 0x26 //I2C address display no1 - temperatures; insert the right address for your device!!
#define I2C_Dis2 0x27 //I2C address display no2 - date, state; insert the right address for your device!!
#define I2C_Clock 0x68 //I2C for clocks; insert the right address for your device!!

// Data (yelow) wires from (DS18B20) temp sensors are plugged into following pins of Arduino UNO
#define ONE_WIRE_BUS1 2 //Sensor no1 - pin no 2
#define ONE_WIRE_BUS2 3 //Sensor no2 - pin no 3
#define ONE_WIRE_BUS3 4 //Sensor no3 - pin no 4
#define ONE_WIRE_BUS4 5 //Sensor no4 - pin no 5
#define CS 10 // CS pin for the MicroSD card

// Setup a oneWire instance for each OneWire device (keep in mind this is not always advantageous architecture but, I use it in order to have full control of sensors. E.g. even if I exchange the sensors I dont have to check its address, my T1 will always be first sensor..) 
OneWire oneWire1(ONE_WIRE_BUS1);
OneWire oneWire2(ONE_WIRE_BUS2);
OneWire oneWire3(ONE_WIRE_BUS3);
OneWire oneWire4(ONE_WIRE_BUS4);  

// Pass oneWire reference to DallasTemperature library
DallasTemperature sensor1(&oneWire1);
DallasTemperature sensor2(&oneWire2);
DallasTemperature sensor3(&oneWire3);
DallasTemperature sensor4(&oneWire4);

//Clock module DS3231
static DS3231 RTC;
int Year_;
int Month_;
int Day_;
int Hours_;
int Minutes_;
int Seconds_;
char daysOfTheWeek[7][12] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};

// Setup instances for LCD dispalys
LiquidCrystal_I2C lcd1(I2C_Dis1,16,2);  // set the LCD address to "I2C_Dis1" for a 16 chars and 2 line display
LiquidCrystal_I2C lcd2(I2C_Dis2,16,2);  // set the LCD address to "I2C_Dis2" for a 16 chars and 2 line display


int deviceCount = 0; //Used to check if the sensor is found or not
int I2CDevices = 0;  //Used to cound the number of I2C device during initialization 

float T1, T2, T3, T4; //Variables for the tmperature readings from sensors

const byte OnOff = 8; //toggle switch (ON-OFF) start/stopp logging 

bool WritingEnabled = false; //we switch the status of this variable with the toggle switch
bool SwitchStatus = false; //by default, this variable is set as false (initial value with withs reading of toggle switch position will be updated (if false - no writting to file)
int OUT = 0; //1 - debug; 0 - final: format of data output into serial
unsigned long startTime;   
unsigned long DeltaT;  //stores delay to be considered in order to have desired data acquisition frequency 
int year = 0;//
unsigned long elapsedTime;
unsigned long elapsedTimeRTC; //Elapsed time based on RTC
unsigned long interval = 3000; //interval between measurements 3000ms (3s), in principle can be longer or shorter (keep in mind loop takes about 2,5 s)

char filename111[19];    // character array variable to store our converted date string for SD.open()

void setup(void)
{ 
  RTC.begin(); // Initialize RTC module
  sensor1.begin(); // Start up the library 1 (temp sensor 1)
  sensor2.begin(); // Start up the library 2 (temp sensor 2)
  sensor3.begin(); // Start up the library 3 (temp sensor 3)
  sensor4.begin(); // Start up the library 4 (temp sensor 4)
  
  Serial.begin(9600);
  delay(500);
  
  sensor1.setResolution(10); //10 bit resolution (0.25°C step); (or 9 bit 0.5°C and faster reading)
  sensor2.setResolution(10); //10 bit resolution (0.25°C step); (or 9 bit 0.5°C and faster reading)
  sensor3.setResolution(10); //10 bit resolution (0.25°C step); (or 9 bit 0.5°C and faster reading)
  sensor4.setResolution(10); //10 bit resolution (0.25°C step); (or 9 bit 0.5°C and faster reading)
  
  // locate devices on the bus
  //Serial.println(F("Locating devices..."));
  //Serial.print(F("Sensor 1 status (1 - OK; 0 - not found) "));
  deviceCount = sensor1.getDeviceCount();
  //Serial.println(deviceCount, DEC);
  //Serial.print(F("Sensor 2 status (1 - OK; 0 - not found) "));
  deviceCount = sensor2.getDeviceCount();
  //Serial.println(deviceCount, DEC);  
  //Serial.print(F("Sensor 3 status (1 - OK; 0 - not found) "));
  deviceCount = sensor3.getDeviceCount();
  //Serial.println(deviceCount, DEC);  
  //Serial.print(F("Sensor 4 status (1 - OK; 0 - not found) "));
  deviceCount = sensor4.getDeviceCount();
  //Serial.println(deviceCount, DEC);  
  //Serial.println("");

//Now scan i2C bus for adresses
  //Wire.begin(); // for i2C_scaner
  //Serial.println(F("We expect on I2C bus:"));
  //Serial.println(F("0x26 - display1; 0x27 - display2"));
  //Serial.println(F("0x57 - EPROM; 0x68 - Clock DS3231"));
  //Serial.println(F("Scanning..."));
  for (byte address = 1; address < 127; ++address) {
    // The i2c_scanner uses the return value of
    // the Wire.endTransmission to see if
    // a device did acknowledge to the address.
    Wire.beginTransmission(address);
    byte error = Wire.endTransmission();

    if (error == 0) {
      if (OUT == 1) 
      {
      Serial.print(F("I2C device found at address 0x"));
      }
      if (address < 16) {
        if (OUT == 1) 
        {
        Serial.print("0");
        }
      }
      if (OUT == 1) 
      {
      Serial.print(address, HEX);
      Serial.println("  !");
      }
      
      ++I2CDevices;
    } else if (error == 4) {
      Serial.print(F("Unknown error at address 0x"));
      if (address < 16) {
        Serial.print("0");
      }
      Serial.println(address, HEX);
    }
  }
  if (I2CDevices == 0) {
    Serial.println(F("No I2C devices found\n"));
  } else {
    if (OUT == 1) 
    {
    Serial.println(F("Scanning done - did it found expected devices?\n"));
    }
  }
//Pin for the switch pins  
  pinMode(OnOff, INPUT);
//  
// Display part-----------------------------------------------------------------------------------
  lcd1.init();                     // initialize the lcd1
  lcd2.init();                     // initialize the lcd2
  lcd1.backlight();
  lcd2.backlight();
//--end of LCD----

//Clock part - for setting absolute time on your RTC module
  ///URTCLIB_WIRE.begin();
  // Comment out below line once you set the date & time.
  // Following line sets the RTC with an explicit date & time
  // for example to set May 06 2023 at 13:44 you would call:
//    RTC.setHourMode(CLOCK_H24);
//    RTC.setDay(11);
//    RTC.setMonth(5);
//    RTC.setYear(2023);
//    RTC.setHours(23);
//    RTC.setMinutes(34);
//    RTC.setSeconds(0);
//end clock 

//SD card module-----------------------------------------------------------------------------------
  if (!sd.begin(CS)) 
  { // Initialize SD card
    Serial.println(F("No SD card found. Reset the device after inserting an SD card.")); // if return value is false, something went wrong.
  while (1) {}
  }

//
//Starting timer for the elapsed time
  startTime = millis();
  if (OUT == 1) 
  {  
  Serial.println (startTime/1000);
  }
  
  //Fill variables with absolute time at the time Logger powered or starting to writte in to file
  Year_ = GetYEAR2();  //Uses function GetYEAR2
  Month_ = RTC.getMonth();
  Day_ = RTC.getDay();
  Hours_ = RTC.getHours();
  Minutes_ = RTC.getMinutes();
  Seconds_ = RTC.getSeconds();
  ////////////////////////////
  
//  Serial.println(Year_);
//  Serial.println(Month_);
//  Serial.println(Day_);
//  Serial.println(Hours_);
//  Serial.println(Minutes_);
//  Serial.println(Seconds_);
  delay(1000);
  if (OUT == 1) 
  { 
  Serial.println(F("Time from upload or powering the device:")); 
  Serial.println(GetNoSecPower());
  }
  //  
}
//
void loop()
{ DeltaT = millis();// serves to make sure we have data acquisition period 3 s
  PrintLCD2(); //update time on LCD
  if (OUT == 1) 
  { 
    Serial.print(F("I2C device found at address 0x"));
    Serial.println(F("Read State"));
  }
  ReadState();
  if (OUT == 1)
  {
  Serial.println(F("filename111 after ReadState in loop"));
  Serial.println(filename111);  
  Serial.println(F("Read Sensors"));
  }
  ReadSensors();
  if (OUT == 1)
  {  
  Serial.println(F("Print to LCD"));
  }
  PrintLCD1();
  PrintLCD2(); //update time on LCD
  if (OUT == 1)
  {   
  Serial.println(F("Read Time"));
  Serial.println(F("filename111 before readtime in loop"));
  Serial.println(filename111); 
  Serial.println (strlen (filename111));
  } 

  if(WritingEnabled == true)//if writing enabled write data to file
  {
    if (OUT == 1)
    {   
    Serial.println(F("In loop writing is enabled -> WriteSD"));
    WriteSD();
    }
  }
  PrintLCD2(); //update time on LCD
  DeltaT = millis() - DeltaT;
    if (OUT == 1)
    {
    Serial.println(DeltaT);
    }
  delay(interval - DeltaT); //
  //delay(5000);
}
//
// Function that reads temperature values from sensors
void ReadSensors()
{  
   elapsedTimeRTC = GetNoSecPower();
   if (OUT == 1)
   { 
   Serial.print(F("Print elapsedTimeRTC:"));
   Serial.println(elapsedTimeRTC);
   }

//  //Collect the values for each sensor    
    sensor1.requestTemperatures(); //request the temperature
    sensor2.requestTemperatures(); //request the temperature
    sensor3.requestTemperatures(); //request the temperature
    sensor4.requestTemperatures(); //request the temperature
//Filling up the variables
    T1 = sensor1.getTempCByIndex(0);  // put zero as index - one sensor per wire!!!!
    T2 = sensor2.getTempCByIndex(0);  
    T3 = sensor3.getTempCByIndex(0);  
    T4 = sensor4.getTempCByIndex(0); 
    if (OUT == 1)
    { 
    Serial.println(F("Temperature readings:"));
    }
    Serial.print(F("T1[°C]:"));
    Serial.print(T1,DEC);
    Serial.print(F(","));
    Serial.print(F("T2[°C]:"));
    Serial.print(T2,DEC);
    Serial.print(F(","));
    Serial.print(F("T3[°C]:"));
    Serial.print(T3,DEC);
    Serial.print(F(","));
    Serial.print(F("T4[°C]:"));
    Serial.println(T4,DEC);
    delay(50);          
}

void PrintLCD1()
{ 
  //16x2 LCD
  //1st LCD for temperatures
  lcd1.setCursor(0, 0); //The cursor's unit is in pixels and not in blocks as in the case of the 16x2 LCD
  lcd1.print(F("1:"));
  lcd1.print(T1,1);
  lcd1.print(F("C  2:"));
  lcd1.print(T2,1);
  lcd1.println(F("C"));
  lcd1.setCursor(0, 1);
  lcd1.print(F("3:"));
  lcd1.print(T3,1);
  lcd1.print(F("C  4:"));
  lcd1.print(T4,1);
  lcd1.println(F("C"));
}

void PrintLCD2()
{ 
  //16x2 LCD
  //1st LCD for temperatures
  lcd2.clear();
  lcd2.setCursor(0, 0); 
  lcd2.print("t: ");
  lcd2.print(elapsedTimeRTC);  
  lcd2.print(" s");

  if(WritingEnabled == true)
  {
    lcd2.setCursor(14, 0); //this display 
    lcd2.print("W"); // W = writing is in progress
  }
  else
  {
    lcd2.setCursor(14, 0); 
    lcd2.print("0"); // 0 = no writing at the moment    
  } 
    lcd2.setCursor(0, 1);
    lcd2.print(GetYEAR2());
    lcd2.print("/");
    lcd2.print(RTC.getMonth());
    lcd2.print("/");
    lcd2.print(RTC.getDay());
    lcd2.print(" ");
    lcd2.print(RTC.getHours());
    lcd2.print(":");
    lcd2.print(RTC.getMinutes());
    lcd2.print(":");
    lcd2.print(RTC.getSeconds());
    lcd2.print("   ");
}

void WriteSD()
{ 
  if (OUT == 1)
  {
  Serial.println(F("In SD Function"));
  Serial.println(filename111);
  Serial.println (strlen (filename111));
  }
  if (!file.open(filename111, O_APPEND | O_WRITE))
      { 
      //close the file, create new file name,open new file with date//
      Serial.println(F("Write to file failed"));
      while(1) {}
      }
      file.print(RTC.getYear());
      file.print(".");
      file.print(RTC.getMonth());
      file.print(".");
      file.print(RTC.getDay());
      file.print(";");
      file.print(RTC.getHours());
      file.print(":");
      file.print(RTC.getMinutes());
      file.print(":");
      file.print(RTC.getSeconds());
      file.print(";");
      file.print(elapsedTimeRTC);
      file.print(";");
      file.print(T1);
      file.print(";");
      file.print(T2);
      file.print(";");
      file.print(T3);
      file.print(";");
      file.print(T4);
      file.print(";");
      file.println();
      file.close();
}

void ReadState()
{  
  if (OUT == 1)
  {
  Serial.println(F("Now we are in ReadState"));
  }
if(digitalRead(OnOff) == HIGH) //if the button is high
  {
    if (OUT == 1)
    { 
    Serial.println(F("Now THE sw is HIGH (OnOFF is high)"));
    }
  if(SwitchStatus == false) //if the previous status was false, we restart the timer to 0 by resetting the startTime.
  {
    if (OUT == 1)
    {
    Serial.println(F("Now the switch was previously OFF - creating file."));
    }
    SwitchStatus = true; //flip the state
    startTime = millis(); //reset the timer to zero
    
    //Here, time of reset is actualised
    Year_ = GetYEAR2();
    Month_ = RTC.getMonth();
    Day_ = RTC.getDay();
    Hours_ = RTC.getHours();
    Minutes_ = RTC.getMinutes();
    Seconds_ = RTC.getSeconds();
    
    getfilename111();// here we create file name
    if (OUT == 1)
    {
    Serial.println(F("filename111 after getfilename111 in ReadState"));
    Serial.println(filename111); 
    }
    if (!file.open(filename111, O_WRITE | O_CREAT))
    { 
    //close the file, create new file name,open new file with date//
    if (OUT == 1)
    {
    Serial.println(F("Write to file failed"));
    Serial.println();
    }
    while(1) {}
    }
    file.println(F("DiscoLapy Temperature Logger v0 - Header"));
    file.print(F("Abs Date; Abs time; Elapsed time [s];T1[deg C];T2[deg C];T3[deg C];T4[deg C]"));
    file.println();
    file.close();
    //Test if file exists
    if (sd.exists(filename111)) 
    {
      if (OUT == 1) 
      {
      Serial.print(F("The file: "));
      Serial.print(filename111);
      Serial.print(F(" already exists.")); 
      }
    } 
  }
  WritingEnabled = true; //enable the writing
  if (OUT == 1)
  {
  Serial.println("ON"); //message for checking the things out
  }
}
else //in this case, the OnOff is LOW, which means, we don't write the SD card
{ 
  if (OUT == 1)
  {
  Serial.println(F("Now OnOff is LOW; SwitchStatus, WriringEnabled to FALSE"));
  }
  SwitchStatus = false; //Switch status is off
  WritingEnabled = false; //we do not write on the SD card
  if (OUT == 1)
  {
  Serial.println("OFF"); //message on the serial, to see what is happening
  }
}  
}

// creates a new file name with today's date
void getfilename111() {
  sprintf(filename111, "%02d%02d%02d%02d%02d%02d.csv",RTC.getYear(), RTC.getMonth(), RTC.getDay(), RTC.getHours(), RTC.getMinutes(),RTC.getSeconds());
  if (OUT == 1)
  {
  Serial.println(F("filename111 is"));
  Serial.println(filename111);
  }
}

// get two digit year from RTC module (from e.g. 2025 -> 25)
int GetYEAR2()
{
    int YEAR2 = RTC.getYear() - floor(RTC.getYear()/100.0)*100.0;
    
    return YEAR2;
}

// return number of seconds from powering the arduino
int GetNoSecPower()
{
    int NoSecPower = ((RTC.getYear() - floor(RTC.getYear()/100.0)*100.0) - Year_)  * 365 * 24 * 60 * 60;
    NoSecPower = NoSecPower + (RTC.getMonth() - Month_) * 30 * 24 * 60 * 60;//here one can do something about leap year
    NoSecPower = NoSecPower + (RTC.getDay() - Day_) * 24 * 60 * 60; 
    NoSecPower = NoSecPower + (RTC.getHours() - Hours_) * 60 * 60; 
    NoSecPower = NoSecPower + (RTC.getMinutes() - Minutes_) * 60;
    NoSecPower = NoSecPower + (RTC.getSeconds() -Seconds_);   
    return NoSecPower;
}
