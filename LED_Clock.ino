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
#define LOOPDELAY 8000  // time in uS for each cycle, effectively the refresh rate
                      // Will use 125 fps (8000uS delay) which 'looks' pretty good.
                      // Making faster is possible but you start to have LEDs not fully turning off between cycles.
                      // see here some human eye fps info: http://www.100fps.com/how_many_frames_can_humans_see.htm
#define AM 0
#define PM 1
#define FASTFACTOR 1   // set to 1 for 'real time', set to 480 typically for debug (8x per second)
#define CYCLELED 13    // indicate which cycle we are in

#define MODE 0         // mode want to run:  
                       // 0 = clock from starting time
                       // 1 = second/minute time counter

//time related stuff
unsigned long startMillis;
byte startTimeHH = 16;
byte startTimeMM = 55;
byte startTimeSS = 00;

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

// displayDigit
// display a value to the display
void displayDigit(byte digit, byte value, byte cycle) { 
  switch(digit) {
     case 1:
       // HOUR 10s DIGIT
       // set all segments in digit on or off depending on if cycle matches AND digit "1" is needed
       // if cycle doesn't match, do nothing as anode may be used for a different thing
       for (int i=0; i < 2; i++) {
         if (HR_10s[i][0] == cycle) {            // only change anything if in this cycle
           if (value == 1)                       // if 1 digit needed, turn on segment
             digitalWrite(HR_10s[i][1],HIGH);
           else                                  // if 1 digit not needed, turn off segment
             digitalWrite(HR_10s[i][1],LOW);
         }
       }
       break;
     case 2:
       // HOUR 1s DIGIT
       for (int i=0; i < 7; i++) {
         if (HR_1s[i][0] == cycle) {            // only change anything if in this cycle
           if (digits[value][i] == 1)           // if needed, turn on segment
             digitalWrite(HR_1s[i][1],HIGH);
           else                                 // if not needed, turn off segment
             digitalWrite(HR_1s[i][1],LOW);
         }
       }
       break;
     case 3:
       // MINUTES 10s DIGIT
       for (int i=0; i < 7; i++) {
         if (MIN_10s[i][0] == cycle) {          // only change anything if in this cycle
           if (digits[value][i] == 1)       // if needed, turn on segment
             digitalWrite(MIN_10s[i][1],HIGH);
           else                                 // if not needed, turn off segment
             digitalWrite(MIN_10s[i][1],LOW);
         }
       }
       break;
     case 4:
       for (int i=0; i < 7; i++) {
         if (MIN_1s[i][0] == cycle) {           // only change anything if in this cycle
           if (digits[value][i] == 1)        // if needed, turn on segment
             digitalWrite(MIN_1s[i][1],HIGH);
           else                                 // if not needed, turn off segment
             digitalWrite(MIN_1s[i][1],LOW);
         }
       }     
       break; 
  }
}

// Write out time in 2 cycles-- 
// one cycle per thing that needs to be turned on
// instead of doing it by digit.  Determine which
// segments are active based on digits that need to
// be displayed, but generally be cycling through the
// digits no matter what
//
// HH   : hours in 24 hour format (0-23)
// MM   : minutes (0-59)
// cycle: 0  (even cycle)
//        1  (odd cycle)
//
void displayCurrentTime(byte HH, byte MM, byte cycle) {

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
  
  // Display time digits
  displayDigit(1, HR10s_dig, cycle);
  displayDigit(2, HR1s_dig, cycle);
  displayDigit(3, MM10s_dig, cycle);
  displayDigit(4, MM1s_dig, cycle);
  
  // display colon
  displayColon(cycle);
  
  // display AM or PM 
  displayAMPM(AMPM, cycle);
  
  } 
  
  
// updates global variables timeHH and timeMM with current time
// calculated from device boot relativer to starting time above
void measureCurrentHHMM() {
  unsigned long currentMillis = millis();
  unsigned long secElapsed = ((currentMillis - startMillis) * FASTFACTOR  / 1000); // figure time in seconds elapsed since program start
  unsigned long minOfDay = (((secElapsed / 60) + (startTimeHH*60) + startTimeMM) % 1440);    // figure the time of day in minutes (0 to 1439)
  
  // calc current minutes
  timeMM = (minOfDay % 60); // 0 - 59
  
  // calc current hour
  // use 24-hour time, convert when displaying back to 12-hour w/ AM/PM set
  timeHH = (minOfDay / 60); // 0 - 23, no need to do mod 24 since minOfDay limited via mod 1440 already.
  
  }
  
void displayColon(byte cycle) {
  // display colon
  if (colon[0] == cycle)
  digitalWrite(colon[1],HIGH);
}
  
void displayAMPM(byte AMPM, byte cycle) {
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

void displayElapsedMMSS(byte cycle) {
  unsigned long currentMillis = millis();
  unsigned long secElapsed = ((currentMillis - startMillis) * FASTFACTOR  / 1000); //figure time in seconds elapsed since program start
  unsigned long minElapsed = (secElapsed / 60);                                    // note that remainder seconds are truncated
  
  byte dig1 = ((minElapsed % 20) / 10); // 0 or 1
  byte dig2  = ((minElapsed % 20) % 10); // 0 - 9
  byte dig3 = ((secElapsed % 60) / 10);
  byte dig4  = ((secElapsed % 60) % 10); 
  
  // Display time digits
  displayDigit(1, dig1, cycle);
  displayDigit(2, dig2, cycle);
  displayDigit(3, dig3, cycle);
  displayDigit(4, dig4, cycle); 

  displayColon(cycle);
  
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

// the loop routine runs over and over again forever
// basically like main with while(1).
//
// DH TBD
// add functionality to sync the time with PC when program is started!
// maybe bluetooth??
// add alarm 
// add countdown feature
//
void loop() {
  // each loop through we alternate 'cycle' which is 
  // how we drive the 4-digit 7-segment LED display

  // cathode pins - enable or disable based on even or odd cycle
  for (int i=0; i < NUM_NEG; i++) {
    if ( i % 2 == loopCycle ) { // turn on those matching cycle
      digitalWrite(neg[i],LOW);
    } else {                // turn off those not matching cycle
      digitalWrite(neg[i],HIGH);
    }
}

  // now display what we want
  switch (MODE) {
    case 0:  // DISPLAY CURRENT TIME
  // update the current time values for timeHH, timeMM
      measureCurrentHHMM();
  // display time based on which cycle, odd or even, we are in
      displayCurrentTime(timeHH,timeMM,loopCycle);
      break;
    case 1:
      displayElapsedMMSS(loopCycle);
      break; 
  }
  
  // determine cycle for next iteration of loop()
  if (loopCycle == 1) {
    if (DEBUGME == 1) 
      digitalWrite(CYCLELED,HIGH);
    loopCycle = 0;
  } else {
    if (DEBUGME == 1)
      digitalWrite(CYCLELED,LOW);
    loopCycle = 1;
  }
  
  // optional delay for display purposes
  if (DEBUGME == 1) 
    delay(1000);
  else
    delayMicroseconds(LOOPDELAY);
}


