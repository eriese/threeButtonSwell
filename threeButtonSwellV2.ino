//This code uses a button state change code to dictate that different functions for each push of the mode button. Millis allows everything to run in dynamic real-time.

// pressing the mode button cycles through the speeds and patterns
// holding the mode button for 3 seconds toggles the swelling
// the up and down buttons control the maximum size of the toy.
//// during swelling, this means the greatest angle that the servo can turn to before switching directions.
//// without swelling this means the angle that the servo maintaining

#include <Servo.h>
Servo swellServo;

// pins
const int morePin = 7; // the pin for the button to make it bigger
const int lessPin = 8; // the pin for the button to make it smaller
const int modePin = 4; // the pin for the button to change the mode

const int servoPin = 3; // the pin for the servo
const int vibeOutPin = 11; // the pin that controls the vibrator

//limits
const int speedMax = 5;
const int sizeMax = 7;
const int modeMax = 5;
const int acute = 25;
const int obtuse = 155;

// variable settings parameters
bool stayPut;
int biggest;
int sizeCount;
int modeCount;

// last button state monitoring
int lastMoreState;
int lastLessState;
int lastModeState;
bool longPress;

// time stuff
long previousMillis;
const long interval = 200;
long pressMillis;
const long longPressLength = 1750;

// for servo positioning
int countupdown;

void setup() {
  Serial.begin(9600);

  // attach pins
  swellServo.attach(servoPin);
  pinMode(morePin, INPUT);
  pinMode(lessPin, INPUT);
  pinMode(modePin, INPUT);
  pinMode(vibeOutPin, OUTPUT);

  // initialize settings
  modeCount = 0;
  sizeCount = 3;
  biggest = map(sizeCount, 0, sizeMax, acute, obtuse);
  stayPut = true;

  // initialize button states
  lastMoreState = LOW;
  lastLessState = LOW;
  lastModeState = LOW;
  longPress = false;

  // start by counting up
  countupdown = 1;
}

void loop() {
  // check the button states
  checkMode();
  bool sizeChange = checkMoreLess();

  // mode dictates the vibration pattern (so far just speeds)
  int speedCount;
  switch (modeCount) {
    case 0:
      speedCount = 0;
      break;
    case 1:
      speedCount = 1;
      break;
    case 2:
      speedCount = 2;
      break;
    case 3:
      speedCount = 3;
      break;
    case 4:
      speedCount = 4;
      break;
  }

  // with all new info registered, set the vibration and swelling
  vibe(speedCount);
  swell(speedCount, sizeChange);
}


bool checkMoreLess() {
  // if the toy is on, check to see if the more or less buttons have been pressed
  bool morePushed = false;
  bool lessPushed = false;
  if (modeCount > 0) {

    // if the more button has been pushed, increment size upward if it's not already at the top
    int moreState = digitalRead(morePin);
    morePushed = moreState != lastMoreState;
    lastMoreState = moreState;

    if (morePushed && moreState == HIGH && sizeCount < sizeMax) {
      sizeCount++;
    }

    // if the less button has been pushed, decrement size downward if it's not already at the bottom
    int lessState = digitalRead(lessPin);
    lessPushed = lessState != lastLessState;
    lastLessState = lessState;

    if (lessPushed && lessState == HIGH && sizeCount > 0) {
      sizeCount--;
    }

  }

  // print if there's been a push
  bool hasPush = morePushed || lessPushed;
  if(hasPush) {
    Serial.print("Size has changed to ");
    Serial.println(sizeCount);
  }

  // return whether or not either button has been pushed
  return hasPush;
}

void checkMode() {
  // read the mode button state
  int modeState = digitalRead(modePin);
  bool modeStateChange = modeState != lastModeState;

  // get how long it's been since a long press started
  long timeSincePress = millis() - pressMillis;
  bool longEnough = timeSincePress >= longPressLength;

  // if the button state has changed
  if (modeStateChange) {
    // reset the press time counter regardless of up or down. this will mean that a press and release will always show as within the interval
    pressMillis = millis();

    // if it changed to low and it's within the long press interval (i.e. it's been released), increment the mode
    if(modeState == LOW && !longEnough && !longPress) {
      modeCount++;

      // cycle back to 0 if necessary
      if (modeCount > modeMax) {
        modeCount = 0;
      }

      // print it
      Serial.println("mode button pushed");
      Serial.print("mode is ");
      Serial.println(modeCount);
    }
    longPress = false;
  }
  // if the button state is the same and it's high (i.e. a sustained press), and it's gone beyond the interval
  else if (modeState == HIGH && longEnough) {
    // toggle the stay put function
    stayPut = !stayPut;
    int pos = swellServo.read();
    sizeCount = map(pos, acute, obtuse, 0, sizeMax);
    Serial.println("long press");
    Serial.print("size: ");
    Serial.println(sizeCount);
    // reset the interval to a few seconds out so even if they keep it pressed for a bit this doesn't keep toggling
    pressMillis = millis() + 15000;
    longPress = true;
  }

  // reset the last mode state to the current mode state for the next loop
  lastModeState = modeState;
}

void swell(int speedCount, bool sizeChange) {

  // if the size has changed
  if(sizeChange) {
    // recalculate the biggest size
    biggest = map(sizeCount, 0, sizeMax, acute, obtuse);
    Serial.print("size: ");
    Serial.println(biggest);
    Serial.print("stay put: ");
    Serial.println(stayPut);

    // if it's in stay put mode, write the new size
    if(stayPut){
      setSize();
    }
  }

  // whether or not the size has changed, if it's not in stay put, swell at the correct pace
  if (!stayPut) {
    unsigned long currentMillis = millis();

    // calculate the interval of movement based on the vibe strength
    int d = map(speedCount, 0, speedMax, 0, 20);
    // if the correct interval has passed since the last time the servo moved
    if ((currentMillis - previousMillis) > (interval / d)) {
      int pos = swellServo.read();
      Serial.print("moving ");
      Serial.println(pos);
       pos += countupdown;
      // write the new position to the servo
      swellServo.write(pos);
      // increment the position for next time


      // if the servo is at biggest allowable size, switch to incrementing downward
      if (pos >= biggest) {
        Serial.println("direction switching down");
        countupdown = -1;
      }

      // if the servo is at 0 degrees, switch to incrementing upward
      if (pos <= 30) {
        Serial.println("direction switching up");
        countupdown = 1;
      }

      // reset the interval
      previousMillis = currentMillis;
    }
  }
}

void setSize() {
  int servPos = swellServo.read();
  Serial.print("size set: ");
  Serial.println(servPos);
  swellServo.write(biggest);
}

void vibe(int speedCount) {
  int vibeOutValue = map(speedCount, 0, speedMax, 0, 255);
  analogWrite(vibeOutPin, vibeOutValue);
}

