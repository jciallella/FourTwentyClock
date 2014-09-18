/* ================================================================================== //
//
//    Jack's 4:20 Clock
//    Last Update: 9-7-2014
//    Version 0.95
//    Written for Arduino Uno Rev. 3
//    Adafruit Hardware: Real Time Clock: DS1307, Neopixel (40 RGB-LED) Shield,
//    7-Segment Display, 14-Segment Aphanumeric Display, Backlight Module
//
// ================================================================================== */

// Libraries
#include "Adafruit_LEDBackpack.h"
#include "Adafruit_NeoPixel.h"
#include "Adafruit_GFX.h"
#include "Wire.h"
#include "RTClib.h"
#include "Pitches.h"

// Smoothing Variables:
#define DEBOUNCE 10                           // # ms (5+ ms is usually plenty)
#define NUMBUTTONS sizeof(buttons)            // Macro determines size of array, by checking size
byte buttons[] = {2, 3, 4, 5};                // Pins of buttons to be doubounced
byte down[NUMBUTTONS], pressed[NUMBUTTONS], released[NUMBUTTONS];

// Input/Output Pins
int brightKnob =          A3;      // 7-Sebment
int minPlusButton =     2;       // Timekeeping
int minMinusButton =    3;       // Timekeeping
int hourPlusButton =    4;       // Timekeeping
int hourMinusButton =   5;       // Timekeeping
int reminderSwitch =      6;       // Switch: Reminder
int brightSwitch =        7;
int piezoOut =            8;       // Speaker
int neoPixelPin =         10;      // RGB-LED: Green

// Variables
int reminderState;                 // Switch (Reminder)
int noteCounter;                   // Counts theme music notes
boolean running =        false;    // Colon ON/OFF
int decimalTime;                   // Overall Clock Time
long previousMillis;
int neoBrightness;

//Hour Adjustment Variables
int hourCount;                   // | > For manual time adjustment
int minuteCount;                 // | ^
int adjustedHour;                // | ^
int adjustedMinute;              // | ^

// Setup Components
RTC_DS1307 RTC;
Adafruit_7segment disp = Adafruit_7segment();
Adafruit_AlphaNum4 alpha4 = Adafruit_AlphaNum4();
Adafruit_NeoPixel strip = Adafruit_NeoPixel(60, neoPixelPin, NEO_GRB + NEO_KHZ800);


// ================================================================================== //
//                                  *** SETUP ***
// ================================================================================== //

void setup()
{
  pinMode (brightKnob, INPUT);                // Potentiometer
  pinMode (brightSwitch, INPUT);              // Reminder Switch
  pinMode (reminderSwitch, INPUT);            // Reminder Switch
  pinMode (minPlusButton, INPUT);             // Minute +
  pinMode (hourPlusButton, INPUT);            // Hour +
  pinMode (minMinusButton, INPUT);            // Minute -
  pinMode (hourMinusButton, INPUT);           // Hour -
  pinMode (piezoOut, OUTPUT);                 // Speaker
  pinMode (neoPixelPin, OUTPUT);              // Neopixel

  // Debounce
  byte buttonCount;
  for (buttonCount = 0; buttonCount < NUMBUTTONS; buttonCount++)
  {
    pinMode (buttons[buttonCount], INPUT);
    digitalWrite(buttons[buttonCount], HIGH);
  }

  Serial.begin(9600);
  Wire.begin();
  RTC.begin();                                            // DSC1730 Clock
  RTC.adjust(DateTime(F(__DATE__), F(__TIME__)));         // Set time at compile
  disp.begin(0x70);                                       // Start 7 Segment
  alpha4.begin(0x71);                                     // Start Alphanumeric
  strip.begin();                                          // Start Neopixel
  strip.show();                                           // Initialize all pixels to 'off'
}


// ================================================================================== //
//                                  *** LOOP ***
// ================================================================================== //

void loop()
{
  disp.print(getDecimalTime());          // Show 12-Hour Time (7 Segment)
  displayDay();                          // Show Weekday: 14 Segment
  blinkColon();                          // Blink Colon
  adjustBrightness();                    // Check Switch & Adjust brightness
  fourTwentyCheck();                     // Check if 4:20pm & Run Alarm
  reminderSwitchCheck();                      // LED / Reminder
  debounceSwitches();                    // Debounce (check state of press)
}

// ================================================================================== //
//                          *** MAIN CLOCK FUNCTIONS ***
// ================================================================================== //

int getDecimalTime()                                    // Calculate and Adjust Hours
{
  int bounceDelay = 400;

  DateTime now = RTC.now();
  int decimalTime = now.hour() * 100 + now.minute();

  int hourPlusState = digitalRead(hourPlusButton);
  int hourMinusState = digitalRead(hourMinusButton);
  int minPlusState = digitalRead(minPlusButton);
  int minMinusState = digitalRead(minMinusButton);

  if (hourPlusState == HIGH)                                   // Hour+ Button
  {
    adjustedHour = hourCount * 100;
    decimalTime += adjustedHour;
    hourCount++;
    delay(bounceDelay);
  }

  if (hourMinusState == HIGH)                                   // Hour- Button
  {
    adjustedHour = hourCount * 100;
    decimalTime += adjustedHour;
    hourCount--;
    delay(bounceDelay);
  }

  if (minPlusState == HIGH)                                      // Minute+ Button
  {
    adjustedMinute = minuteCount + 1;
    minuteCount++;
    delay(bounceDelay);
  }

  if (minMinusState == HIGH)                                     // Minute- Button
  {
    adjustedMinute = minuteCount - 1;
    minuteCount--;
    delay(bounceDelay);
  }

  decimalTime += adjustedHour + adjustedMinute;
 
  if (now.hour() + hourCount * 100 > 1100) hourCount = 0;
  if (now.minute() + minuteCount > 58) minuteCount = 0;
  
  if (decimalTime > 1259) decimalTime -= 1200;
  if (decimalTime < 59) decimalTime += 1200;

  return decimalTime;
}


void displayDay ()                                                // Convert Day Number to Display Letters
{
  DateTime now = RTC.now();
  int daynumber = now.dayOfWeek();
  int slot[] = {0, 1, 2, 3};
  char dayLetters[4];

  char sunday[] =     {' ', 'S', 'U', 'N'};
  char monday[] =     {' ', 'M', 'O', 'N '};
  char tuesday[] =    {'T', 'U', 'E', 'S'};
  char wednesday[] =  {' ', 'W', 'E', 'D'};
  char thursday[] =   {'T', 'H', 'U', 'R'};
  char friday[] =     {' ', 'F', 'R', 'I'};
  char saturday[] =   {' ', 'S', 'A', 'T'};

  if (daynumber == 0) strcpy (dayLetters, sunday);
  if (daynumber == 1) strcpy (dayLetters, monday);
  if (daynumber == 2) strcpy (dayLetters, tuesday);
  if (daynumber == 3) strcpy (dayLetters, wednesday);
  if (daynumber == 4) strcpy (dayLetters, thursday);
  if (daynumber == 5) strcpy (dayLetters, friday);
  if (daynumber == 6) strcpy (dayLetters, saturday);

  for (int n = 0; n < 4; n++) alpha4.writeDigitAscii(slot[n], dayLetters[n]);
  alpha4.writeDisplay();
}


// ================================================================================== //
//                         *** SECONDARY CLOCK FUNCTIONS ***
// ================================================================================== //


void blinkColon()                                       // Blinks Colon
{
  long interval = 750;
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis > interval)
  {
    previousMillis = currentMillis;
    running = !running;
    disp.drawColon(running);
    disp.writeDisplay();
  }
}


void adjustBrightness()                                              // Brightness Check & Adjust
{
  int brightReading = analogRead(brightKnob);                        // Check & Map Potentiometer
  int clockBrightness = map(brightReading, 0, 1023, 1, 15);
  neoBrightness =  map(brightReading, 0, 1023, 100, 255);

  int brightSwitchRead = digitalRead(brightSwitch);
  if (brightSwitchRead == HIGH)
  {
    disp.setBrightness(clockBrightness);
    alpha4.setBrightness(clockBrightness);
  }
  else strip.setBrightness(neoBrightness);
}


void fourTwentyCheck()                                 // 420 Check, Blink & Buzz
{
  int decimalTime = getDecimalTime();
  if (decimalTime == 420)
  {
    strip.setBrightness(250);                          // Set brightness for purple/green blinking
    disp.print(getDecimalTime());                      // Write time again, so 4:19 isn't displayed
    disp.blinkRate(2);
    disp.writeDisplay();
    fourTwentyWords();
    themeMusic();
  }
  else
  {
    noTone(piezoOut);
    disp.blinkRate(0);
    alpha4.blinkRate(0);
  }
}


void fourTwentyWords()                                     // Writes "HIGH" & Blinks Alphanumeric Display
{
  beep(NOTE_G4, 500);
  beep(NOTE_G4, 500);
  beep(NOTE_F4, 250);

  char *message = "DUDE.. DUDE.. DUDE... IT'S   HIGH TIME   HIGH TIME   HIGH TIME   ";

  for (uint8_t i = 0; i < strlen(message) - 4; i++)
  {
    alpha4.blinkRate(0);
    alpha4.writeDigitAscii(0, message[i]);
    alpha4.writeDigitAscii(1, message[i + 1]);
    alpha4.writeDigitAscii(2, message[i + 2]);
    alpha4.writeDigitAscii(3, message[i + 3]);
    alpha4.writeDisplay();
    delay(200);
    alpha4.clear();
  }
  alpha4.blinkRate(2);
  alpha4.writeDigitAscii(0, 'H');
  alpha4.writeDigitAscii(1, 'I');
  alpha4.writeDigitAscii(2, 'G');
  alpha4.writeDigitAscii(3, 'H');
  alpha4.writeDisplay();
}


// ================================================================================== //
//                             *** LED FUNCTIONS ***
// ================================================================================== //


void ledMatrixOFF()
{
  strip.setBrightness(0);
  strip.show();                                        // ... both of these functions
}


void reminderSwitchCheck()                                   // Checks Switch & Activates LED
{
  int reminderState = digitalRead(reminderSwitch);        // Check Reminder Switch
  if (reminderState == HIGH) rainbowCycle(20);
  else if ((reminderState == LOW) && (decimalTime != 420)) ledMatrixOFF();
}


void beep(int note, int duration)                        // Creates Individual Notes & Alternates LED's
{
  long interval = 20;
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis > interval)
  {
    previousMillis = currentMillis;
    tone(piezoOut, note, duration);                          //Play tone on buzzerPin
    if (noteCounter % 2 == 0)                                // Alternate Green then UV LED's
    {
      uint16_t j;
      for (j = 0; j < 40 ; j++)
      {
        strip.setPixelColor(j, 189, 43, 215);    // Purple Color
        strip.show();
      }
      delay(duration);
    }
    else
    {
      uint16_t j;
      for (j = 0; j < 40 ; j++)
      {
        strip.setPixelColor(j, 143, 230, 41);    // Green Color
        strip.show();
      }
      delay(duration);
    }
    noTone(piezoOut);
    noteCounter++;
  }
}


void rainbowCycle(uint8_t wait)                             // Runs rainbow led fade
{
  strip.setBrightness(neoBrightness);
  uint16_t i, j;

  for (j = 0; j < 256 ; j++)
  {
    for (i = 0; i < strip.numPixels(); i++)
    {
      strip.setPixelColor(i, Wheel(((i * 256 / strip.numPixels()) + j) & 255));
    }
    strip.show();
    delay(wait);
  }
}

// Required for rainbowCycle()
uint32_t Wheel(byte WheelPos)                                 // Colors transition r - g - b - r - g...
{
  if (WheelPos < 85)
  {
    return strip.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
  }
  else if (WheelPos < 170)
  {
    WheelPos -= 85;
    return strip.Color(255 - WheelPos * 3, 0, WheelPos * 3);
  }
  else
  {
    WheelPos -= 170;
    return strip.Color(0, WheelPos * 3, 255 - WheelPos * 3);
  }
}


// ================================================================================== //
//                             *** OTHER FUNCTIONS ***
// ================================================================================== //


void debounceSwitches()
{
  for (byte i = 0; i < NUMBUTTONS; i++)
  {
    static byte previousstate[NUMBUTTONS];
    static byte currentstate[NUMBUTTONS];
    static long lasttime;
    byte index;

    if (millis() < lasttime) lasttime = millis();                                  // we wrapped around, try again
    if ((lasttime + DEBOUNCE) > millis()) return;                         // not enough time has passed to debounce
    lasttime = millis();                                          // waited DEBOUNCE milliseconds, reset timer

    for (index = 0; index < NUMBUTTONS; index++)
    {
      currentstate[index] = digitalRead(buttons[index]);          // read the button
      if (currentstate[index] == previousstate[index])
      {
        if ((pressed[index] == LOW) && (currentstate[index] == LOW))
        {
          pressed[index] = 1;
        }
        else if ((pressed[index] == HIGH) && (currentstate[index] == HIGH))
        {
          released[index] = 1;
        }
        pressed[index] = !currentstate[index];                   // remember, digital HIGH means NOT pressed
      }
      previousstate[index] = currentstate[index];                // keep a running tally of the buttons
    }
  }
}


void themeMusic()                                                // Plays game of thones theme
{
  for (int i = 0; i < 3; i++)
  {
    beep(NOTE_G4, 500);
    beep(NOTE_C4, 500);
    beep(NOTE_DS4, 250);
    beep(NOTE_F4, 250);
  }
  for (int i = 0; i < 3; i++)
  {
    beep(NOTE_G4, 500);
    beep(NOTE_C4, 500);
    beep(NOTE_E4, 250);
    beep(NOTE_F4, 250);
    beep(NOTE_G4, 500);
    beep(NOTE_C4, 250);
    beep(NOTE_E4, 250);
    beep(NOTE_F4, 250);
  }
  for (int i = 0; i < 3; i++)
  {
    beep(NOTE_G3, 500);
    beep(NOTE_AS3, 250);
    beep(NOTE_C4, 250);
    beep(NOTE_D4, 500);
  }
}

