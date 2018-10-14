/*
  This code runs vixen through the arduino, usining pins 3, 5, and 6 with
  3 seperate servos; pins 9, 10, and 11 are used for 3 LED circuits with PWM;
  pins 12, 13 are extra digital pins.

  Adapted from code by GSRVAhiker17, zparticle, victor_pv, Si_champion, and Neil Tapp.
*/

#include <Servo.h>    // Servos
#include <avr/wdt.h>  // Raw AVR watchdog timer

// Testing mode (treat servos as simple PWM)
const bool TESTINGMODE = false;

// Vixen header (identifies a light sequence)
const int HEADER_LEN = 2;
const char seqHeader[] = {'~','!'};

// Timeout waiting for serial input before going to random mode (in milliseconds).
const int TIME_OUT = 1000;

// Channels mapped to pin numbers
const int PUMPKIN1_MOUTH = 9;
const int PUMPKIN2_MOUTH = 10;
const int PUMPKIN3_MOUTH = 11; 
const int PUMPKIN1_EYES  = 3; 
const int PUMPKIN2_EYES = 5; 
const int PUMPKIN3_EYES = 6;
const int LIGHTS1 = 12;  
const int LIGHTS2 = 13;   
const int CHAN9 = 2;
const int CHAN10 = 4;
const int CHAN11 = 5;
const int CHAN12 = 7;
const int CHAN13 = A0;
const int CHAN14 = A1;
const int CHAN15 = A2;
const int CHAN16 = A3;
const int CHAN17 = A4;
const int CHAN18 = A5;

// List of active channels
const int channels[] = {PUMPKIN1_MOUTH, PUMPKIN2_MOUTH, PUMPKIN3_MOUTH, 
                        PUMPKIN1_EYES, PUMPKIN2_EYES, PUMPKIN3_EYES, LIGHTS1, LIGHTS2,
                        CHAN9, CHAN10, CHAN11, CHAN12, CHAN13, CHAN14, CHAN15, CHAN16,
                        CHAN17, CHAN18
                       };

// Number of active channels
const int NUM_ACTIVE_CHANNELS = 8;

// PWM map
const int isPWM[] = {true, true, true, true, true, true, false, false, false, false,
                     false, false, false, false, false, false, false, false
                    };

// Servos
const int SERVO_DELAY = 15; // delay after servo is activated (allow it to move)
const int NUM_SERVOS = 3;
const int NEUTRAL = 90;  // Neutral position
Servo servos[NUM_SERVOS];

// Min servo opening in degrees from neutral position
const int servoMin[] = {0, 0, 0};

// Max servo opening in degrees from neutral position
const int servoMax[] = {35, 45, 45};
//const int servoMax[] = {20, 15, 15};

// Servo channel map
const int NO_SERVO = -1;
const int servoNumber[] = {0, 1, 2, NO_SERVO, NO_SERVO, NO_SERVO, NO_SERVO, NO_SERVO,
                        NO_SERVO, NO_SERVO, NO_SERVO, NO_SERVO, NO_SERVO, NO_SERVO, NO_SERVO, NO_SERVO,
                        NO_SERVO, NO_SERVO
                       };
                       
// Servo direction
const int CLOCKWISE = 1;
const int COUNTERCLOCKWISE = -1;
const int servoDirection[] = {COUNTERCLOCKWISE, CLOCKWISE, CLOCKWISE};

// Serial
const long COM_SPEED = 115200;
int incomingByte[NUM_ACTIVE_CHANNELS];  // array to store the channel values from the serial port

// Misc
int i = 0;                              // Loop counter
int j = 0;                              // Loop counter
volatile unsigned long  timer_a = 0;    // Timer


//setup the pins/ inputs & outputs
void setup()
{
  // enable the watchdog timer with a time of 1 second. If the board freezes, it will reset itself after 1 second.
  wdt_enable(WDTO_1S);

  // specifically for the UNO
  sei();

  // initalize PWM Channels / Pins
  for (i = 0; i < NUM_ACTIVE_CHANNELS; i++) {
    pinMode(channels[i], OUTPUT);
    if ((servoNumber[i] != NO_SERVO) && !TESTINGMODE) {
      servos[servoNumber[i]].attach(channels[i]);
    }
  }

  // set all the channels off to begin
  for (i = 0; i < NUM_ACTIVE_CHANNELS; i++) {
    digitalWrite(channels[i], LOW);
    if ((servoNumber[i] != NO_SERVO) && !TESTINGMODE) {
      servos[servoNumber[i]].write(NEUTRAL);
    }
  }

  test_sequence(); // brief test
  Serial.begin(COM_SPEED);   // set up Serial
}

void loop()
{
  if (Serial.available() >= NUM_ACTIVE_CHANNELS + HEADER_LEN) {
    wdt_reset(); // resets the watchdog (prevents board lockup)
    timer_a = millis ();  // Mark the time when a message was received

    // read the header to verify this is in fact a light sequence
    // probably overkill, but borrowing from the above sources...
    for (int i = 0; i < HEADER_LEN; i++) {
      if (seqHeader[i] != Serial.read()) { return; }
      //Serial.read();
    }
    
    // read the oldest byte in the serial buffer:
    for (int i = 0; i < NUM_ACTIVE_CHANNELS; i++) {
      // read each byte
      incomingByte[i] = Serial.read();

      if ((servoNumber[i] != NO_SERVO) && !TESTINGMODE) {
        // SERVOS ------------------------------
        int angle = map(incomingByte[i], 0, 255, servoMin[servoNumber[i]], servoMax[servoNumber[i]]);
        angle *= servoDirection[servoNumber[i]];
        servos[servoNumber[i]].write(NEUTRAL + angle);
        delay(SERVO_DELAY);
      } else if (isPWM[i]) {
        // PWM ---------------------------------
        analogWrite(channels[i], incomingByte[i]);
      } else {
        // DIGITAL (A2D) -----------------------
        if (incomingByte[i] <= 127) {
          digitalWrite(channels[i], LOW);
        } else {
          digitalWrite(channels[i], HIGH);
        }
      }
    }

  } else {
    // Random mode starts if no serial input has been received in TIME_OUT milliseconds
    wdt_reset(); // resets the watchdog (prevents board lockup)
    unsigned long diff = millis() - timer_a;
    if (diff >= TIME_OUT) {
      timer_a = millis ();
      int random_a = 0;
      for (i = 0; i < NUM_ACTIVE_CHANNELS; i++) {
        if ((servoNumber[i] != NO_SERVO) && !TESTINGMODE) continue;
        random_a = random(0, 2);
        if (random_a == 0) {
          digitalWrite(channels[i], LOW);
        } else {
          digitalWrite(channels[i], HIGH);
        }
      }
    }
  }
}

// Test the setup briefly
void test_sequence() {
  for (i = 0; i < NUM_ACTIVE_CHANNELS; i++) {
    wdt_reset(); // resets the watchdog
    digitalWrite(channels[i], HIGH);
    delay (500);
    digitalWrite(channels[i], LOW);
  }
}
