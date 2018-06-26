#include <SHT1x.h>
#include <Wire.h>
#include <DFRobot_RGBLCD.h>
#include <GravityRtc.h>

const unsigned short  SolenoidOnePin=8;

const unsigned short  WaterPumpOnePin=10;
const unsigned short  WaterPumpTwoPin=11;
const unsigned short  WaterPumpThreePin=12;
const unsigned short  WaterPumpFourPin=13;

const unsigned short  TempHumidityDataPin=6;
const unsigned short  TempHumidityClockPin=7;

const unsigned short  WaterLevelSensorOnePin=2;
const unsigned short  WaterLevelSensorTwoPin=3;
const unsigned short  WaterLevelSensorThreePin=4;
const unsigned short  WaterLevelSensorFourPin=5;

int r,g,b;
int t = 0;
float temperature_c;
float temperature_f;
float humidity;
int tankOneWaterLevel = 0;
int tankTwoWaterLevel = 0;
int tankThreeWaterLevel = 0;
int tankFourWaterLevel = 0;
bool pumping = false;
bool filling = false;
int lastFillingHour = 0;
int lastFillingMinute = 0;
int fillingHour = 0;
int fillingMinute = 0;
int fillingMaxDuration = 30;
int fillingMaxFreqHours = 6;

GravityRtc rtc;             //RTC Initialization
DFRobot_RGBLCD lcd(16,2);   //16 characters and 3 lines of show
SHT1x sht1x(TempHumidityDataPin, TempHumidityClockPin);

void setup() {

  pinMode(WaterLevelSensorOnePin,INPUT);
  pinMode(WaterLevelSensorTwoPin,INPUT);
  pinMode(WaterLevelSensorThreePin,INPUT);
  pinMode(WaterLevelSensorFourPin,INPUT);
  
  pinMode(WaterPumpOnePin, OUTPUT);
  pinMode(WaterPumpTwoPin, OUTPUT);
  pinMode(WaterPumpThreePin, OUTPUT);
  pinMode(WaterPumpFourPin, OUTPUT);

  pinMode(SolenoidOnePin, OUTPUT);
  
  Serial.begin(9600);

  rtc.setup();
  //Set the RTC time automatically: Calibrate RTC time by your computer time
  rtc.adjustRtc(F(__DATE__), F(__TIME__));
  
  
  
  // initialize the lcd screen
  lcd.init();

}

void loop() {

  char logFilename[26];
  sprintf(logFilename, "%s%d%d%d%s", "IrrigationLog_", rtc.year, rtc.month, rtc.day, ".log");

  Serial.print("Current log filename: ");
  Serial.println(logFilename);

  rtc.read();

  r= (abs(sin(3.14*t/180)))*255;          //get R,G,B value
  g= (abs(sin(3.14*(t+60)/180)))*255;
  b= (abs(sin(3.14*(t+120)/180)))*255;
  t=t+3;
  lcd.setRGB(r, g, b);                  //Set R,G,B Value
  lcd.setCursor(0,0);
  lcd.print("Todd's Irrig Sys");
  lcd.setCursor(0,1);
  //R:0-255 G:0-255 B:0-255
  
  char lcdOutput[16];
  if ( rtc.second % 2 == 0 )
  {
    sprintf(lcdOutput, "%s%d%s%d%s%d%s", "Date: ", rtc.month, "/", rtc.day, "/", rtc.year, "  ");
  }
  else
  {
    sprintf(lcdOutput, "%s%d%s%d%s%d%s", "Time: ", rtc.hour, ":", rtc.minute, ":", rtc.second, "  ");
  }
  lcd.print(lcdOutput);

  char currentTimestamp[19];
  sprintf(currentTimestamp, "%d%s%d%s%d%s%d%s%d%s%d", rtc.year, "-", rtc.month, "-", rtc.day, "T", rtc.hour, ":", rtc.minute, ":", rtc.second);

  Serial.print("Current timestamp: ");
  Serial.println(currentTimestamp);
  
  tankOneWaterLevel = digitalRead(WaterLevelSensorOnePin);
  tankTwoWaterLevel = digitalRead(WaterLevelSensorTwoPin);
  tankThreeWaterLevel = digitalRead(WaterLevelSensorThreePin);
  tankFourWaterLevel = digitalRead(WaterLevelSensorFourPin);

  // Read values from the sensor
  temperature_c = sht1x.readTemperatureC();
  temperature_f = sht1x.readTemperatureF();
  humidity = sht1x.readHumidity();

  
  
  Serial.print("Temperature (C): ");
  Serial.println(temperature_c);
  
  Serial.print("Temperature (F): ");
  Serial.println(temperature_f);
  
  Serial.print("Humidity: ");
  Serial.println(humidity);

  FillTanksIfNeeded();
  WaterIfNeeded();

  delay(1);
}

void FillTanksIfNeeded()
{
    if(tankOneWaterLevel == 0 || tankTwoWaterLevel == 0 || tankThreeWaterLevel == 0 || tankFourWaterLevel == 0)
    {
      Serial.println("One of the tanks is low on water...");
  
      Serial.print("Water Level of Tank 1: ");
      Serial.println(tankOneWaterLevel);
    
      Serial.print("Water Level of Tank 2: ");
      Serial.println(tankTwoWaterLevel);
  
      Serial.print("Water Level of Tank 3: ");
      Serial.println(tankThreeWaterLevel);
  
      Serial.print("Water Level of Tank 4: ");
      Serial.println(tankFourWaterLevel);

      StartFillingTanks();
    }
    else
    {
      StopFillingTanks();
    }

    if(lastFillingHour > 0)
    {
        Serial.print("The tanks last completed filling at: ");
        Serial.print(lastFillingHour);
        Serial.print(":");
        Serial.print(lastFillingMinute);
        Serial.println("...");
    }
            
    if(filling == true)
    {
      Serial.print("The tanks started filling at: ");
      Serial.print(fillingHour);
      Serial.print(":");
      Serial.print(fillingMinute);
      Serial.println("...");
 
      if(fillingHour > 0)
      {
        if(fillingHour == rtc.hour && ((fillingMinute + fillingMaxDuration) < rtc.minute))
        {
          Serial.println("The tanks have been filling for over 20 minutes. The water level sensors may not be working. Stopping the fill...");
          StopFillingTanks();
        }
        else if((fillingHour + 1) == rtc.hour && ((fillingMinute - 60 + fillingMaxDuration) < rtc.minute))
        {
          Serial.println("The tanks have been filling for over 20 minutes. The water level sensors may not be working. Stopping the fill...");
          StopFillingTanks();
        }
      }  
    }
}


void WaterIfNeeded()
{
  if(rtc.hour == 7)
  {
    //Water for 5 minutes to start.
    if(rtc.minute >= 0 and rtc.minute <= 5)
    {
      //Morning watering...
      StartPumpWater();
    }
    else
    {
      //Morning watering complete...
      StopPumpWater();
    }
  }
  else if(rtc.hour == 18)
  {
    //Water for 5 minutes to start.
    if(rtc.minute >= 0 and rtc.minute <= 5)
    {
      //Evening watering...
      StartPumpWater();
    }
    else
    {
      //Evening watering complete...
      StopPumpWater();
    }
  }
}


void StartFillingTanks()
{
  if(filling == false)
  {
    if(lastFillingHour == 0 || (lastFillingHour + fillingMaxFreqHours) < rtc.hour)
    {
      Serial.println("Filling tanks...");
      //digitalWrite(SolenoidOnePin, HIGH);
      Serial.println("Filling started...");
      filling = true;
      fillingHour = rtc.hour;
      fillingMinute = rtc.minute;     
    }
    else
    {
      Serial.println("The tanks were filled less than six hours ago, the tanks will not be filled yet.");
    }
  }
}

void StopFillingTanks()
{
  if(filling == true)
  {
    Serial.println("Stopping tank filling...");
    //digitalWrite(SolenoidOnePin, LOW);
    Serial.println("Filling stopped...");
    filling = false;
    lastFillingHour = fillingHour;
    lastFillingMinute = fillingMinute;
    fillingHour = 0;
    fillingMinute = 0;
  }
}

void StartPumpWater()
{
  if(pumping == false)
  {
    Serial.println("Starting pumps...");
    digitalWrite(WaterPumpOnePin, HIGH);
    digitalWrite(WaterPumpTwoPin, HIGH);
    digitalWrite(WaterPumpThreePin, HIGH);
    digitalWrite(WaterPumpFourPin, HIGH);
    Serial.println("Pumps started...");
    pumping = true;
  }
}

void StopPumpWater()//
{
  if(pumping == true)
  {
    Serial.println("Stopping pumps...");
    digitalWrite(WaterPumpOnePin, LOW);
    digitalWrite(WaterPumpTwoPin, LOW);
    digitalWrite(WaterPumpThreePin, LOW);
    digitalWrite(WaterPumpFourPin, LOW);
    Serial.println("Pumps stopped...");
    pumping = false;
  }
}
