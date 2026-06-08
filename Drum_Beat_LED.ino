#include <Keypad.h>                                                                       

// shift register pins
#define DATA_PIN  10  
#define LATCH_PIN 12  
#define CLOCK_PIN 11  


int rowTarget = 0;
int colTarget = 0;

// variables that hold the state of each row
bool r1State = HIGH;
bool r2State = HIGH;
bool r3State = HIGH;
bool r4State = HIGH;
bool r5State = HIGH;
bool r6State = HIGH;
bool r7State = HIGH;
bool r8State = HIGH;

const byte keypadROWS = 4; 
const byte keypadCOLS = 4;

// P = play, MNO not used
char keys[keypadROWS][keypadCOLS] = {
  {'A','B','C','D'}, // HH
  {'E','F','G','H'}, // Snare
  {'I','J','K','L'}, // Kick
  {'M','N','O','P'}
};

byte rowPins[keypadROWS] = {5, 4, 3, 2}; 
byte colPins[keypadCOLS] = {6, 7, 8, 9}; 

Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, KP_ROWS, KP_COLS);

// column pins
#define C1 13
#define C2 A0
#define C3 A1
#define C4 A2
#define C5 A3
#define C6 A4
#define C7 A5
#define C8 0


// 0 = LED OFF, 1 = ON
bool beatMatrix[3][8] = {
  {0, 0, 0, 0, 0, 0, 0, 0}, // Row 1: HH
  {0, 0, 0, 0, 0, 0, 0, 0}, // Row 2: Snare 
  {0, 0, 0, 0, 0, 0, 0, 0}  // Row 3: Kick
};

// evt sweep animation on row 8
int displayColumn = 1;
bool isSweeping = false;
int sweepColumn = 1;
unsigned long lastSweepTime = 0;

unsigned long BPM = 216; // CHANGE THE BPM HERE !!!!!!!!!!!!!!!!!


// 60000UL / BPM = milliseconds per beat (quarter notes)
// divided by 2 = milliseconds per step (eighth Note)
unsigned long sweepSpeed = (60000UL / BPM) / 2;

// standard definitions for Kick, Snare, HIHAT
#define NOTE_KICK 36
#define NOTE_SNARE 38
#define NOTE_HIHAT 42

// sends midi data to online website
void sendMIDI(byte command, byte note, byte speed) {
  Serial.write(command);
  Serial.write(note);
  Serial.write(speed);
}



void PlayStep(int currentColumn) {
  // shuts evt off, prevents ghosting
  SelectCol(0);


  // start with all rows OFF (11111111)
  byte rowBits = 0b11111111; 
  
  // arrays are zero-indexed, physical column 1 is array column 0
  int arrayCol = currentColumn - 1;

  // if an instrument is toggled ON (1), force its bit to 0 (Ground)
  if (beatMatrix[0][arrayCol] == 1) bitWrite(rowBits, 7, 0); // Row 1
  if (beatMatrix[1][arrayCol] == 1) bitWrite(rowBits, 6, 0); // Row 2
  if (beatMatrix[2][arrayCol] == 1) bitWrite(rowBits, 5, 0); // Row 3


  // if the animation is active (boolean true) AND the display is currently drawing the sweep column
  if (isSweeping && currentColumn == sweepColumn) {
    bitWrite(rowBits, 0, 0); // force Row 8 to ground
  }

  // send data to shift register
  digitalWrite(LATCH_PIN, LOW);
  shiftOut(DATA_PIN, CLOCK_PIN, LSBFIRST, rowBits);
  digitalWrite(LATCH_PIN, HIGH);


  // goes last because the old data has to be changed first before sent out
  SelectCol(currentColumn);
}


void SelectRow(int row) {
  // use shortened if/else statements called ternary operators. if row is == 1, if yes, set low, if not, set high
  r1State = (row == 1) ? LOW : HIGH;
  r2State = (row == 2) ? LOW : HIGH;
  r3State = (row == 3) ? LOW : HIGH;
  r4State = (row == 4) ? LOW : HIGH;
  r5State = (row == 5) ? LOW : HIGH;
  r6State = (row == 6) ? LOW : HIGH;
  r7State = (row == 7) ? LOW : HIGH;
  r8State = (row == 8) ? LOW : HIGH;

  UpdateShiftRegister();
}

void SelectCol(int col) {
  if (col == 1) {digitalWrite(C1, HIGH);} else {digitalWrite(C1, LOW);}
  if (col == 2) {digitalWrite(C2, HIGH);} else {digitalWrite(C2, LOW);}
  if (col == 3) {digitalWrite(C3, HIGH);} else {digitalWrite(C3, LOW);}
  if (col == 4) {digitalWrite(C4, HIGH);} else {digitalWrite(C4, LOW);}
  if (col == 5) {digitalWrite(C5, HIGH);} else {digitalWrite(C5, LOW);}
  if (col == 6) {digitalWrite(C6, HIGH);} else {digitalWrite(C6, LOW);}
  if (col == 7) {digitalWrite(C7, HIGH);} else {digitalWrite(C7, LOW);}
  if (col == 8) {digitalWrite(C8, HIGH);} else {digitalWrite(C8, LOW);}
}

void UpdateShiftRegister() {
  byte rowBits = 0b00000000;
  bitWrite(rowBits, 7, r1State); 
  bitWrite(rowBits, 6, r2State);
  bitWrite(rowBits, 5, r3State);
  bitWrite(rowBits, 4, r4State);
  bitWrite(rowBits, 3, r5State);
  bitWrite(rowBits, 2, r6State);
  bitWrite(rowBits, 1, r7State);
  bitWrite(rowBits, 0, r8State);

  digitalWrite(LATCH_PIN, LOW);
  shiftOut(DATA_PIN, CLOCK_PIN, LSBFIRST, rowBits);
  digitalWrite(LATCH_PIN, HIGH);
}

void CycleStep(int row, int col1, int col2) {
  // STATE 0 (Both OFF) -> turn on LED 1
  if (beatMatrix[row][col1] == 0 && beatMatrix[row][col2] == 0) {
    beatMatrix[row][col1] = 1; 
  } 
  // STATE 1 (LED 1 ON) -> turn off LED 1, turn on LED 2
  else if (beatMatrix[row][col1] == 1 && beatMatrix[row][col2] == 0) {
    beatMatrix[row][col1] = 0; 
    beatMatrix[row][col2] = 1; 
  } 
  // STATE 2 (BOTH ON) -> turn on LED 1, keep LED 2 on 
  else if (beatMatrix[row][col1] == 0 && beatMatrix[row][col2] == 1) {
    beatMatrix[row][col1] = 1; 
    beatMatrix[row][col2] = 1; 
  }
  // STATE 3 (BOTH OFF) -> turn off both LEDs
  else if (beatMatrix[row][col1] == 1  && beatMatrix[row][col2] == 1) {
    beatMatrix[row][col1] = 0; 
    beatMatrix[row][col2] = 0; 
  }
}

void setup() {
  // 115200 is needed to send midi data
  Serial.begin(115200); 

  // separates pin 0 and 1 from its normal function, treat it like a normal pin. We are now able to use pin 0 as a normal pin and pin 1 as an output
  UCSR0B &= ~bit(RXEN0);

  pinMode(DATA_PIN, OUTPUT);
  pinMode(CLOCK_PIN, OUTPUT);
  pinMode(LATCH_PIN, OUTPUT);

  pinMode(C1, OUTPUT);
  pinMode(C2, OUTPUT);
  pinMode(C3, OUTPUT);
  pinMode(C4, OUTPUT);
  pinMode(C5, OUTPUT);
  pinMode(C6, OUTPUT);
  pinMode(C7, OUTPUT);
  pinMode(C8, OUTPUT);


  digitalWrite(C1, LOW);
  digitalWrite(C2, LOW);
  digitalWrite(C3, LOW);
  digitalWrite(C4, LOW);
  digitalWrite(C5, LOW);
  digitalWrite(C6, LOW);
  digitalWrite(C7, LOW);
  digitalWrite(C8, LOW); 

  SelectRow(rowTarget); 
}


void loop() {
  // constantly check for button presses
  char key = keypad.getKey();

  if (key) {
  
    // (A-D) controls Row 0 (HH)
    if (key == 'A') CycleStep(0, 0, 1); // controls Step 1 & 2
    if (key == 'B') CycleStep(0, 2, 3); // controls Step 3 & 4
    if (key == 'C') CycleStep(0, 4, 5); // controls Step 5 & 6
    if (key == 'D') CycleStep(0, 6, 7); // controls Step 7 & 8

    // (E-H) controls Row 1 (Snare)
    if (key == 'E') CycleStep(1, 0, 1); // controls Step 1 & 2
    if (key == 'F') CycleStep(1, 2, 3); // controls Step 3 & 4
    if (key == 'G') CycleStep(1, 4, 5); // controls Step 5 & 6
    if (key == 'H') CycleStep(1, 6, 7); // controls Step 7 & 8

    // (I-L) controls Row 2 (Kick)
    if (key == 'I') CycleStep(2, 0, 1); // controls Step 1 & 2
    if (key == 'J') CycleStep(2, 2, 3); // controls Step 3 & 4
    if (key == 'K') CycleStep(2, 4, 5); // controls Step 5 & 6
    if (key == 'L') CycleStep(2, 6, 7); // controls Step 7 & 8


    // sweep logic
    if (key == 'P') {
      isSweeping = !isSweeping;  // flips between true and false every press

      if (isSweeping) {
        // if we just turned it ON, reset the animation to the start
        sweepColumn = 1;           
        lastSweepTime = millis();  
      }
    }
  }
  
  if (isSweeping) {
    // this if statement keeps the led consistent, as the bpm becomes the interval between each LED step
    if (millis() - lastSweepTime >= sweepSpeed) {
      lastSweepTime = millis(); 
      int arrayCol = sweepColumn - 1;


      // Row 0: HH 
      if (beatMatrix[0][arrayCol] == 1) {
        sendMIDI(153, NOTE_HIHAT, 127); // 153 = note ON, Channel 10, as requested by online website
        sendMIDI(137, NOTE_HIHAT, 0);   // 137 = note OFF, Channel 10
      }
      // Row 1: snare
      if (beatMatrix[1][arrayCol] == 1) {
        sendMIDI(153, NOTE_SNARE, 127); 
        sendMIDI(137, NOTE_SNARE, 0);   
      }
      // Row 2: kick
      if (beatMatrix[2][arrayCol] == 1) {
        sendMIDI(153, NOTE_KICK, 127); 
        sendMIDI(137, NOTE_KICK, 0);   
      }
      sweepColumn++;            // move to the next column
      
      // if it passes column 8, loop back to 
      if (sweepColumn > 8) {
        sweepColumn = 1;     
      }
    }
  }
  
  // draw the matrix
  PlayStep(displayColumn);
  displayColumn++;
  if (displayColumn > 8){
    displayColumn = 1;
  }
}



