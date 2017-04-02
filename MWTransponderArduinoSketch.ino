/*
  MWScore Transponder 2017
  R-Team Robotics Version
  
  XBEE setup:
  ATBD = 5 (38400bps)
  ATID = 6200
  MY   = 6200 + TRANSPONDER_ID
  DL   = 6201
  CH   = c
  
  Scoring Receiver XBEE setup (Send Broadcast message)
  ATBD = 5 (38400bps)
  ATID = 6200
  MY   = 6201
  DL   = FFFF
  DH   = 0
  CH   = c
*/ 

#include <TimerOne.h>
#include <PinChangeInt.h>

#define XBEE_BAUDRATE 38400

#define PANELMASK  240
#define PANEL1     112
#define PANEL2     176
#define PANEL3     208
#define PANEL4     224

#define MS_COOLDOWN 1000
#define MS_SIGNAL 50

#define LED_RATE 50
#define LED_PERIOD 300                // LED_RATE * 6

// Optional Rule Variables
#define MAX_PANEL_HIT 15              // Max hit points scored on panel
#define HEALING_HP_THRESHOLD 10       // Hit points when healing starts
#define HEALING_RATE 5000             // Healing Rate (ms)

//#define TimerMS 7813                // 128 Hz
//#define TimerMS 15625               //  64 Hz 
//#define TimerMS 31250               //  32 Hz
//#define TimerMS 62500               //  16 Hz
//#define TimerMS 125000              //   8 Hz
#define TimerMS 250000                //   4 Hz
//#define TimerMS 500000              //   2 Hz
//#define TimerMS 1000000             //   1 Hz
//#define TimerMS 2000000             // 0.5 Hz

volatile uint8_t hit = 0;
volatile uint8_t hitpoint = 20;
volatile uint8_t panel = 0;
uint8_t id;

// Optional rules variables
volatile uint8_t tphit[5];
volatile uint32_t lasthittime = 0;
volatile uint32_t curenttime = 0;

// Optional rules variables
// 0 = Default Rule
// 1 = Max Hit Point per Panel
// 2 = Healing Enabled
volatile uint8_t rules = 0;

// Arduino Pin Assignments
int hiti = 3;
int hitu = 2;

int tp1 = 4;
int tp2 = 5;
int tp3 = 6;
int tp4 = 7;

// ID Pin Assignments (Analog Pins)
int sw0 = A0;
int sw1 = A1;
int sw2 = A2;
int sw3 = A3;
int sw4 = A4;
int sw5 = A5;
int sw6 = A6;
int sw7 = A7;

void hittp1() {
  hit = PANEL1;
}

void hittp2() {
  hit = PANEL2;
}

void hittp3() {
  hit = PANEL3;
}

void hittp4() {
  hit = PANEL4;
}

volatile bool toTransmit = false;

void ISRTimer1(){
  // tx hit packet
  toTransmit = true;
}


void setup() {
  pinMode(hiti, OUTPUT);
  pinMode(hitu, OUTPUT);
  
  pinMode(tp1, INPUT_PULLUP);
  pinMode(tp2, INPUT_PULLUP);
  pinMode(tp3, INPUT_PULLUP);
  pinMode(tp4, INPUT_PULLUP);
  
  PCintPort::attachInterrupt(tp1, hittp1, CHANGE);
  PCintPort::attachInterrupt(tp2, hittp2, CHANGE);
  PCintPort::attachInterrupt(tp3, hittp3, CHANGE);
  PCintPort::attachInterrupt(tp4, hittp4, CHANGE);

  pinMode(sw0, INPUT_PULLUP);
  pinMode(sw1, INPUT_PULLUP);
  pinMode(sw2, INPUT_PULLUP);
  pinMode(sw3, INPUT_PULLUP);
  pinMode(sw4, INPUT_PULLUP);
  pinMode(sw5, INPUT_PULLUP);
  pinMode(sw6, INPUT_PULLUP);
  pinMode(sw7, INPUT_PULLUP);
  
  Serial.begin(XBEE_BAUDRATE);

  // set ID intially
  id = 63 - (0x3f & PINC);

  // set intial ruleset variables
  rules = 0;
  tphit[0] = 0;
  tphit[1] = 0;
  tphit[2] = 0;
  tphit[3] = 0;
  tphit[4] = 0;

  // blink LED board 3 times
  for (int x = 0; x < 3; x++)
  {
    digitalWrite(hitu, HIGH);
    delay(LED_RATE);
    digitalWrite(hitu, LOW);
    delay(LED_RATE);
  }

  // set up interrupt to send HP status message
  Timer1.initialize(TimerMS);
  Timer1.attachInterrupt(ISRTimer1);
}

void maybetransmit() {
  if (toTransmit) {
    toTransmit = false;
    Serial.write((uint8_t) 0x55);
    Serial.write((uint8_t) id);
    Serial.write((uint8_t) (0xff - id));
    Serial.write((uint8_t) panel);
    Serial.write((uint8_t) hitpoint);
  }
}

void loop() {
  uint32_t delayms = 0;
  byte receive[5];
  uint8_t oldhitpoint = 0;

  // update ID when changed
  id = 63 - (0x3f & PINC);

  // Check for HP change
  // Scoring Receiver sends broadcast message to set HP
  // If no message default is 20 HP
  // Receive message 0x55 ID 255-ID HP RULES
  while (Serial.available() > 0) {
    uint8_t key = Serial.read();
    if (key == 0x55) {
      receive[0] = 0x55;
      int p = 1;
      long when = millis();
      while ((p < 5) && (millis() - when) < 1000) {
        if (Serial.available()) {
          receive[p++] = Serial.read();
        }
      }
      if((p == 5) && (receive[1] == id) && ((receive[1] + receive[2]) == 255)) {
        hitpoint = (int)receive[3];
        rules = (int)receive[4];
        tphit[0] = 0;
        tphit[1] = 0;
        tphit[2] = 0;
        tphit[3] = 0;
        tphit[4] = 0;
        for (int i = 0; i != 4; ++i) {
          digitalWrite(hitu, HIGH);
          delay(20);
          digitalWrite(hitu, LOW);
          delay(70);
        }
      } else {
        // something went wrong -- let someone know
        digitalWrite(hitu, HIGH);
        delay(100);
        digitalWrite(hitu, LOW);
      }
    }
  }
  
  if ((hit != 0) && (hitpoint > 0)) {   
    // determine panel that was hit
    if ((hit & PANELMASK) == PANEL1) {
      delayms = MS_SIGNAL * 1;
      panel = 1;
      }
    else if ((hit & PANELMASK) == PANEL2) {
      delayms = MS_SIGNAL * 2;
      panel = 2;
      }
    else if ((hit & PANELMASK) == PANEL3) {
      delayms = MS_SIGNAL * 3;
      panel = 3;
      }
    else if ((hit & PANELMASK) == PANEL4) {
      delayms = MS_SIGNAL * 4;
      panel = 4;
      }
    else {
      delayms = MS_SIGNAL * 5;
      panel = 5;
      }
    
    if(panel != 0) {
      // Stores Original Hit Points 
      oldhitpoint = hitpoint;
      // Stores time when hit
      lasthittime = millis();
      
      if (rules == 1) {
        // decrease hitpoints if panel is hit less than max     
        if(tphit[panel-1] <= MAX_PANEL_HIT) {
          // increase hit count on panel counter
          tphit[panel-1]++;
          hitpoint--;
        }
      }
      else {
        // decrease hitpoints
        hitpoint--;
      }

      // Only Pulse and Blink Board if Hit Points is lower
      if(hitpoint < oldhitpoint) {
        // hit output high
        digitalWrite(hiti, HIGH);

        // delay and reset hit output
        delay(delayms);
        digitalWrite(hiti, LOW);
        maybetransmit();
			
        // blink LED board 3 times
        for (int x = 0; x < 3; x++)
        {
          digitalWrite(hitu, HIGH);
          delay(LED_RATE);
          maybetransmit();
          digitalWrite(hitu, LOW);
          delay(LED_RATE);
          maybetransmit();
        }
        // delay for the remaining cooldown period
        while (millis() - lasthittime < 1000) {
          maybetransmit();
        }
      }
    }
    
    // reset variables
    hit = 0;
    delayms = 0;
    panel = 0;
  }

  // Optional Healing Rule
  if (rules == 2) {
    curenttime = millis();
    if (((curenttime - lasthittime) > HEALING_RATE) && (hitpoint < HEALING_HP_THRESHOLD)) {
      // increment HP
      hitpoint++;
      // reset time since last hit
      lasthittime = millis();
    }
  }

  // leave LED on board when dead
  if (hitpoint == 0) {
    digitalWrite(hitu, HIGH);
    //  let the 'mech know that the game is over
    digitalWrite(hiti, HIGH);
  } else {
    digitalWrite(hitu, LOW);
  }

    maybetransmit();
}


