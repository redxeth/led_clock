// Source: https://github.com/redxeth/led_clock
//
// NOTE:  If have problems loading onto Teensy-- try higher quality USB cable!!!

//      DIGIT 1      DIGIT 2      DIGIT 3      DIGIT 4

// o    ---A---      -------      -------      -------
//      |     |      |     |      |     |      |     |
//      F     B      |     |  o   |     |      |     |
//      |     |      |     |      |     |      |     |
// o    |--G--|      |-----|      |-----|      |-----|
//      |     |      |     |  o   |     |      |     |
//      E     C      |     |      |     |      |     |
//      |     |      |     |      |     |      |     |
//      ---D---      -------      -------      -------

#define DEBUGME 0     // whether to use serial port for debugging 

#define NUM_NEG 3     // number of cathode pins
#define NUM_POS 15    // number of anode pins
#define LOOPDELAY 100  // time in uS during cycle (for now 40mS given 1/25 flash rate
#define AM 0
#define PM 1
#define FASTFACTOR 1
//#define FASTFACTOR 480 // how fast you want to speed up the clock, 1 means 1x real-time, 480 for testing...
#define CYCLELED 13    // indicate which cycle we are in

//time related stuff
unsigned long startMillis;
byte startTimeHH = 12;
byte startTimeMM = 00;
byte startTimeSS = 00;
byte startTimeAMPM = AM;

byte timeHH, timeMM, timeSS, timeAMPM; // current time
byte loopCycle = 0;

// LED pins:    1, 2, 3    <-- pins 1 and 3 get run on odd cycles, pin 2 on even cycles
//    cycle:    0, 1, 0    <-- order here is important, alternate even/odd cycles
byte neg[3] = {14,15,16};  // cathode pins on Teensy

//  LED pins:     4     5     6      9     10      11      12      13      14      15       16     17     18     19    24
//  function: dotPM dotAM    B1  E2/C1  G2/B2   D2/C2   F2/A2   A3/F3   B3/G3   C3/D3    E4/E3  G4/B4  D4/C4  F4/A4   colon
//     cycle:     0     0     1    0/1    0/1     0/1     0/1     0/1     0/1     0/1      0/1    0/1    0/1    0/1    0
byte pos[15] =  {17,   18,   12,    11,    10,      9,      8,      7,      6,      5,       4,     3,     2,    1,    0};  // anode pins on Teensy



// Each pair below indicates {cycle,anode} for each LED segment
//                     A      B      C     D      E     F      G
byte HR_10s[2][2]  =       {{1,12},{1,11}};                             // 10s HR
byte HR_1s[7][2]   = {{1,8},{1,10}, {1,9},{0,9},{0,11},{0,8},{0,10}};    // 1s HR
byte MIN_10s[7][2] = {{0,7}, {0,6}, {0,5},{1,5}, {1,4},{1,7}, {1,6}};    // 10s MIN
byte MIN_1s[7][2]  = {{1,1}, {1,3}, {1,2},{0,2}, {0,4},{0,1}, {0,3}};    // 1s MIN

byte colon[2]     = {0, 0}; // colon
byte dotPM[2]     = {0,18}; // upper dot
byte dotAM[2]     = {0,17}; // lower dot

// Lookup table to help with forming numbers
//  LED segments                 A  B  C  D  E  F  G
byte digits[10][7] =           {{1, 1, 1, 1, 1, 1, 0},  // 0
                                {0, 1, 1, 0, 0, 0, 0},  // 1
                                {1, 1, 0, 1, 1, 0, 1},  // 2
                                {1, 1, 1, 1, 0, 0, 1},  // 3
                                {0, 1, 1, 0, 0, 1, 1},  // 4
                                {1, 0, 1, 1, 0, 1, 1},  // 5
                                {1, 0, 1, 1, 1, 1, 1},  // 6
                                {1, 1, 1, 0, 0, 0, 0},  // 7
                                {1, 1, 1, 1, 1, 1, 1},  // 8
                                {1, 1, 1, 0, 0, 1, 1}}; // 9

// extra in case want to display letters             
byte letters[19][7]            {{1, 1, 1, 0, 1, 1, 1},  // A
                                {0, 0, 1, 1, 1, 1, 1},  // b
                                {1, 0, 0, 1, 1, 1, 0},  // C
                                {0, 1, 1, 1, 1, 0, 1},  // d
                                {1, 0, 0, 1, 1, 1, 1},  // E
                                {1, 0, 0, 0, 1, 1, 1},  // F
                                {1, 0, 1, 1, 1, 1, 1},  // G
                                {0, 1, 1, 0, 1, 1, 1},  // H
                                {0, 1, 1, 0, 0, 0, 0},  // I
                                {0, 1, 1, 1, 1, 0, 0},  // J
                                {0, 0, 0, 1, 1, 1, 0},  // L
                                {0, 0, 1, 0, 1, 0, 1},  // n
                                {0, 0, 1, 1, 1, 0, 1},  // o
                                {1, 1, 1, 1, 1, 1, 0},  // O                                
                                {1, 1, 0, 0, 1, 1, 1},  // P
                                {1, 0, 1, 1, 0, 1, 1},  // S
                                {0, 1, 1, 1, 1, 1, 0},  // U
                                {0, 1, 1, 1, 0, 1, 1},  // Y
                                {1, 1, 0, 1, 1, 0, 1}};  // Z

// Write out time in 2 cycles-- 
// one cycle per thing that needs to be turned on
// instead of doing it by digit.  Determine which
// segments are active based on digits, but generally
// be cycling through the digits no matter what
//
// cycle: 0  (even cycle)
//        1  (odd cycle)
//
void displayTime(byte HH, byte MM, byte cycle) {

  // HH comes in 24 hour mode, i.e. 0-23:
  // 24 HR   =>  12 HR
  // 0           12am
  // 1-9         1-9am
  // 10-11       10-11am
  // 12          12pm
  // 13-21       1-9pm
  // 22-23       10-11pm
  byte hh12 = (HH % 12); // convert 24hr to 12hr format: 0-11
  if (hh12 == 0) // convert 0 to 12
    hh12 = 12;
  
  byte HR10s_dig = (hh12 / 10); // 0 or 1
  byte HR1s_dig  = (hh12 % 10); // 0 - 9
  byte MM10s_dig = (MM / 10);
  byte MM1s_dig  = (MM % 10); 
  boolean AMPM = (HH > 11); // true if PM
  
  if (DEBUGME == 1) {
    Serial.printf("Cycle: %i, HH: %2i, MM: %2i\r\n", cycle, HH, MM);
    Serial.printf("      HR10s_dig: %i, HR1s_dig: %i, hh12: %i\r\n", HR10s_dig, HR1s_dig, hh12);
  }
  
 // cathode pins - enable or disable based on even or odd cycle
  for (int i=0; i < NUM_NEG; i++) {
    if ( i % 2 == cycle ) { // turn on those matching cycle
      digitalWrite(neg[i],LOW);
    } else {                // turn off those not matching cycle
      digitalWrite(neg[i],HIGH);
    }
  } 
  
  // HOUR 10s DIGIT
  // set all segments in digit on or off depending on if cycle matches AND digit "1" is needed
  // if cycle doesn't match, do nothing as anode may be used for a different thing
  for (int i=0; i < 2; i++) {
    if (HR_10s[i][0] == cycle) {            // only change anything if in this cycle
      if (HR10s_dig == 1)                   // if 1 digit needed, turn on segment
        digitalWrite(HR_10s[i][1],HIGH);
      else                                  // if 1 digit not needed, turn off segment
        digitalWrite(HR_10s[i][1],LOW);
    }
  }
  
  // HOUR 1s DIGIT
  for (int i=0; i < 7; i++) {
    if (HR_1s[i][0] == cycle) {            // only change anything if in this cycle
      if (digits[HR1s_dig][i] == 1)        // if needed, turn on segment
        digitalWrite(HR_1s[i][1],HIGH);
      else                                 // if not needed, turn off segment
       digitalWrite(HR_1s[i][1],LOW);
    }
  }
  
  // MINUTES 10s DIGIT
  for (int i=0; i < 7; i++) {
    if (MIN_10s[i][0] == cycle) {          // only change anything if in this cycle
      if (digits[MM10s_dig][i] == 1)       // if needed, turn on segment
        digitalWrite(MIN_10s[i][1],HIGH);
      else                                 // if not needed, turn off segment
        digitalWrite(MIN_10s[i][1],LOW);
    }
  }
  
  // MINUTES 1s DIGIT
  for (int i=0; i < 7; i++) {
    if (MIN_1s[i][0] == cycle) {           // only change anything if in this cycle
      if (digits[MM1s_dig][i] == 1)        // if needed, turn on segment
        digitalWrite(MIN_1s[i][1],HIGH);
      else                                 // if not needed, turn off segment
        digitalWrite(MIN_1s[i][1],LOW);
    }
  }
  
  // display colon
  if (colon[0] == cycle)
  digitalWrite(colon[1],HIGH);
  
  // display AM or PM
  if (dotPM[0] == cycle) {
    if (AMPM)     // PM
    digitalWrite(dotPM[1],HIGH);
    else          // AM
    digitalWrite(dotPM[1],LOW);
  }
  if (dotAM[0] == cycle) {
    if (!AMPM)     // AM
    digitalWrite(dotAM[1],HIGH);    
    else           // PM
      digitalWrite(dotAM[1],LOW);    
  }
  
}

// initial setup for inputs/outputs
void setup() {
  if (DEBUGME == 1) {
    Serial.begin(9600);
  }
  // set outputs and turn all off
  // cathode pins
  for (int i=0; i < NUM_NEG; i++) {
    pinMode(neg[i], OUTPUT);
    digitalWrite(neg[i],HIGH);
  }
  // anode pins
  for (int i=0; i < NUM_POS; i++) {
    pinMode(pos[i], OUTPUT);
    digitalWrite(pos[i],LOW);   
  }
  // mode LED
  pinMode(CYCLELED, OUTPUT);
  startMillis = millis();
 }


void measureTime() {
  unsigned long currentMillis = millis();
  unsigned long secElapsed = ((currentMillis - startMillis) * FASTFACTOR  / 1000); //figure time in seconds elapsed since program start
  unsigned long minElapsed = (secElapsed / 60);                                    // note that remainder seconds are truncated
  
  // calc current hour
  // use 24-hour time, convert when displaying back to 12-hour w/ AM/PM set
  timeHH = ((minElapsed/60) + startTimeHH) % 24;                   // results in 0-23

  // calc current minutes    
  timeMM = (minElapsed - ((minElapsed/60)*60) + startTimeMM) % 60; // results in 0-59
}

// the loop routine runs over and over again forever
// basically like main with while(1).
void loop() { 
  // update the current time values for timeHH, timeMM
  measureTime();
    
  // display time based on which cycle, odd or even, we are in
  displayTime(timeHH,timeMM,loopCycle);
  
  // determine cycle for later calc  
  if (loopCycle == 1) {
    if (DEBUGME == 1) 
      digitalWrite(CYCLELED,HIGH);
    loopCycle = 0;
  } else {
    if (DEBUGME == 1)
      digitalWrite(CYCLELED,LOW);
    loopCycle = 1;
  }
  
  if (DEBUGME == 1) 
    delay(1000);
  else
    delayMicroseconds(LOOPDELAY);
}


