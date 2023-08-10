#include "Button.h"
#include "arduino_secrets.h"
#include "thingProperties.h"
//Hidden Variables:
  //    String message;
  //   int cTemperature;
  //   int setTime;
  //   int tempatureSet;
  //   CloudTime timeLeft;

#include "DryingFunctions.h"
#include "Temperature.h"
#include "Fan.h"

//buttons
  //10s reset function:
  void reset(){
    ESP.restart();
  }

  Button button1(4, reset, 3);
  Button button2(5, reset, 3);

  Fan fan1(9);
  Fan fan2(8);

//thermistors:
  Temperature dummy;
  Temperature sensorout(0, 3950, 100000, 298.15, heatOutPin);
  Temperature sensorin(sizeof(sensorout)+sensorout.getAddress(), 3950, 100000, 298.15, heatInPin);

void setup() {
  // Initialize serial and wait for port to open:
  Serial.begin(baudrate); 

  //initialize eeprom
  EEPROM.begin(512);

  // Output pins
  pinMode(heatPin, OUTPUT);
  pinMode(statusLed, OUTPUT);
  pinMode(errorLed, OUTPUT);
  // input pins
  // pinMode(heatInPin, INPUT);
  // pinMode(heatOutPin, INPUT);

  // ensure that heat and fan are off at start.
  analogWrite(heatPin, 0);

  // start Runtime variable
  runTime = millis();

  //set up OLED Startup Screen
  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    displayError("SSD1306 allocation failed");
    for(;;); // Don't proceed, loop forever
  }

  // Clear the buffer.
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(10,0);
  display.println("Lets heat filament!");
  display.display();
  
  //statusLed blink for 1 second
  for (int i =0; i < 10; i++){
    digitalWrite(statusLed, HIGH);
    delay(100);
    digitalWrite(statusLed, LOW);
    delay(100);
  }

  //get last saved in EEPROM:
  displayError("getting from memory:\n");

//sensor in:
  display.println("Inside Sensor:");
  EEPROM.get(sensorin.getAddress(), dummy);

  if (!dummy.printb() || isnan(dummy.printb())){
    EEPROM.put(sensorin.getAddress(), sensorin);
    display.print("Success put! ");
    display.println(dummy.printb());
  }
  else{
    EEPROM.get(sensorin.getAddress(), sensorin);
    display.print("Success get!  ");
    display.println(sensorin.printb());
  }

//sensor out:
  display.println("outside sensor:");
  EEPROM.get(sensorout.getAddress(), dummy);

  if (!dummy.printb() || isnan(dummy.printb())){
    EEPROM.put(sensorout.getAddress(), sensorout);
    display.print("Success put!  ");
    display.println(dummy.printb()); 
  }
  else{
    EEPROM.get(sensorout.getAddress(), sensorout);
    display.print("Success get!  ");
    display.println(sensorout.printb());
  }
//commit, start booting.
  EEPROM.commit();
  display.display();
  delay(10000);

  
  // Defined in thingProperties.h
  initProperties();

  // Connect to Arduino IoT Cloud
  ArduinoCloud.begin(ArduinoIoTPreferredConnection);
  /*
     The following function allows you to obtain more information
     related to the state of network and IoT Cloud connection and errors
     the higher number the more granular information youâll get.
     The default is 0 (only errors).
     Maximum is 4
 */
  setDebugMessageLevel(2);
  ArduinoCloud.printDebugInfo();
  display.clearDisplay();
  display.display();
  message = "Ready.";
}

void loop() {
  //get current run time
    runTime = millis();

  //check for heat sensors
    heatOutDetected = checkHeatPin(heatOutPin);
    heatInDetected = checkHeatPin(heatInPin);

  //blink LED
    // blink(statusLed, blinkRate, statusLedBlinkTime);//statusLed not working
    blink(errorLed, errorBlinkRate, errorLedBlinkTime);

  if (!test_mode){//test mode bypasses error handling.

    //if sensors dont exit, terminate code, shut off heat
      checkSensors();

    // if an error occured, dont proceed.
      if (generalError){
        return;
      }
  }

    //code below runs if no error occured.

  //get updates
    updateCloud();

  //time setting
    if (wait_answer_time){
        disable_chat = true;
        while (message == "How many hours?" || (millis() - message_event) > 20000){//20 second timeout
          ArduinoCloud.update();
        }
        wait_answer_time = false;
        if (message.toInt() > 0){
          countDown = message.toInt() * 3600000;// in millis
          timeLeft = countDown / 3600000 ;//in hours
          countDownTriggered = true;
          message = "time set.";
          ArduinoCloud.update();
          countDownEvent = millis();
        }
        else if (millis() - message_event > 20000){
          message = "Timeout occured.";
        }
        else if (message == "0"){
          countDown = 0;
          timeLeft = countDown;
          countDownTriggered = false;
          message = "time removed.";
          ArduinoCloud.update();
        }
        else{
          message = "Invalid Time.";
        }
      }

  //Limit Setting
    if (wait_answer_limit){
        disable_chat = true;
        while (message == "New Plate Max temp?" || (millis() - message_event) > 20000){//20 second timeout
          ArduinoCloud.update();
        }
        wait_answer_limit = false;
        if (message.toInt() > 60){
          plate_limit = message.toInt();
          message = "Limit set to " + message + ".";
        }
        else if (millis() - message_event > 20000){
          message = "Timeout occured.";
        }
        else{
          message = "Invalid input.";
        }
      }

  //Speed Setting
    if (wait_answer_speed){
        disable_chat = true;
        while (message == "New Fan speed?" || (millis() - message_event) > 20000){//20 second timeout
          ArduinoCloud.update();
        }
        wait_answer_speed = false;
        if (message.toInt() > 0){
          fan1.fan_speed = map(message.toInt(), 0, 100, 0, 255);
          fan2.fan_speed = map(message.toInt(), 0, 100, 0, 255);
          fan1.fanPWM();
          fan2.fanPWM();
          fan_override = true;
          message = "Fan Override on. Fan at " + message + "%.";
        }
        else if (millis() - message_event > 20000){
          message = "Timeout occured.";
        }
        else{
          message = "Invalid input.";
        }
      }

  //calibration
    if (calibration){
      thermistor = requestThermistor();//get which thermistor.
      delay(3000);
      //interpret reading
        //if cancel code sent
        if (thermistor < 0){
          message = "Cancled.";
          displayError(message);
          delay(10000);
          ArduinoCloud.update();
          calibration = false;//reset calibration
          return;
        }
        else if (thermistor == 1){
          dummy = sensorout;
        }
        else if (thermistor == 2){
          dummy = sensorin;
        }
        //if input is not as expected:
        else{
          displayError("Invalid input.");
          message = "Invalid input.";
          ArduinoCloud.update();
          calibration = false;//reset calibration
          return;
        }

      realtemp = requestRealTemp();
      dummy.calibrate(realtemp);//changes b value
      EEPROM.put(dummy.getAddress(), dummy);//saves the object
      EEPROM.commit();
      rewriteLine1(8, "New B value = ");
      display.println(String(dummy.printb()));
      display.println("Resistance: = ");
      display.println(String(dummy.getResistance()));
      display.display();
      delay(10000);
      getAllThermistors();//retrieves new values to propper variables
      rewriteLine1(56, "Ready.");
      calibration = false;//reset calibration
    }
    
  //read temperature:
    heatTempOut = sensorout.getTemp(sensorout.getResistance());
    heatTempIn = sensorin.getTemp(sensorin.getResistance());
    // heatTemp = (heatTempOut + heatTempIn) / 2;//rounded temperature - averaged
    heatTemp = heatTempOut;//to get an integer instead of a float
    cTemperature = heatTemp;//to read temp on arduino cloud dashboard

  //constantly update temperature
    clearLine(18, 2);
    display.setTextSize(2);
    display.setCursor(30,20);
    // y = round(heatTemp);
    // display.print(y);
    display.print(heatTemp);
    display.print("C");
    display.display();
    

  //cooldown
    if (!heating){
      if (!fan_override){      
          if (heatTempIn >= 50){
            fan1.fanOn();
            fan2.fanOn();
          }
          else{
            fan1.fanOff();
            fan2.fanOff();
          }
        }

      coolDown();
      button2.attach(switchMaterial);
    }
  //heat filament
    else{
      //Fan speeds
        if (!fan_override){      
          diff = (heatTempIn-heatTempOut);
          if (diff > 5 || overflowWarning){//if the difference between thermistors is too great
            fan1.fanOn();
            fan2.fanOn();
          }
          else if (diff < 1){
            fan1.fanOff();
            fan2.fanOff();
          }
          else{
            fan1.fan_speed = map(diff, 0, 5, 0, 255);
            fan2.fan_speed = map(diff, 0, 5, 0, 255);
            fan1.fanPWM();
            fan2.fanPWM();
          }
        }

      //status event
      if ((millis() - status_event) > 60000){//update status every minute{
        status_event = millis();
        sendStatus();
      }

      //heat function & buttons
      heatFilament();
      button1.attach(coolDown);
      button2.detach(0);
    }

  //count down timer triggered, with heating
    if (countDownTriggered && heating){
      if ((millis() - countDownEvent) >= 6000){//after every minute
        countDown = countDown - (millis() - countDownEvent);//subtract time lasped...
        countDownEvent = millis();
        clearLine(45, 1);
        display.setCursor(0, 45);
        display.setTextSize(1);
        display.print("Time left: ");
        display.print(String(countDown / 60000 + "m"));
        display.display();
        timeLeft = countDown / 60000;//in minutes
        ArduinoCloud.update();//push change
      }

      if (countDown <= 0){
        countDownTriggered = false;//turns off countdown
        coolDown();// turns off heating
        clearLine(45, 1);
        display.setCursor(0, 45);
        display.setTextSize(1);
        display.print("Heating Turned off due to timeout...");
        display.display();
        timeLeft = 0;
        message = "Heating Turned off due to timeout...";
        ArduinoCloud.update();//push change
        delay(3000);
      }
    }

    
  //calls the function associated to a button:
    button1.call();
    button2.call();
    } //end main loop
    
//function to reset thermistors by memory.
  void getAllThermistors(){
    displayError("New values:");
    EEPROM.get(sensorin.getAddress(), sensorin);
    EEPROM.get(sensorout.getAddress(), sensorout);

    display.println();
    display.println("inside");
    display.print("b: ");
    display.println(sensorin.printb());
    display.print("res: ");
    display.println(sensorin.getResistance());
    display.println("outside");
    display.print("b: ");
    display.println(sensorout.printb());
    display.print("res: ");
    display.println(sensorout.getResistance());
    display.display();
    delay(5000);
  }

//Button functions:
  bool menuExit;
  int selection;
  int choice_count_up;
  int choice_count_down;

//button 1 functions:
  void pla(){
    rewriteLine1(0, "Set to 50C");  
    tempatureSet = 50;
    onTempatureSetChange();
    message = "heating to 50C.";
  }
  void tpu(){
    rewriteLine1(0, "Set to 55C");  
    tempatureSet = 55;
    onTempatureSetChange();
    message = "heating to 55C.";
  }
  void petg(){
    rewriteLine1(0, "Set to 60C");  
    tempatureSet = 60;
    onTempatureSetChange();
    message = "heating to 60C.";
  }
  void downButton(){
    selection --;
    if (selection < 0){
      selection = 0;
    }
  }

  //heat menu, sets temperature.
    void heatMenu(){
      displayError("Set heat.");
      rewriteLine1(56, message);
      rewriteLine1(8, "Temperature:");
      menuExit = false;
      selection = 50;
      choice_count_up = 75;//75 celcius limit.
      choice_count_down = 0;//0 is off...
      button2.attach(upButton);
      button1.attach(downButton);
      button1.attach(goBack, 4);
      button2.attach(goBack, 4);
      button1.attach(downButton);
      button2.attach(selecter, 1);//sets select to long press.
      button1.attach(selecter, 1);//sets select to long press.
      //menu loop:
      while (!menuExit){
        button1.call();
        button2.call();

        clearLine(18, 2);
        display.setTextSize(2);
        display.setCursor(40,20);
        display.print(selection);
        display.print("C");
        display.display();
      }
      //finish with selection as temperature.
      tempatureSet = selection;
      onTempatureSetChange();
      message = "heating to " + String(selection) + ".";
      button1.detach(0);//detaches button down.
      button1.detach(1);//detaches selecter.
      button2.detach(0);//detaches button up.
      button2.detach(1);//detaches selecter.
      button2.detach(4);//detaches goBack.
      button1.detach(4);//detaches goBack.
      if (selection == 0){
        return;
      }
      displayError("Heat Set to: ");
      display.print(selection);
      display.display();
      delay(3000);
    }

  //time menu. takes a full screen, is a loop. sets the heat time limit.
    void timeMenu(){
      displayError("Set timout.");
      rewriteLine1(56, message);
      rewriteLine1(8, "time:");
      menuExit = false;
      selection = 0;
      choice_count_up = 72;//72 hour limit.
      choice_count_down = 0;
      button2.attach(upButton);
      button1.attach(downButton);
      button2.attach(selecter, 1);//sets select to long press.
      button1.attach(selecter, 1);//sets select to long press.
      button2.attach(goBack, 4);//sets select to long press.
      button1.attach(goBack, 4);//sets select to long press.
      //menu loop:
      while (!menuExit){
        button1.call();
        button2.call();

        clearLine(18, 2);
        display.setTextSize(2);
        display.setCursor(40,20);
        display.print(selection);
        display.print("H");
        display.display();
      }
      //finish with selection as time.
      setTime = selection;
      onSetTimeChange();
      button1.detach(0);//detaches button down.
      button1.detach(1);//detaches selecter.
      button2.detach(0);//detaches button up.
      button2.detach(1);//detaches selecter.
      button1.detach(4);//detaches goBack.
      button2.detach(4);//detaches goBack.
      if (selection == 0){
        return;
      }
      displayError("Time Set to: ");
      display.print(selection);
      display.display();
      delay(3000);
    }

//button 2 functions:
  //sub menu... displayed at base of screen at all times.
    void switchMaterial(){
      int listlength = 4;

      material++;
      if (material > listlength){
        material = 0;//reset list
      }
      if (material == 0){
        button1.attach(pla);
        message = "set to PLA";
        rewriteLine1(56, message);    
      }
      else if (material == 1){
        button1.attach(tpu);
        message = "set to TPU";
        rewriteLine1(56, message);    
      }
      else if (material == 2){
        button1.attach(petg);
        message = "set to PETG";
        rewriteLine1(56, message);    
      }
      else if (material == 3){
        button1.attach(timeMenu);
        message = "Set timout.";
        rewriteLine1(56, message);    
      }
      else if (material == 4){
        button1.attach(heatMenu);
        message = "Set custom temp.";
        rewriteLine1(56, message);    
      }
    }
  void upButton(){
    selection ++;
    if (selection > choice_count_up){
      selection = choice_count_up;
    }
  }

//dual button commands:
  void selecter(){
    menuExit = true;
  }
  void goBack(){
    menuExit = true;
    selection = 0;
  }
  //end button commands

  int fanStatus (Fan fan){
    if (fan.fanPWMStatus()){
      return map(fan.fan_speed, 0, 255, 0, 100);
    }
    else{
      return fan.fanStatus();
    }
  }

//Status sender
  void sendStatus(){
    message = "Outside Temp:\t" + String(heatTempOut, 1) + "\nInside Temp:\t" + String(heatTempIn, 1) +
              "\nFan value:\t" + String(fanStatus(fan1)) + 
              "\nHeating:\t" + String(heating);
    ArduinoCloud.update();
  }

//When Command gets sent
  void onMessageChange(){
    if (message == "PLA"){
      message = "Heating to 55C...";
      tempatureSet = 55;
      onTempatureSetChange();
    }
    else if (message == "PETG"){
      message = "Heating to 60C...";
      tempatureSet = 60;
      onTempatureSetChange();
    }
    else if (message == "TPU"){
      message = "Heating to 55C...";
      tempatureSet = 55;
      onTempatureSetChange();
    }
    else if (message == "OFF" || (message == "0" && !calibration && !disable_chat) || message == "Off"){
      message = "Heating Off...";
      tempatureSet = 0;
      onTempatureSetChange();
    }
    else if (message == "Calibrate"){
      calibration = true;
      message = "ok...";
      ArduinoCloud.update();
      message = "Calibrating.";
    }
    else if (message == "Clear"){
      if (confirm()){
        clearEEPROM();      
      }
      else {
        message = "Canceled.";
        ArduinoCloud.update();
      }
    }
    else if (message == "Reset"){
      if (confirm()){
        ESP.restart();      
      }
      else{
        message = "Canceled.";
        ArduinoCloud.update();
      }
    }
    else if (message.toInt() > 0 && !calibration && !disable_chat){//any integer
      tempatureSet = message.toInt();
      message = "Heating to " + message + "...";    
      onTempatureSetChange();
    } 
    else if (message == "Fan"){
      if (!fan1.fanStatus() || !fan2.fanStatus()){
        fan1.fanOn();
        fan2.fanOn();

        message = "Fan On.\nFan Override turned on.";
        fan_override = true;
        ArduinoCloud.update();
      }
      else{
        fan1.fanOff();
        fan2.fanOff();

        message = "Fan Off.\nFan Override turned on.";
        fan_override = true;
        ArduinoCloud.update();
      }
    }
    else if (message == "DisableFanOverride" || message == "disablefanoverride" || message == "Disablefanoverride"){
        fan_override = false;
        message = "Fan Override turned off.";
        ArduinoCloud.update();
    }
    else if (message == "Status"){
        sendStatus();
    }
    else if(message == "Test"){
      if (!test_mode){
        test_mode = true;
        message = "test mode on.";
        ArduinoCloud.update();
      }
      else{
        test_mode = false;
        message = "test mode off.";
        ArduinoCloud.update();
      }
    }
    else if (message == "Time"){
      message = "How many hours?";
      message_event = millis();
      wait_answer_time = true;
      ArduinoCloud.update();
    }
    else if (message == "Limit"){
      message = "New Plate Max temp?";
      message_event = millis();
      wait_answer_limit = true;
      ArduinoCloud.update();
    }
    else if (message == "Timeleft"){
      message = String(countDown / 60000) + " minutes.";
      ArduinoCloud.update();
    }
    else if (message == "Fanspeed"){
      message = "New Fan speed?";
      message_event = millis();
      wait_answer_speed = true;
      ArduinoCloud.update();
    }
    else if (message == "!help" || message == "Help" || message == "help"){
      message = "command list:\nReset\nCalibrate\nClear: \tclears eeprom\nOff\nTPU\nPETG\nPLA\nFan\nDisableFanOverride\nStatus\nTest\nTime\nLimit\nTimeleft\nFanspeed";
      ArduinoCloud.update();
    }
    else if (!calibration && !disable_chat){//invalid command
      message = "invalid command.\nType \"help\" for command list.";
      ArduinoCloud.update();
    }
    else{//turn off diable_chat
      disable_chat = false;
    }
    rewriteLine1(56, message);
    ArduinoCloud.update();
  }