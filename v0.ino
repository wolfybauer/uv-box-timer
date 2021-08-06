#include <Adafruit_GFX.h>
#include <Adafruit_SH1106.h>

#include <Bounce2.h>

// OLED pins
#define OLED_CLK   9 // labelled D0 on hardware
#define OLED_MOSI  10 // labelled D1 on hardware
#define OLED_DC    11
#define OLED_CS    12
#define OLED_RESET 13

// button pins
#define START_PIN   8
#define STOP_PIN    5
#define UP_PIN      7
#define DOWN_PIN    6

// RELAY CONTROL pin
#define LED_PIN     3

// buzzer pin + logic
#define BUZZER_PIN  4
bool buzzerState = false;

// build display instance
Adafruit_SH1106 screen(OLED_MOSI, OLED_CLK, OLED_DC, OLED_RESET, OLED_CS);

// build buttons
Bounce START_BUTTON  = Bounce();
Bounce STOP_BUTTON   = Bounce();
Bounce TIMER_UP      = Bounce();
Bounce TIMER_DOWN    = Bounce();

// timer vars
unsigned long TIMER_START;
unsigned long TIMER_SET = 300000;      // ( default running time. 30000 = 30 seconds )
unsigned long x = TIMER_SET;          // variable x is the counter
unsigned long SET_INCREMENT = 30000;   // amount counter adjusts +/- while in reset mode. 10000 = 10sec

// time conversion variables
uint16_t secs = x/1000;          // total running time (in seconds) (not shown on screen)
uint8_t  mins = secs/60;                  // actually shown on screen
uint8_t  secs_adj = secs - (mins * 60);   // actually shown on screen



// timer logic flags
bool isRunning  = false;
bool isStopped  = false;
bool ranOut     = false;

void setup() {
  //Serial init (may be redundant, would have to look in the gfx+sh1106 libraries)
  Serial.begin(9600);

  // buttons init
  START_BUTTON.attach(START_PIN, INPUT_PULLUP);
  START_BUTTON.interval(5);
  STOP_BUTTON.attach(STOP_PIN, INPUT_PULLUP);
  STOP_BUTTON.interval(5);
  TIMER_UP.attach(UP_PIN, INPUT_PULLUP);
  TIMER_UP.interval(5);
  TIMER_DOWN.attach(DOWN_PIN, INPUT_PULLUP);
  TIMER_DOWN.interval(5);

  // RELAY / LED init
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  // buzzer init
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);
  
  //display init
  screen.begin(SH1106_SWITCHCAPVCC);
  screen.display();
  
  screen.clearDisplay();

}

void loop() {

  screenStuff();
  buttonStuff();
  
  if(isRunning == true) { // checks flag, starts timer if true
  
    runTimer();
  }
  
  if(x == 0) {            // (some) runout logic lives in here
    
    isRunning = false;
    isStopped = false;
    ranOut    = true;
    buzzerState = true;
  } // counter reset

  
  ledCheck();             // checks isRunning flag, turns on RELAY/LED if true

}

void buttonStuff() {

  // poll all buttons for falling edge
  START_BUTTON.update();
  STOP_BUTTON.update();
  TIMER_UP.update();
  TIMER_DOWN.update();

  if(buzzerState == true) {

    buzzerStuff();
    buzzerState = false;
    x = TIMER_SET;
    return;
  }

  // checks all 3 flags. all false = in reset mode,
  // enables counter up/down controls. else disabled.
  if(isRunning == false && isStopped == false && ranOut == false) {
    if(TIMER_UP.fell()) {
      TIMER_SET += SET_INCREMENT;
      x = TIMER_SET;
    }
    if(TIMER_DOWN.fell()) {
      if(TIMER_SET - SET_INCREMENT > 0) {
        TIMER_SET -= SET_INCREMENT;
        x = TIMER_SET;
      }
    }
  }

  

  // start button logic
  if(START_BUTTON.fell()) {

    TIMER_START = millis(); // i tried putting this inside runTimer(), didn't work. why?

    // if there is >0 time remaining on the clock, set flags accordingly
    if(x > 0) {
      Serial.println("START_BUTTON !"); // for testing
      
      isRunning = true;
      isStopped = false;
      ranOut = false;

      
    }
  }

  // stop/reset button logic
  if(STOP_BUTTON.fell()) {

    // if running, stop running
    if(isRunning == true && isStopped == false) {
      
      Serial.println("STOP_BUTTON !");
      
      isRunning = false;
      isStopped = true;
      ranOut    = false;
      
      return;
    }
    // if stopped, reset counter
    else if(isRunning == false && isStopped == true) {
      
      Serial.println("RESET_BUTTON !");
      
      x = TIMER_SET;
      
      isRunning = false;
      isStopped = false;
      ranOut    = false;
      
      return;
    }
    // if stopped (due to time running out), activate buzzer, reset counter
    // (then waits at 00:00 until stop is pressed again, goes back to reset mode
    if(ranOut == true) {
      
      //buzzerStuff();
      
      x = TIMER_SET;

      // all flags false, aka reset mode
      isRunning = false;
      isStopped = false;
      ranOut    = false;

      return;
    }
    
  }
}

// logic that actually does whatever you need doing while the timer is on
// in this case, it's activating a relay (or led)
void ledCheck() {
  if(isRunning == true) {
    digitalWrite(LED_PIN, HIGH);
  } else {
    digitalWrite(LED_PIN, LOW);
  }
}

// ACTUAL TIMER LOGIC. Only code in here that i ripped from some forum on the internet,
// but still had to play with it a lil bit.
void runTimer() {
  
  //TIMER_START = millis();   // hmm. this has to live in the start button logic, for some reason

  // ahhhh. i see what it is: more than 1000millis (1 second) had to have passed before the
  // if statement gets triggered. dumb luck? i guess i could put another if statement around
  // this if statement, but with a smaller increment. >=1 or 10 or something. and include that
  // TIMER_START = millis() from earlier. idk, what do you think?
  
  if(millis() - TIMER_START >= 1000) { // if 1 second has passed
    x -= 1000;                         // decrement x by 1 second (count down)
    secs = x / 1000;                   // convert x from millis to secs
    TIMER_START += 1000;               // increment TIMER_START 1 second, to catch up
  }
}

void screenStuff() {
  screen.clearDisplay(); // clear display

  // update all neccesary vars, to keep screen up to date
  secs = x/1000;
  mins = secs/60;
  secs_adj = secs - (mins * 60);

  // text init (apparently this has to be done every loop for the shit to work)
  screen.setTextSize(3);
  screen.setTextColor(WHITE);
  screen.setCursor(20, 20);

  // print text, with leading zeros
  if(mins < 10) {
    screen.print(0);
  }
  screen.print(mins);
  screen.print(":");
  if(secs_adj < 10) {
    screen.print(0);
  }
  screen.println(secs_adj);

  screen.display(); // update display
}

// this was an attempt to use a pwm technique to generate a beep directly from the arduino
// at something like 500hz, if i did my math correctly. (hz = cycles per second, and its working in
// milliseconds. 1 cycles = 1 millisecond HIGH, 1 millisecond LOW. aka 1000/2 = 500hz. just above
// A440.) it didn't work. arduino does not have enough to juice to pull this off directly to a 0.5 watt
// speaker. but anyway, this is activated whenever the thing completes a full countdown. anything could go here.
// NOTE: ialso borrowed this technique, its called BlinkWithoutDelay and is documented on arduino forums
void buzzerStuff() {
  
  digitalWrite(BUZZER_PIN, HIGH);
  delay(2000);
  digitalWrite(BUZZER_PIN, LOW);
  
}
