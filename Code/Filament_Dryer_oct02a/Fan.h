#pragma once

class Fan{
  private:
    int pin;

    bool fan_status = false;
    bool pwm_on = false;

  public:
    int fan_speed;

    Fan(int pin){
      this->pin = pin;
      pinMode(pin, OUTPUT);
    }

    void fanOn();
    void fanOff();
    void fanTrigger();

    void fanPWM();
    void incrementSpeed();
    void deIncrementSpeed();

    bool fanStatus();
    bool fanPWMStatus();
};

void Fan::fanOn(){
  digitalWrite(pin, HIGH);
  fan_status = true;
  pwm_on = false;
  fan_speed = 0;
}

void Fan::fanOff(){
  digitalWrite(pin, LOW);
  pwm_on = false;
  fan_speed = 0;
  fan_status = false;
}

void Fan::fanTrigger(){
  if (fan_status){
    fanOff();
  }
  else{
    fanOn();
  }
}

void Fan::incrementSpeed(){
  fan_speed = fan_speed++;
}

void Fan::deIncrementSpeed(){
  fan_speed = fan_speed--;
}

void Fan::fanPWM(){
  analogWrite(pin, fan_speed);
  if (fan_speed > 0 && fan_speed < 51){
    fan_speed = 51;
    pwm_on = true;
  }
  if (fan_speed > 0){
    fan_status = true;
    pwm_on = true;
  }
  else {
    fan_status = false;
    pwm_on = false;
    }
}

bool Fan::fanStatus(){
  return fan_status;
}
bool Fan::fanPWMStatus(){
  return pwm_on;
}

