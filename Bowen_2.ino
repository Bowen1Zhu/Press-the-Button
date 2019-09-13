/* Game Overview
 * Before the game: All LEDs will keep blinking until all four buttons are pressed at the same time to start the game.
 * Rules: During each round of the game, 1-3 colors of the LEDs will be lighted.
 *        Press all the buttons whose colors are not lighted to advance to the next round.
 *        If the player accidentally hits a lighted color or fails to press all the buttons required within the time limit,
 *        he will be informed of his mistakes and then return to the pregame state.
 * Settings: The potentiometer controls the tempo of the game as well as the blinking rate of the LEDs.
 *           Rotate the knob clockwise to accelerate; rotate to the leftmost position to "power off".
 *           Press all four buttons together to pause or resume at any time of the game.
 */

const int LEDPins[4] = {3, 5, 6, 9};    //pin numbers of yellow, blue, green, red, respectively
const int buttonPins[4] = {2, 4, 7, 8}; //pin numbers of yellow, blue, green, red, respectively
const int potentiometerPin = A0;        //pin number of the potentiometer
float interval;                         //tempo controller, which depends on the potentiometer reading

void setup() {
  Serial.begin(9600);
  randomSeed(analogRead(A1));                                           //ensure randomness
  for (int i = 0; i < 4; i ++) {
    pinMode(LEDPins[i], OUTPUT);
    pinMode(buttonPins[i], INPUT_PULLUP);
  }
}

void loop() {
  blinkAllLEDs();
  delay(1000);                                                          //wait for 1 second before the game

  bool endGame = false;
  bool pauseGame = false;
  while (!endGame) {
    //***Pregame Preparation***
    //update the tempo
    interval = map(analogRead(potentiometerPin), 1023, 0, 1200, 300);
    //check if "power off" during the game
    if (interval == 1200) {
      break;
    }

    //randomize the LEDs to be lighted
    bool ArduinoValue[4] = {random(2), random(2), random(2), random(2)};//initially, ArduinoValue holds the colors to be lighted (1--on, 0--off)
    //discard the cases when the LEDs are all on or all off
    while ((ArduinoValue[0] == true && ArduinoValue[1] == true && ArduinoValue[2] == true && ArduinoValue[3] == true) ||
    (ArduinoValue[0] == false && ArduinoValue[1] == false && ArduinoValue[2] == false && ArduinoValue[3] == false)) {
      ArduinoValue[0] = random(2);
      ArduinoValue[1] = random(2);
      ArduinoValue[2] = random(2);
    }
    /* probability distribution:
     * 1 LED  on -- 28.57%
     * 2 LEDs on -- 42.86%
     * 3 LEDs on -- 28.57%
     */


    //***Game Actually Starts***
    //display the LEDs
    for (int i = 0; i < 4; i ++) {
      digitalWrite(LEDPins[i], ArduinoValue[i]);
    }

    //check against the player's input
    endGame = !checkPlayerInput(ArduinoValue, &pauseGame);


    //***Feedback After Each Round***
    if (pauseGame) {
      break;
    }
    if (endGame) {
      turnOffAllLEDs();
      /*blink for 1 second the LEDs that the player failed to press/unpress correctly:
        either the lighted colors that the player hit accidentally, or the colors the player did not hit within the time limit*/
      long oneSecondTimer = millis();
      for (long i = millis(); i < oneSecondTimer + 1000; i = millis()) {
        for (int j = 0; j < 4; j ++) {
          if (ArduinoValue[j] == 0) {                                   //ArduinoValue now stores the player's result--1 if correct, 0 if wrong
            digitalWrite(LEDPins[j], i % 200 < 100);
          }
        }
        if (checkAllButtonsPressed()) {                                 //player may pause at any time
          endGame = true;
          break;
        }
      }
    }
    turnOffAllLEDs();
    
    //wait for more time in case player presses a button for too long
    long delayTimer = millis();
    for (long i = millis(); i < delayTimer + interval; i = millis()) {  //delay(interval);
      if (checkAllButtonsPressed()) {                                   //meanwhile keep checking--player may pause at any time
        endGame = true;
        break;
      }
    }
  }
  delay(500);                                                           //extra time interval before reset
}
//end of loop function


void blinkAllLEDs() {
  while (true) {
    //update the tempo--blinking frequency also depends on it
    interval = map(analogRead(potentiometerPin), 1023, 0, 1200, 300);
    //check if "power off" during blinking
    while (interval == 1200) {
      interval = map(analogRead(potentiometerPin), 1023, 0, 1200, 300);
    }
    
    //brighten
    for (int brightness = 0; brightness <= 255; brightness ++) {
      for (int i = 0; i < 4; i ++) {
        analogWrite(LEDPins[i], brightness);
      }
      //check if player starts the game
      if (checkAllButtonsPressed()) {
        turnOffAllLEDs();
        return;
      }
      delay(map(interval, 1200, 300, 4, 2));
    }

    //fade
    for (int brightness = 255; brightness >= 0; brightness --) {
      for (int i = 0; i < 4; i ++) {
        analogWrite(LEDPins[i], brightness);
      }
      //check if player starts the game
      if (checkAllButtonsPressed()) {
        turnOffAllLEDs();
        return;
      }
      delay(map(interval, 1200, 300, 3, 1));
    }
  }
}

bool checkAllButtonsPressed() {
  for (int i = 0; i < 4; i ++) {
    if (digitalRead(buttonPins[i])) {
      return false;
    }
  }
  return true;
}

void turnOffAllLEDs() {
  for (int i = 0; i < 4; i ++) {
    digitalWrite(LEDPins[i], LOW);
  }
}

bool checkPlayerInput(bool ArduinoValue[], bool* pauseGamePointer) {
  long startTime = millis();                                            //start the timer
  bool PlayerPassTest[4];                                               //true means player has passed the corresponding color test, false otherwise
  for (int i = 0; i < 4; i ++) {
    PlayerPassTest[i] = ArduinoValue[i];                                //for colors that are lighted up, they're already passed
  }
  while (millis() - startTime <= 2 * interval) {                        //total time limit = 2 x interval
    if (millis() - startTime == interval) {                             //LEDs will be lighted for the first half of the time
      turnOffAllLEDs();
    }
    for (int i = 0; i < 4; i ++) {
      if (ArduinoValue[i]) {
        if (!digitalRead(buttonPins[i])) {                              //if player has accidentally pressed the lighted color
          delay(100);                                                   //wait after the time delay between different buttons being pressed
          for (int j = 0; j < 4; j ++) {
            if (ArduinoValue[j] && !digitalRead(buttonPins[j])) {
              ArduinoValue[j] = 0;                                      //use the original array to store player's mistakes
            } else {
              ArduinoValue[j] = 1;
            }
          }
          return false;                                                 //game ends
        }
      } else {
        if (!digitalRead(buttonPins[i])) {
          PlayerPassTest[i] = true;                                     //this color test is passed
        }
      }
    }
    if (PlayerPassTest[0] == true && PlayerPassTest[1] == true && PlayerPassTest[2] == true && PlayerPassTest[3] == true) {
      return true;                                                      //all tests passed--next round
    }
    if (checkAllButtonsPressed()) {                                     //player may pause at any time
      *pauseGamePointer = true;
      return 1;                                                         //return value doesn't matter
    }
  }
  //timeout
  for (int i = 0; i < 4; i ++) {
    ArduinoValue[i] = PlayerPassTest[i];                                //use the original array to store the colors that the player has not pressed before timeout
  }
  return false;                                                         //game ends
}
