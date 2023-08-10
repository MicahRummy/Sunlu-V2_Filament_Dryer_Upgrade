#pragma once

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "EEPROM.h"

#ifndef DryingFunctions.h
#define DryingFunctions.h
#endif

#define SCREEN_WIDTH 128 // display display width, in pixels
#define SCREEN_HEIGHT 64 // display display height, in pixels

#define display_RESET -1 // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, display_RESET);

//display functions:
void clearLine(int x, int multiplier){
  int s = x + (8 * multiplier);  
  for (x; x<s; x++){
    for (int y = 0; y<127; y++){
      display.drawPixel(y,x,BLACK);
    }
  }
}

void displayError(String msg){
  clearLine(0, 8);//leaves the last line available.
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0,0);
  display.print(msg);
  display.display();
}

void rewriteLine1(int x, String str){
    clearLine(x, 1);
    display.setCursor(0, x);
    display.setTextSize(1);
    display.print(str);
    display.display();
}

//Pin Definition Variables
  //24V pins
const int heatPin = 10;//D10 on Board
  //buttons
// const int button1Pin = 4;//D2 on Board
// const int button2Pin = 5;//D3 on Board
  //sensors
const int heatInPin = 3;//D1/A1 on Board
const int heatOutPin = 2;//D0/A0 on Board
  //Indicators
const int statusLed = 20;//D7 on Board
const int errorLed = 21;//D8 on Board

//serial Variables
const int baudrate = 9600;

//Events
int statusLedBlinkTime = 0;
int blinkRate = 0;

int errorLedBlinkTime = 0;
int errorBlinkRate = 0;

int countDown = 0;
int countDownEvent = 0;

int updateEvent = 0;
int updateTiming = 0;

int calibrateevent = 0;

int message_event = 0;

int status_event = 0;

//Values Variables
int material;
int runTime;
float heatTempOut = 0; //current temperature
float heatTempIn = 0; //current temperature
int heatTemp = 0; //average temp
int y;
int heatIncrement = 0;
int overflow = 5;
int sensorThreshold = 30;
int diff;
float realtemp;
int plate_limit = 95;

//Bools Variables
bool heatInDetected = false;
bool heatOutDetected = false;
bool heating = false;
bool countDownTriggered = false;
bool overflowWarning = false;
bool heatInError = false;
bool heatOutError = false;
bool generalError = false;
bool awaitUpdate = true;
bool calibration = false;
bool stable = false;
bool fan_override = false;
bool test_mode = false;
bool wait_answer_time = false;
bool wait_answer_limit = false;
bool disable_chat = false;
bool wait_answer_speed = false;

//Functions:

void clearEEPROM(){
  displayError("Erasing...");
  delay(1500);
  for (int i = 0; i < EEPROM.length(); i++){
    EEPROM.write(i, 0);
    display.print("erasing 0x");
    display.println(i, HEX);   
    display.display(); 
    delay(10);
  }  
  EEPROM.commit();
  displayError("All memory deleted.");
  delay(2000);
}

bool confirm(){
  disable_chat = true;
  message = "are you sure? (y or n)";
  while (message == "are you sure? (y or n)"){
    ArduinoCloud.update();
  }
  if (message == "y" || message == "Y" || message == "Yes" || message == "yes" || message == "YES"){
    return 1;  
  }
  else if (message == "n" || message == "N" || message == "No" || message == "no" || message == "NO"){
    return 0;
  }
  else{
    message = "invalid. Cancled.";
    ArduinoCloud.update();
    return 0;
  }
}

void onTempatureSetChange()  {
  // Add your code here to act upon TempatureSet change
  if (tempatureSet != 0){
    heating = true;
    awaitUpdate = false;
    display.clearDisplay();
    display.setTextSize(1);
    display.setCursor(0, 8);
    display.print("heating to ");
    display.print(tempatureSet);
    display.print(".");
    display.display();
    delay(3000);
  }
  else{
    heating = false;
    message = "Ready.";
    rewriteLine1(0, "heating turned off.");
    delay(3000);
  }
  ArduinoCloud.update();//update again incase set temperature incramented.
}

//when Time is set
void onSetTimeChange()  {
  // Add your code here to act upon SetTime change
  timeLeft = setTime / 60000 / 60;//show time left in hours.
  if (setTime > 0){
    countDown = setTime * 60000 * 60;//in hours
    countDownTriggered = true;
  }
  else{
    countDownTriggered = false;
    countDownEvent = 0;
    countDown = 0;
  }
  ArduinoCloud.update();//update again incase time incramented.
}

//Heat envirnment
void heatFilament (){
  //if heat is more than the set temp, but less than the overflow:
  if ((heatTempOut >= (tempatureSet + 1)) && (heatTempOut < tempatureSet + overflow)){
    heatIncrement--;
    overflowWarning =false;
    stable = false;
  }
  //is heat is less than what has been set:
  else if (heatTempOut < tempatureSet && heatTempIn <= plate_limit){
    heatIncrement++;
    overflowWarning =false;
    stable = false;
  }
  //if heat is more than the set temp plus 10:
  else if(heatTempOut >= (tempatureSet + overflow) || heatTempIn > plate_limit){
    heatIncrement = 0;
    overflowWarning =true;
    stable = false;
    message = "Warning: Heat Overflow";
    rewriteLine1(8, message);
  }
  //desired heat is reached:
  else{
    stable = true;
  }
  //adjust increment to be within parameter:
  if (heatIncrement > 255){
    heatIncrement = 255;
  }
  if (heatIncrement < 0){
    heatIncrement = 0;
  }

  //set blinkrates:
  if (overflowWarning){
    blinkRate = 100;
    errorBlinkRate = 100;
  }
  else if (stable){
    blinkRate = -1;
    rewriteLine1(8, "Heat is Stable.");
    // analogWrite(heatPin, 0);
  }
  else{
    blinkRate = 10;
    errorBlinkRate = 0;
    rewriteLine1(8, ("heating to " + String(tempatureSet)));
  }

  //Write the increment to pwm pin:
  analogWrite(heatPin, heatIncrement);
  awaitUpdate = false;
  updateTiming = 20000;//update cloud every 20 seconds
}

void updateCloud(){
  if (awaitUpdate == true && (runTime - updateEvent) >= updateTiming){
    if (ArduinoCloud.connected()){
      displayError("Waiting for command..");
      rewriteLine1(56, "updating...");
    }
    else if (!ArduinoCloud.connected()){
      displayError("Wifi not connected.");
    }
    // digitalWrite(statusLed, HIGH);
    // digitalWrite(errorLed, HIGH);
    // Serial.println("Updating...");
    ArduinoCloud.update();
    runTime = millis();
    updateEvent = runTime;
    rewriteLine1(56, message);
  }
  else if ((runTime - updateEvent) >= updateTiming){//updateTiming set to 15 seconds when heating starts
    if (!ArduinoCloud.connected()){
        displayError("Wifi not connected.");
      }
    updateEvent = runTime;
    // Serial.println("Updating...");
    rewriteLine1(56, "updating...");
    ArduinoCloud.update();
    rewriteLine1(56, message);
  }
}

//cools the envirnment
void coolDown(){
  analogWrite(heatPin, 0);
  tempatureSet = 0;
  if (heating){
    message = "Ready.";
    heating = false;
  }
  awaitUpdate = true;
  if (heatTemp > 35){
    blinkRate = 300;
    errorBlinkRate = 0;
    updateTiming = 3*1000;
    rewriteLine1(0, "Cooling Down...");
    rewriteLine1(8, "Still hot.");
  }
  else if (heatTemp <= 35){
    blinkRate = 0;
    errorBlinkRate = 0;
    updateTiming = 5000;//update every 5 seconds
  }
}

//Error Handling
void onError (String error){
  displayError("Error: " + error + " has occured.\n\nCooling down...");
  coolDown();
  message = "Error:\n" + error + " has occured.\nCooling down...\nHeat in Value:\t" + String(analogRead(heatInPin)) + "\nHeat out value:\t" +String(analogRead(heatOutPin));
  ArduinoCloud.update();
  blinkRate = 1000;//override blinkrate
  errorBlinkRate = -1;
}

//check sensors
void checkSensors(){
  if(!heatInDetected){
      // Serial.println("Bad In");
      onError("bad sensor inside");
      generalError = true;
    }
    else{
      // Serial.print("Good in");
      // Serial.println(analogRead(heatInPin));
      generalError = false;
    }
    if(!heatOutDetected){
      // Serial.println("Bad out");
      onError("bad sensor outside");
      generalError = true;
    }
    else{
      // Serial.println("Good out");
      generalError = false;
    }
}

//digital pin signal
void reverseSignal(int pin, int eventTime, int &eventTimer){
  if ((runTime - eventTimer) > eventTime){
    if (digitalRead(pin) == LOW){
      digitalWrite(pin, HIGH);
    }
    else{
      digitalWrite(pin, LOW);
    }
  eventTimer = runTime;
  }
}

//blink LED
void blink(int pin, int rate, int &event){
  if (rate >= 10){
  reverseSignal(pin, rate, event);
  }
  else if (rate == -1){
    digitalWrite(pin, HIGH);
  }
  else {
    digitalWrite(pin, LOW);
  }
  }


//Check if thermistors exist
bool checkHeatPin(int pin){
  int heatSensorValues[sensorThreshold];
  for (int i = 0; i < sensorThreshold; i++){
    heatSensorValues[i] = analogRead(pin);
    // Serial.println(heatSensorValues[i]);
  }
  int maxV = 0;
  int minV = 4096;
  for (int i = 0; i<sensorThreshold; i++){
    if (heatSensorValues[i]<minV){
      minV = heatSensorValues[i];
    }
    else if(heatSensorValues[i]>maxV){
      maxV = heatSensorValues[i];
    }
  }
  if  (minV == 0 || maxV == 4095){//removed:  (maxV-minV) > 30 || 
    return false;
  }
  else{
    return true;
  }
}