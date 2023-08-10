// #ifndef Temperature.h
// #define Temperature.h
// #endif

#include <math.h>
#include "DryingFunctions.h"

int thermistor;

int requestThermistor(){
  displayError("Calibrating...");
  message = "what thermistor? (1 = out, 2 = in)";
  runTime = millis();
  calibrateevent = runTime;
  while (message == "what thermistor? (1 = out, 2 = in)" && (runTime - calibrateevent) < 120000){//2 minute timeout
    ArduinoCloud.update();//force update
    runTime = millis();
  }
  
  if (runTime - calibrateevent > 120000){
    message = "canceled";
    return -1;
  }
  else if (message == "cancel"){
    message = "canceled";
    return -1;
  }
  else if (message.toInt() == 0){
    return 10;
  }

    else{
      displayError(message);
      return message.toInt();
    }
  }


float requestRealTemp(){
  message = "What is current temp?";
  runTime = millis();
  calibrateevent = runTime;
  while (message == "What is current temp?" && (runTime - calibrateevent) < 120000){//2 minute timeout
    ArduinoCloud.update();//force update
    runTime = millis();
  }
  ArduinoCloud.update();//force update
  if (runTime - calibrateevent > 120000){
    message = "canceled";
    return 0;
  }
  else if (message == "cancel"){
    message = "canceled";
    return 0;
  }
  else if (message.toFloat() < 10){
    message = "unexpected temperature.";
    return 0;
  }
  
  float ctemp = message.toFloat();
  message = "calibrated";
  ArduinoCloud.update();//force update
  return ctemp;
}

class Temperature{
  private:
    byte addr;
    float b;
    float r1;
    float t;
    int pin;

  public:
    Temperature(byte saddr, float sb, float sr1, float st, int spin){
        addr = saddr;  
        b = sb;
        r1 = sr1;
        t = st;        
        pin = spin;   
        pinMode(pin, INPUT);   
    }
    Temperature(){
      addr = 0;
      b = 3850;
      r1 = 60000;
      t = 300;
      pin = 0;
    }

    bool operator==(const Temperature& rhs)
    const;

    float printb(){
      return b;
    }
    
    float R1 = 100000;//Value of R1 resistor (default)

    float getTemp(int r2){
      float temp = ((t * b) / (t * log(r2 / r1)+b))-273.15;
      return temp;
    }

    float getResistance(){
      int value = analogRead(pin);
      float resistance = abs((value*R1)/(4095-value));//Value of R2
      return resistance;
    }

    void calibrate(float realtemp){
      if (realtemp == 0){
        displayError("Canceled.");
        delay(10000);
        return;
      }
      realtemp = realtemp + 273.15;//convert celsius to kelvin
      float oldb = b;
      b = log(r1/getResistance()) * pow(pow(t, -1) - pow(realtemp, -1), -1);
      if (b < 0){
        b = oldb;
        displayError("Error, b cant be negative.");
        delay(10000);
        return;
      }
      t = realtemp;
      r1 = getResistance();
  }

  int getAddress(){
  return addr;
  }

  void changeB(float B){
    b = B;
  }
};

bool Temperature::operator==(const Temperature& rhs)
const{
  return b == rhs.b && r1 == rhs.r1 && t == rhs.t;
}