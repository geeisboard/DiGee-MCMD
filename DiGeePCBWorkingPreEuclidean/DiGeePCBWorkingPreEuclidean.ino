#include "Button2.h"
#include <Encoder.h>
#include <Bounce2.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// 'Arrow', 17x10px
const unsigned char Arrow [] PROGMEM = {
	0x00, 0x08, 0x00, 0x00, 0x0c, 0x00, 0x00, 0x0e, 0x00, 0xff, 0xff, 0x00, 0xff, 0xff, 0x80, 0xff, 0xff, 0x80, 0xff, 0xff, 0x00, 0x00, 0x0e, 0x00, 0x00, 0x0c, 0x00, 0x00, 0x08, 0x00};

// 'OLEDCursor', 6x8px
const unsigned char OLEDCursor [] PROGMEM = {
	0x30, 0x30, 0x30, 0x30, 0x30, 0xfc, 0x78, 0x30};

// 'pauseIcon', 10x16px
const unsigned char pauseIcon [] PROGMEM = {
	0xf3, 0xc0, 0xf3, 0xc0, 0xf3, 0xc0, 0xf3, 0xc0, 0xf3, 0xc0, 0xf3, 0xc0, 0xf3, 0xc0, 0xf3, 0xc0, 0xf3, 0xc0, 0xf3, 0xc0, 0xf3, 0xc0, 0xf3, 0xc0, 0xf3, 0xc0, 0xf3, 0xc0, 0xf3, 0xc0, 0xf3, 0xc0};

// 'playIcon', 10x16px
const unsigned char playIcon [] PROGMEM = {
	0xe0, 0x00, 0xf0, 0x00, 0xf8, 0x00, 0xfc, 0x00, 0xfe, 0x00, 0xff, 0x00, 0xff, 0x80, 0xff, 0xc0, 0xff, 0xc0, 0xff, 0x80, 0xff, 0x00, 0xfe, 0x00, 0xfc, 0x00, 0xf8, 0x00, 0xf0, 0x00, 0xe0, 0x00};

// Array of all bitmaps for convenience.
const int epd_bitmap_allArray_LEN = 5;
const unsigned char* epd_bitmap_allArray[4] = {Arrow, OLEDCursor, playIcon, pauseIcon };

////////////////////////////////////////////
////////////// PIN DEFINITIONS /////////////
////////////////////////////////////////////

// SCREEN
#define SCREEN_WIDTH 128 
#define SCREEN_HEIGHT 64 
#define OLED_RESET     -1 
#define SCREEN_ADDRESS 0x3C
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// MIDI TX 
#define TX1 1
#define TX2 8
// MIDI CODE
#define MIDI_CLOCK 0xF8
#define MIDI_START 0xFA
#define MIDI_STOP  0xFC
#define DEFAULT 16

// ENCODER
#define SW 7
#define ENC_A 10
#define ENC_B 9
Encoder encoder(ENC_B, ENC_A);
Button2 encButton;

// BUTTONS
#define S11 16
#define S12 15
#define S13 14
#define S14 13
#define INTERVAL 30

////////////////////////////////////////////
////////////// ROTARY ENCODER //////////////
////////////////////////////////////////////

struct Counter{
  int bpm = 120;
  int bpmPoly = 120;
  int min = 60;
  int max = 240;

  void increment(int by){
    if(by>0 && bpm < max){
      bpm += by;
      if(bpm > max) bpm = max;
    }
    else if(by<0 && bpm > min){
      bpm += by;
      if(bpm < min) bpm = min;
    }
  }

  bool loop() {
    // CHECK THE ENCODER VALUE
    int encoderVal = encoder.read(); 
    int delta = encoderVal - bpm;
    if (delta!=0) {
      increment(delta); 
      encoder.write(bpm);
      return true;
    }
    return false;
  }
    
};
Counter ctr;

//////////////////////////////////////////////
////////////// BINARY BUTTONS ////////////////
//////////////////////////////////////////////

struct Binary {
  static const int size = 4;
  const int pin[size] = {S11, S12, S13, S14};
  bool state[size] = {false, false, false, false};
  Bounce2::Button bounce[size];

  Binary() {
    for (int btn = 0; btn < size; btn++) {
      bounce[btn].attach(pin[btn], INPUT_PULLUP);
      bounce[btn].setPressedState(LOW);
      bounce[btn].interval(INTERVAL);
    }
  }

  bool loop() {
    bool press = false;
    for (int btn = 0; btn < size; btn++) {
      bounce[btn].update();
      if (bounce[btn].fell()) {
        state[btn] =! state[btn];
        press = true;
      }
    }
    return press;
  }
  
  int total() { // RETURNS THE VALUE OF THE BUTTONS USING BIT SHIFTING :)
    int tally=0;
    for (int i=0;i<size;i++) {
      if (state[i]) {
        tally |= (1 << i);
      }
    }
    if (tally == 0){
      tally = 16;
    }
    return tally;
  }
};

Binary binary = Binary();

////////////////////////////////////////////
////////////// OLED OLED OLED //////////////
////////////////////////////////////////////

struct OLEDControl {
  // Controller Variables
  bool mathController = true; // Starts in DIVIDE mode - false represents MULTIPLY
  bool stateController = false; // Starts in STOP mode - true represents PLAY
  
  ////////// SCREEN VARIABLES //////////

  // BPM Variables
  int bpmClockY = 0;
  int bpmBaseClockX = 0;
  int bpmPolyClockX = 65;

  // Center Strip Variables - playState / Binary / mathMode (posY all the same but named for readability)
  // PlayState Symbol
  int stateY = 18;
  int stateX = 6;
  
  // Math Symbol
  int symbolY = 18;
  int symbolX = 110;

  // Binary Display Variables
  int binaryY = 18; // binary value height on screen
  const unsigned int binaryX[4] = {90, 70, 50, 30};

  // Equation Variables - ( 8 / 16 )
  int equationY = 45; 

  int digitLeftX = 33;
  int singleDigitLeftX = 43;
  int digitRightX = 75;
  
  // CHANGE CONTROLLER VARIABLES

  void switchMathMode() {
    mathController = !mathController;
  }
  void switchPlayState() {
    stateController = !stateController;
  }

  ////////// DISPLAY CONTROL //////////

  // Rect to update on screen
  struct Rect {
    int16_t x;
    int16_t y;
    int16_t w;
    int16_t h;
  }; 
  // Variables
  static const uint8_t MAX_RECTS = 11;
  uint8_t numUndrawnRects = 0;
  Rect undrawnRects[MAX_RECTS];

  // Append Rect to draw on updateScreen
  void appendRect(int16_t x, int16_t y, int16_t w, int16_t h) {
    if (numUndrawnRects >= MAX_RECTS) return; // prevent overflow

    for (uint8_t i = 0; i < numUndrawnRects; i++) {
      if (undrawnRects[i].x==x && undrawnRects[i].y==y) {
        return;
      }
    }
    undrawnRects[numUndrawnRects] = {x, y, w, h};
    numUndrawnRects++;
  }

  // Draw Rects from array undrawnRects - Bare metal programming
  void displayWindow(int16_t x, int16_t y, int16_t w, int16_t h) {
    // Tell the chip which columns we care about
    display.ssd1306_command(SSD1306_COLUMNADDR);
    display.ssd1306_command(x);
    display.ssd1306_command(x + w - 1);

    // Which pages (8-pixel high rows)
    uint8_t startPage = y >> 3;
    uint8_t endPage   = (y + h - 1) >> 3;
    display.ssd1306_command(SSD1306_PAGEADDR);
    display.ssd1306_command(startPage);
    display.ssd1306_command(endPage);

    // Blast the bytes to the screen
    Wire.beginTransmission(0x3C);   // library gives us the address
    Wire.write(0x40); // data stream marker
    for (uint8_t p = startPage; p <= endPage; ++p) {
      for (uint16_t c = x; c < x + w; ++c) {
        Wire.write(display.getBuffer()[c + p * 128]);
      }
    }
    Wire.endTransmission();
  }

  private: 
    ////////// WIPES //////////
    
    // Wipe BPM display
    void wipeBPM(int clock) {
      display.setTextSize(1);
      switch(clock){
        case 0:
          display.fillRect(bpmBaseClockX, bpmClockY, 48, 7, BLACK);
          display.setCursor(bpmBaseClockX, bpmClockY);
          appendRect(bpmBaseClockX, bpmClockY, 48, 7);
          break;
        case 1:
          display.fillRect(bpmPolyClockX, bpmClockY, 60, 7, BLACK);
          display.setCursor(bpmPolyClockX, bpmClockY);
          appendRect(bpmPolyClockX, bpmClockY, 48, 7);
          break;
      }
    }
    // Wipe Play/Pause symbol
    void wipePlayStateSymbol() {
      display.fillRect(stateX, stateY, 10, 16, BLACK);
      appendRect(stateX, stateY, 10, 16);
    }
    // Wipe MathMode symbol
    void wipeMathSymbol() {
      display.setTextSize(2);
      display.fillRect(symbolX, symbolY, 10, 20, BLACK);
      display.setCursor(symbolX, symbolY);
      appendRect(symbolX, symbolY, 10, 20);
    }
    // Wipe Binary digit
    void wipeBinary(int xPos) {
      display.fillRect(xPos, binaryY, 10, 20, BLACK);
      display.setCursor(xPos, binaryY);
      appendRect(xPos, binaryY, 12, 20);
    }
    // Wipe Left equation digit
    void wipeLeftDigit() {
      display.setTextSize(2);
      display.fillRect(digitLeftX, equationY, 22, 14, BLACK);
      // If we are drawing a custom value and its a single digit we need to position right a bit
      if (binary.total() < 10 && mathController) {
        display.setCursor(singleDigitLeftX, equationY);
      }
      else{
        display.setCursor(digitLeftX, equationY);
      }
      appendRect(digitLeftX, equationY, 22, 14);
    }
    // Wipe Right equation digit
    void wipeRightDigit() {
      display.setTextSize(2);
      display.fillRect(digitRightX, equationY, 22, 14, BLACK);
      display.setCursor(digitRightX, equationY);
      appendRect(digitRightX, equationY, 22, 14);
    }
    
    ////////// DISPLAY //////////

    // Display BPM information - "BPM: XXX"
    void displayBPM(int clock, int bpm) {
      switch(clock){
        // Clock 1
        case 0:
          wipeBPM(0);
          display.write("BPM: ");
          display.print(int(bpm));
          break;
        // Clock 2
        case 1:
          wipeBPM(1);
          display.write("BPM: ");
          display.print(int(bpm));
          break;
      }
    }
    // Display Play/Pause symbol - using internal state variable
    void displayPlayStateSymbol() {
      if (stateController) {
        display.drawBitmap(stateX, stateY, playIcon, 10, 16, WHITE);
      } else {
        display.drawBitmap(stateX, stateY, pauseIcon, 10, 16, WHITE);
      }
    }
    // Display Math symbol - using internal state variable
    void displayMathSymbol() {
      wipeMathSymbol();
      if (mathController) {
        display.write("%");
      }
      else {
        display.write("*");
      }
    }
    // Display custom equation number - "8"
    void displayTally() {
      display.print(binary.total());
    }
    // Display equation divisor symbol - "/"" only drawn once xx
    void displayDivisor() {
      display.setTextSize(2);
      display.setCursor(61, 45);
      display.write("/");
    }
    // Display default equation number - "16"
    void displayDefault() {
      display.write("16");
    }

  ////////// UPDATES ( PUBLIC CALLS ) //////////

  public:
    
    // Call when rotary encoder changed
    void updateBPM() {
      displayBPM(0, ctr.bpm);
      displayBPM(1, ctr.bpmPoly);
    }
    // Call when polyrhythm bpm update
    void updatePolyBPM() {
      displayBPM(1, ctr.bpmPoly);
    }
    // Call when any binary button is pressed
    void updateBinary(bool* state) {
      display.setTextSize(2);
      for (int btn = 0; btn < 4; btn++) {
        wipeBinary(binaryX[btn]);
        display.print(int(state[btn]));    
      }
    }
    // Call on equation update ( 8 / 16 )
    void updateEquation() {
      if (mathController) {
        wipeLeftDigit(); // Left side first with custom number in DIVIDE mode - 8
        displayTally();
        wipeRightDigit(); // Right side second with default number in DIVIDE mode - 16
        displayDefault();
      } else {
        wipeRightDigit(); // Right side first with custom number in MULTIPLY mode - 8
        displayTally();
        wipeLeftDigit(); // Left side second with default number in MULTIPLY mode - 16
        displayDefault();
      }
    }
    // Call on change math mode
    void updateMathMode() {
      switchMathMode(); // Change internal mathMode controller variable
      wipeMathSymbol(); // Wipe the old mathMode symbol
      displayMathSymbol(); // Draw the new mathMode symbol
      updateEquation(); // Update the equation as it will have flipped around
    }
    // Call on Play/Pause
    void updateState() {
      switchPlayState(); // Change internal playState controller variable
      wipePlayStateSymbol(); // Wipe the old playState symbol
      displayPlayStateSymbol(); // Draw new playState symbol
    }
    // Call in loop
    void updateScreen() {
      for (uint8_t i = 0; i < numUndrawnRects; i++) {
        Rect &r = undrawnRects[i];
        displayWindow(r.x, r.y, r.w, r.h);
      }
      numUndrawnRects = 0;
    }
    // Call in setup
    void displayInit() {
      display.clearDisplay(); // Clear the OLED
      display.setTextColor(WHITE); // Set the text coloUr
      updateBPM(); // Display BPM: 120 etc.
      updateState(); // Display Pause Symbol
      updateBinary(binary.state);
      updateMathMode(); // Display Math Symbol
      updateEquation(); // Display equation
      displayDivisor(); // Display "/" only called once 
      display.display(); // Draw entire screen to be safe
    }
    // Call on set euclidean mode
    void displayEuclidean() {
      display.clearDisplay(); // Clear the OLED

      // these three to stay for sure
      updateBPM(); // Display BPM: 120 etc.
      updateState(); // Display Pause Symbol
      updateBinary(binary.state); // Display binary tally 

      // prob no math mode, instead euclidean symbol?
      updateMathMode(); // Display Math Symbol

      // prob change to something representing euclidean equation
      updateEquation(); // Display equation

      // prob remove the divisor too
      displayDivisor(); // Display "/" only called once 

      // Draw entire screen to be safe
      display.display();
    }
    // Call on set polyrhythm mode
    void displayPolyrhythm() {
      display.clearDisplay(); // Clear the OLED
      updateBPM(); // Display BPM: 120 etc.
      updateState(); // Display Pause Symbol
      updateBinary(binary.state);
      updateMathMode(); // Display Math Symbol
      updateEquation(); // Display equation
      displayDivisor(); // Display "/" only called once 
      display.display(); // Draw entire screen to be safe
    }

};

OLEDControl OLED;

////////////////////////////////////////////
////////////// MATHS MATHS MATHS ///////////
////////////////////////////////////////////

struct Maths {
  // Calculates the bpm
  uint32_t calcBPM(bool divide, float bpm, float inputStep) {
    if (divide) {
      return (bpm * inputStep / 16.0f);
    } else {
      return (bpm * 16.0f / inputStep);
    }
  }
  // Swaps the inputs of input step and 16 based on which math mode we're in
  uint32_t calcTick(bool divide, float bpm, float inputStep) {
    if (divide) {
      return bpmTickInterval(bpm, inputStep, 16.0);
    } 
    else {
      return bpmTickInterval(bpm, 16.0, inputStep);
    }
  }
  // Calculates the tick interval
  uint32_t bpmTickInterval(float bpm, float inputStep, float normally16) { 
    return (60000000.0 / 24.0 / bpm) * (normally16 / inputStep);
  }
};
Maths math;

IntervalTimer clock1; //CLOCK 1 IS NORMAL
IntervalTimer clock2; //CLOCK 2 IS POLYRHYTHMIC

////////////////////////////////////////////
////////////// MIDI MESSAGES ///////////////
////////////////////////////////////////////

struct MidiTiming {
  uint32_t clockCounter = 0;
  bool timerFlag = false;
  bool mathController = true;
  int pRhythm = 16; // Holds active polyrhythm

  static MidiTiming* instance;

  void switchState(bool state) {
    mathController = state;
  }

  void midiStart(HardwareSerial &serialPort) {
    serialPort.write(MIDI_START);
  }

  void midiStop(HardwareSerial &serialPort) {
    serialPort.write(MIDI_STOP);
  }

  void midiClock(HardwareSerial &serialPort) {
    serialPort.write(MIDI_CLOCK);
    if(&serialPort==&Serial1) {
      midiClockCount();
    }
  }

  static void midiClockStatic1() {
    instance->midiClock(Serial1);
  }

  static void midiClockStatic2() {
    instance->midiClock(Serial2);
  }

  void midiClockCount() {
    
    // can this be anymore efficient? and more effective to keep the two clocks really tight
    
    clockCounter+=1;
    clockCounter=clockCounter%96;
    if (clockCounter==0&&timerFlag==true){
      // midiStop(Serial1);
      // midiStop(Serial2);
      // clock1.end();
      // clock2.end();
      pRhythm = binary.total();
      uint32_t interval1 = math.calcTick(mathController, ctr.bpm, DEFAULT);
      uint32_t interval2 = math.calcTick(mathController, ctr.bpm, pRhythm);
      clock1.update(interval1);
      clock2.update(interval2);
      timerFlag=false;
      midiStart(Serial1);
      midiStart(Serial2);
    }
  }

};

MidiTiming midi;
MidiTiming* MidiTiming::instance = nullptr;

/////////////////////////////////////////////
////////////// DEVICE CONTROLLER ////////////
/////////////////////////////////////////////

struct Device{
  enum PlayMode {
    PLAY_MODE,
    STOP_MODE
  };
  enum MathMode {    
    DIVIDE_MODE,  // Timer is counting down
    MULTIPLY_MODE   // User sets the timer duration
  };
  enum FeatureMode {
    EUCLIDEAN_MODE,
    POLYRHYTHM_MODE
  };
  PlayMode currentPlayState = PlayMode::STOP_MODE;
  MathMode currentMathState =  MathMode::DIVIDE_MODE;
  FeatureMode currentFeatureState = FeatureMode::POLYRHYTHM_MODE;

  bool mathMode() {
    if (currentMathState==DIVIDE_MODE) {
      return true;
    }
    return false;
  }
  bool featureMode() {
    if (currentFeatureState==POLYRHYTHM_MODE) {
      return true;
    }
    return false;
  }

  void switchPlayState() {
    if (currentPlayState==PLAY_MODE) {
      currentPlayState=STOP_MODE; 
      stopClocks();   
      OLED.updateState();
    }
    else {
      currentPlayState=PLAY_MODE; 
      updateClock2(); // We need to update Clock2 with the new tally once before playing, then it will auto update every Clock1 loop
      startClocks(); 
      OLED.updateState();
    }
  }
  // Long click to change feature mode between euclidean and polyrhythm
  void switchFeatureMode() {
    if (currentFeatureState==POLYRHYTHM_MODE) {
      currentFeatureState=EUCLIDEAN_MODE;
      // DO STUFF
      OLED.displayEuclidean();
    }
    else {
      currentFeatureState=POLYRHYTHM_MODE;
      // DO STUFF
      OLED.displayPolyrhythm();
    }
  }
  // Double click to change math mode
  void switchMathMode() {
    if (currentMathState==DIVIDE_MODE) {
      currentMathState=MULTIPLY_MODE;

      // Clock changes
      uint32_t interval = math.calcTick(mathMode(), ctr.bpm, midi.pRhythm);
      clock2.update(interval);
      calculatePoly();
      
      // OLED changes
      OLED.updateMathMode(); // Redraw the equation
      OLED.updatePolyBPM(); // Update only the polyrhythmic bpm
      
      // MIDI changes
      midi.switchState(mathMode());
    }
    else {
      currentMathState=DIVIDE_MODE;

      // Clock changes
      bool modeM = mathMode();
      uint32_t interval = math.calcTick(modeM, ctr.bpm, midi.pRhythm);
      clock2.update(interval);
      calculatePoly();

      // OLED changes
      OLED.updateMathMode(); // Redraw the equation
      OLED.updatePolyBPM(); // Update only the polyrhythmic bpm

      // MIDI changes 
      midi.switchState(mathMode());
    }
  }

  void onRotation() {
    updateClocks(); // Update timers immediately
    calculatePoly(); 
    OLED.updateBPM();
  }

  void onBinaryPress() {
    if(currentPlayState==PLAY_MODE) { // If we are in play mode, raise flag to change for next cycle
      midi.timerFlag = true; 
    }
    OLED.updateBinary(binary.state); // Update binary representation
    calculatePoly(); // Calculate new PolyBPM
    OLED.updateBPM(); // Update BPM
    OLED.updateEquation(); // Update Equation
  }

  // Calculate new polyrhythm for display purposes ONLY
  void calculatePoly() {
    bool modeM = mathMode();
    ctr.bpmPoly = math.calcBPM(modeM, ctr.bpm, binary.total()); 
  }

  // Call when rotation detected
  void updateClocks() {
    bool modeM = mathMode();
    uint32_t interval1 = math.calcTick(modeM, ctr.bpm, DEFAULT);
    uint32_t interval2 = math.calcTick(modeM, ctr.bpm, midi.pRhythm);
    clock1.update(interval1);
    clock2.update(interval2); 
  }

  void updateClock2() {
    bool modeM = mathMode();
    uint32_t interval = math.calcTick(modeM, ctr.bpm, midi.pRhythm);
    clock2.update(interval); 
  }

  void startClocks() {
    midi.midiStart(Serial1);
    midi.midiStart(Serial2);
  }

  void stopClocks() {
    midi.midiStop(Serial1);
    midi.midiStop(Serial2);
  }
};
Device device;

void setup() {
  // Setup serial
  Serial.begin(9600);
  Serial1.begin(31250);
  Serial2.begin(31250);

  // Setup encoder rotation
  pinMode(ENC_A, INPUT); // primary input from rotary encoder, standard pin setting NOT INTERRUPTS that was a lie that made me waste a whole day
  pinMode(ENC_B, INPUT); // secondary input from rotary encoder, standard pin setting
  encoder.write(ctr.bpm); // set default bpm to encoder

  // Setup encoder button handler
  encButton.begin(SW);
  encButton.setClickHandler([](Button2& btn){device.switchPlayState();});
  encButton.setDoubleClickHandler([](Button2& btn){device.switchMathMode();});
  encButton.setLongClickHandler([](Button2& btn){device.switchFeatureMode();});

  // Start clocks
  midi.instance = &midi;
  bool modeM = device.mathMode();
  uint32_t interval1 = math.calcTick(modeM, ctr.bpm, DEFAULT);
  uint32_t interval2 = math.calcTick(modeM, ctr.bpm, midi.pRhythm);
  clock1.begin(MidiTiming::midiClockStatic1, interval1);
  clock2.begin(MidiTiming::midiClockStatic2, interval2);

  // Initialise screen
  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
        Serial.println(F("SSD1306 allocation failed"));
        for (;;);
    }
  OLED.displayInit();
}

void loop() {
  encButton.loop(); // Check encoder button

  if(binary.loop()) { // Check binary buttons
    device.onBinaryPress(); // If true update
  } 

  if(ctr.loop()) { // Check encoder rotation
    device.onRotation(); // If true update
  } 
    
  OLED.updateScreen(); // Draw screen changes if any
}
