//SETUP:

#include <HX711_ADC.h>
#if defined(ESP8266)|| defined(ESP32) || defined(AVR)
#include <EEPROM.h>
#include <TimeLib.h>
#endif

//pins:
const int HX711_dout = 4; //mcu > HX711 dout pin
const int HX711_sck = 5; //mcu > HX711 sck pin

//HX711 constructor:
HX711_ADC LoadCell(HX711_dout, HX711_sck);



const int calVal_eepromAdress = 0;
unsigned long t = 0;
int timeNow;
int feedingTimes[] = {};
int numberInArray = 0;
int feedingActive = 0;

//ajutised muutujad:
int rawHours[] = {"8", "15", "20"};
int rawMinutes[] = {"00", "30", "45"};



void setup() {
  Serial.begin(57600); delay(10);

  //KATSETAMINE:

  //createFeedingTime(rawHours[1], rawMinutes[1]);
  //Serial.println(feedingTimes[1]);
  feedingTimes[0] = {28800};
  feedingTimes[1] = {55800};
  feedingTimes[2] = {74700};
  numberInArray = 3;


  LoadCell.begin();
  //LoadCell.setReverseOutput(); //uncomment to turn a negative output value to positive
  unsigned long stabilizingtime = 2000; // preciscion right after power-up can be improved by adding a few seconds of stabilizing time
  boolean _tare = true; //set this to false if you don't want tare to be performed in the next step
  LoadCell.start(stabilizingtime, _tare);
  if (LoadCell.getTareTimeoutFlag() || LoadCell.getSignalTimeoutFlag()) {
    Serial.println("Timeout, check MCU>HX711 wiring and pin designations");
    while (1);
  }
  else {
    LoadCell.setCalFactor(1.0); // user set calibration value (float), initial value 1.0 may be used for this sketch
    Serial.println("Startup is complete");
  }
  while (!LoadCell.update());
  calibrate(); //start calibration procedure
}

void loop() {
  timeNow = 3600 * hour() + 60 * minute() + second();
  float i = LoadCell.getData();

  Serial.println(i);
  //TIME CHECK
  for (int i = 0; i < numberInArray; i++) {
    if (feedingTimes[i] == timeNow) {
      feedingActive = 1;
    }
  }

  //FEEDING ACTIVE
  if (feedingActive == 1) {

  }


  // receive command from serial terminal
  if (Serial.available() > 0) {
    char inByte = Serial.read();
    if (inByte == 't') LoadCell.tareNoDelay(); //tare
    //else if (inByte == 'r') calibrate(); //calibrate
  }

  // check if last tare operation is complete
  if (LoadCell.getTareStatus() == true) {
    Serial.println("Tare complete");
  }

}

void calibrate() {
  Serial.println("***");
  Serial.println("Start calibration:");
  Serial.println("Place the load cell an a level stable surface.");
  Serial.println("Remove any load applied to the load cell.");
  Serial.println("Send 't' from serial monitor to set the tare offset.");

  boolean _resume = false;
  while (_resume == false) {
    LoadCell.update();
    if (Serial.available() > 0) {
      if (Serial.available() > 0) {
        char inByte = Serial.read();
        //if (inByte == 't') LoadCell.tareNoDelay();
      }
    }
    if (LoadCell.getTareStatus() == true) {
      Serial.println("Tare complete");
      _resume = true;
    }
  }

  Serial.println("Now, place your known mass on the loadcell.");
  Serial.println("Then send the weight of this mass (i.e. 100.0) from serial monitor.");

  float known_mass = 0;
  _resume = false;
  while (_resume == false) {
    LoadCell.update();
    if (Serial.available() > 0) {
      known_mass = Serial.parseFloat();
      if (known_mass != 0) {
        Serial.print("Known mass is: ");
        Serial.println(known_mass);
        _resume = true;
      }
    }
  }

  LoadCell.refreshDataSet(); //refresh the dataset to be sure that the known mass is measured correct
  float newCalibrationValue = LoadCell.getNewCalibration(known_mass); //get the new calibration value

  Serial.print("New calibration value has been set to: ");
  Serial.print(newCalibrationValue);
  Serial.println(", use this as calibration value (calFactor) in your project sketch.");
  Serial.print("Save this value to EEPROM adress ");
  Serial.print(calVal_eepromAdress);
  Serial.println("? y/n");

  _resume = false;
  while (_resume == false) {
    if (Serial.available() > 0) {
      char inByte = Serial.read();
      if (inByte == 'y') {
#if defined(ESP8266)|| defined(ESP32)
        EEPROM.begin(512);
#endif
        EEPROM.put(calVal_eepromAdress, newCalibrationValue);
#if defined(ESP8266)|| defined(ESP32)
        EEPROM.commit();
#endif
        EEPROM.get(calVal_eepromAdress, newCalibrationValue);
        Serial.print("Value ");
        Serial.print(newCalibrationValue);
        Serial.print(" saved to EEPROM address: ");
        Serial.println(calVal_eepromAdress);
        _resume = true;

      }
      else if (inByte == 'n') {
        Serial.println("Value not saved to EEPROM");
        _resume = true;
      }
    }
  }

  Serial.println("End calibration");
  Serial.println("***");
  Serial.println("To re-calibrate, send 'r' from serial monitor.");
  Serial.println("For manual edit of the calibration value, send 'c' from serial monitor.");
  Serial.println("***");
}



//CREATE FEEDING TIME
int createFeedingTime(char* hoursChar, char* minutesChar) {
  //int hoursInt = hoursChar.toInt();
  //int minutesInt = minutesChar.toInt();
  int hoursInt = 0;
  int minutesInt = 0;

  Serial.println(hoursInt);
  Serial.println(minutesInt);
  int newTime = hoursInt * 3600 + minutesInt * 60;
  if (newTime < 86399) {
    feedingTimes[numberInArray] = newTime;
    Serial.println("Uus kellaaeg on lisatud!");
    numberInArray += 1;
  }
  else{
    Serial.println("Antud kellaaeg ei sobi!");
  }
}