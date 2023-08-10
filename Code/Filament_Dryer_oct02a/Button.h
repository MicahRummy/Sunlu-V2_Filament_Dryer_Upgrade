// #ifndef Button.h
// #define Button.h
// #endif

class Button{
  private:
    /*
    *not sure if this will be implemented*
      modes:
        0 = ON LOW
        1 = RISING
        2 = FALLING = default
        3 = CHANGE
        4 = ON HIGH
    */

    int mode;//not used yet

    int pin;
    bool is_attached;

  //button timeouts
    int button_timeout = 0;
    int button_timeout_new = 0;
    int button_timeout_old = 0;
    int button_timeout_timer_min = 100;
    int button_timeout_timer_max = 500;

  //all functions false by default.
    bool func0 = false;
    bool func1 = false;
    bool func2 = false;
    bool func3 = false;
    bool func4 = false;

    typedef void (*attachedFunc)(void);

    attachedFunc longPress;//1 second press
    attachedFunc long5Press;//5 second press
    attachedFunc long10Press;//10 second press
    attachedFunc doublePress;//double press

    attachedFunc userFunc;//user defined function
    attachedFunc nothing;//nothing function...? fatal error if called. code prevents it from being called.
    
  public:
  //Constructor declare pin as input:
    Button(int pin){
      this->pin = pin;
      pinMode(pin, INPUT);
      func0 = false;
      func1 = false;
      func2 = false;
      func3 = false;
      func4 = false;
      is_attached = false;
    }
    
  //constructor with a default function attached:
    Button(int pin, void (*func)()){
      this->pin = pin;
      pinMode(pin, INPUT);
      is_attached = true;
      func0 = true;
      func1 = false;
      func2 = false;
      func3 = false;
      func4 = false;
    //sets the user function.
      userFunc = func;
    }
  //constructor with a function attached by mode:
    Button(int pin, void (*func)(), int mode){
      this->pin = pin;
      pinMode(pin, INPUT);

      is_attached = true;
      if (mode == 0){
      if (userFunc == func){return;}
      userFunc = func;
      func0 = true;
      }

      else if (mode == 1){
        if (longPress == func){return;}
        longPress = func;
        func1 = true;
      }

      else if (mode == 2){
        if (long5Press == func){return;}
        long5Press = func;
        func2 = true;
      }

      else if (mode == 3){
        if (long10Press == func){return;}
        long10Press = func;
        func3 = true;
      }

      else if (mode == 4){
        if (doublePress == func){return;}
        doublePress = func;
        func4 = true;
      }
    }

  //constructor with a default function and long press:
    Button(int pin, void (*func)(),void (*longPress)()){
      is_attached = true;
      this->pin = pin;
      pinMode(pin, INPUT);
    //sets the user functions.
      userFunc = func;
      this->longPress = longPress;
      func0 = true;
      func1 = true;
      func2 = false;
      func3 = false;
      func4 = false;
    }

  //constructor with a default function, long press and 5 second long press:
    Button(int pin, void (*func)(),void (*longPress)(),void (*long5Press)()){
      is_attached = true;
      this->pin = pin;
      pinMode(pin, INPUT);
    //sets the user functions.
      userFunc = func;
      this->longPress = longPress;
      this->long5Press = long5Press;
      func0 = true;
      func1 = true;
      func2 = true;
      func3 = false;
      func4 = false;
    }

  //constructor with a default function, long press, 5 second long press, and 10 second long press:
    Button(int pin, void (*func)(),void (*longPress)(),void (*long5Press)(), void (*long10Press)()){
      is_attached = true;
      this->pin = pin;
      pinMode(pin, INPUT);
    //sets the user functions.
      userFunc = func;
      this->longPress = longPress;
      this->long5Press = long5Press;
      this->long10Press = long10Press;
      func0 = true;
      func1 = true;
      func2 = true;
      func3 = true;
      func4 = false;
    }
    
  //constructor with a default function, long press, 5 second long press, 10 second long press, and double press:
    Button(int pin, void (*func)(),void (*longPress)(),void (*long5Press)(), void (*long10Press)(), void (*doublePress)()){
      is_attached = true;
      this->pin = pin;
      pinMode(pin, INPUT);
    //sets the user functions.
      userFunc = func;
      this->longPress = longPress;
      this->long5Press = long5Press;
      this->long10Press = long10Press;
      this->doublePress = doublePress;
      func0 = true;
      func1 = true;
      func2 = true;
      func3 = true;
      func4 = true;
    }

  //function calls:
    bool is_pressed();//gets the button state.

    void attach(void (*func)());//associates a function.
    void attach(void (*func)(), int mode);//associates a function and a mode.

    void detach();//removes all functions.
    void detach(int mode);//removes specified function.

    void detachIrrelevance();//removes all except 10s press.

    int hold();//returns seconds held

    void call();//calls the function.
};

/*  *******************
    ***   Methods   ***
    *******************
*/

//when button is pressed
  bool Button::is_pressed(){
    return digitalRead(pin);
  }

//make an attachment to a function (not IRS)
 void Button::attach(void (*func)()){
    is_attached = true;
    if (func == userFunc){
      return;
    }
    userFunc = func;
    func0 = true;
  }

//make an attachment to a specified function (not IRS)
 void Button::attach(void (*func)(), int mode){
  //ensures function is not created twice
    is_attached = true;
    if (mode == 0){
    if (userFunc == func){return;}
    
    userFunc = func;
    func0 = true;
    }
    else if (mode == 1){
      if (longPress == func){return;}
      longPress = func;
      func1 = true;
    }
    else if (mode == 2){
      if (long5Press == func){return;}
      long5Press = func;
      func2 = true;
    }
    else if (mode == 3){
      if (long10Press == func){return;}
      long10Press = func;
      func3 = true;
    }
    else if (mode == 4){
      if (doublePress == func){return;}
      doublePress = func;
      func4 = true;
    }
 }

//detaches all functions:
 void Button::detach(){
    is_attached = false;
    userFunc = nothing;
    func0 = false;

    longPress = nothing;
    func1 = false;

    long5Press = nothing;
    func2 = false;

    long10Press = nothing;
    func3 = false;

    doublePress = nothing;
    func4 = false;
 }

//detach specified function:
 void Button::detach(int mode){
   if (mode == 0){
    userFunc = nothing;
    func0 = false;
   }
   else if (mode == 1){
    longPress = nothing;
    func1 = false;
   }
   else if (mode == 2){
    long5Press = nothing;
    func2 = false;
   }
   else if (mode == 3){
    long10Press = nothing;
    func3 = false;
   }
   else if (mode == 4){
    doublePress = nothing;
    func4 = false;
   }
 }

 void Button::detachIrrelevance(){
    for(int i = 0; i < 2; i++){
      detach(i);
    }  
    detach(4);
 }

//returns how long a button is pressed.
  int Button::hold(){
    int run = millis();
    float ran = 0;
    if (is_pressed()){
      button_timeout_old = button_timeout_new;
      button_timeout_new = millis();
      button_timeout = button_timeout_new - button_timeout_old;
    }

    if (!is_pressed() || !is_attached){
      return -2;
    }

  
    while (is_pressed() && ran < 10){
      ran = (millis() - run) / 1000;//ran is in seconds.
    }
    //if button gets spammed, do nothing.
    if (button_timeout <= button_timeout_timer_min){
      return -2;
    }
    //if button is pressed twice within the max and min limits:
    if (button_timeout >= button_timeout_timer_min && button_timeout < button_timeout_timer_max){
      return -1;
    }
    return ran;
 }

//call a function.
 void Button::call(){
   int held = hold();
    
   if (held == -2){
    return;
   }
   if (held == -1 && func4){
     doublePress();     
   }
   else if (held < 1 && func0){
    userFunc();
   }
   else if (held >= 1 && held < 5 && func1){
    longPress();
   }
   else if (held >= 5 && held < 10 && func2){
    long5Press();
   }
   else if (func3 && held >= 10){
    long10Press();
   }
   else{
    //  Serial.println("no function defined for this.");
   }
 }