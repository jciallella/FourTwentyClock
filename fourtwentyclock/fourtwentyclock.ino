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
#include "Wire.h"
#include "Adafruit_LEDBackpack.h"
#include "Adafruit_GFX.h"
#include "RTClib.h"
#include "pitches.h"
#include "Adafruit_NeoPixel.h"

// Debouncing Items
#define DEBOUNCE 10                           // # ms (5+ ms is usually plenty)
#define NUMBUTTONS sizeof(buttons)            // Macro determines size of array, by checking size

byte buttons[] = {2, 3, 4, 5, 12};            // Pins of buttons to be doubounced
byte down[NUMBUTTONS], pressed[NUMBUTTONS], released[NUMBUTTONS];

// Input/Output Pins
int neoLightIn =        A0;      // Neopixel Dimming
int photoCellIn =       A1;      // Auto Dimming
int nightLightIn =      A2;      // Backlight
int clockLightIn =      A3;      // 7-Sebment
int minPlusButton =     2;       // Timekeeping
int minMinusButton =    3;       // Timekeeping
int hourPlusButton =    4;       // Timekeeping
int hourMinusButton =   5;       // Timekeeping
int reminderSwitchIn =  6;       // Switch: Reminder
int uvLED =             7;       // LEDs: UV
int neoPixel =          8;       // RGB-LED: Green
int fourTwentyLED =     9;       // LED: Green
int piezoOut =          10;      // Speaker
int nightLightOut =     11;      // LED: Backlight
int brightSwitchIn =    12;      // Switch: Auto Brightness
int dstSwitchIn =       13;      // Switch: Savings Time

// Variables
int dstState;                    // DST
int reminderState;               // Switch (Reminder)
int autoBrightState;             // Switch (Auto-Dimmer)
int photoCellRead;               // Photo Cell
int autoBrightAverage;           // Photo Cell
long previousMillis;             // Timer
int noteCounter;                 // Counts theme music notes
int decimalTime;                 // Overall Clock Time
int neoBrightness =      100;    // Neopixel Shield
boolean running =        false;  // Colon ON/OFF

// Hour Adjustment Variables
int hourCount;                   // | > For manual time adjustment
int minuteCount;                 // | ^
int adjustedHour;                // | ^
int adjustedMinute;              // | ^
int dstButtonCount;              // counter for the number of button presses
int dstLastState;                // previous state of dst button

// Smoothing Variables:
const int numReadings =  12;      // Number of readings to use (speeds/slows)
int readings[numReadings];        // Readings from the analog input
int index = 0;                    // Index of the current reading
int total = 0;                    // Running total
int average = 0;                  // Final average

// Setup Components
RTC_DS1307 RTC;
Adafruit_7segment disp = Adafruit_7segment();
Adafruit_AlphaNum4 alpha4 = Adafruit_AlphaNum4();
Adafruit_NeoPixel strip = Adafruit_NeoPixel(60, neoPixel, NEO_GRB + NEO_KHZ800);


// ================================================================================== //
//                                  *** SETUP ***
// ================================================================================== //

void setup()
{
  pinMode (dstSwitchIn, INPUT);         // DST Button
  pinMode (nightLightIn, INPUT);        // Nightlight Potentiometer
  pinMode (reminderSwitchIn, INPUT);    // Reminder Switch
  pinMode (neoLightIn, INPUT);          // Neopixel Potentiometer
  pinMode (brightSwitchIn, INPUT);      // Auto Brightness Switch
  pinMode (photoCellIn, INPUT);         // Photocell
  pinMode (minPlusButton, INPUT);       // Minute +
  pinMode (hourPlusButton, INPUT);      // Hour +
  pinMode (minMinusButton, INPUT);      // Minute -
  pinMode (hourMinusButton, INPUT);     // Hour -

  pinMode (fourTwentyLED, OUTPUT);      // Green LED
  pinMode (uvLED, OUTPUT);              // UV LED
  pinMode (neoPixel, OUTPUT);           // Neopixel

  byte buttonCount;                     // Debounce Items

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
  smooth();
  adjustBrightness();                    // Check Switch & Adjust brightness
  fourTwentyCheck();                     // Check if 4:20pm & Run Alarm
  reminderSwitch();                      // LED / Reminder
  dstHold();                             // Checks for Daylight Savings activation
  debounceSwitches();                    // Debounce (check state of press)
}

// ================================================================================== //
//                          *** MAIN CLOCK FUNCTIONS ***
// ================================================================================== //


int getDecimalTime()                                    // Calculate and Adjust Hours
{
  DateTime now = RTC.now();
  int decimalTime = now.hour() * 100 + now.minute();

  int bounceDelay = 500;

  if (dstButtonCount == 1) decimalTime += 100;                // Plus/Minus 1 Hour
  if (hourCount < 12) decimalTime -= adjustedHour;
  if (minuteCount > 0) decimalTime += adjustedMinute;
  if ((decimalTime > 1159) && !(decimalTime < 1259)) decimalTime -= 1200;

  if ((decimalTime > 1200) && (decimalTime < 100)) hourCount = 0;

  int hourPlusState = digitalRead(hourPlusButton);
  int hourMinusState = digitalRead(hourMinusButton);
  int minPlusState = digitalRead(minPlusButton);
  int minMinusState = digitalRead(minMinusButton);

  if (hourPlusState == HIGH)                                   // Hour+ Button
  {
    hourCount++;
    adjustedHour = hourCount * 100;
    decimalTime += adjustedHour;
    delay(bounceDelay);
  }

  if (hourMinusState == HIGH)                                   // Hour- Button
  {
    hourCount--;
    adjustedHour = hourCount * 100;
    decimalTime += adjustedHour;
    delay(bounceDelay);
  }

  if (minPlusState == HIGH)                                      // Minute+ Button
  {
    if (now.minute() > 59) minuteCount = 0;
    adjustedMinute = minuteCount + 1;
    minuteCount++;
    delay(bounceDelay);
  }


  if (minMinusState == HIGH)                                     // Minute- Button
  {
    if (now.minute() < 1) minuteCount = 0;
    adjustedMinute = minuteCount - 1;
    minuteCount--;
    delay(bounceDelay);
  }
  return decimalTime;
}


void displayDay ()                                // Grab Day Number & Display Letters
{
  DateTime now = RTC.now();
  int daynumber = now.dayOfWeek();
  int slot[] = {0, 1, 2, 3};
  char dayLetters[4];

  char sunday[] =     {'S', 'U', 'N', ' '};
  char monday[] =     {'M', 'O', 'N', ' '};
  char tuesday[] =    {'T', 'U', 'E', 'S'};
  char wednesday[] =  {'W', 'E', 'D', ' '};
  char thursday[] =   {'T', 'H', 'U', 'R'};
  char friday[] =     {'F', 'R', 'I', ' '};
  char saturday[] =   {'S', 'A', 'T', ' '};

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
  long interval = 500;
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis > interval)
  {
    previousMillis = currentMillis;
    running = !running;
    disp.drawColon(running);
    disp.writeDisplay();
  }
}


void adjustBrightness()                                          // Brightness Check & Adjust
{


  int clockKnob = analogRead(clockLightIn);                      // Check & Map Potentiometers/Photocell
  int clockBrightness = map(clockKnob, 0, 1023, 1, 15);
  int lightKnob = analogRead(nightLightIn);
  int lightBrightness = map(lightKnob, 0, 1023, 5, 255);        // Backlight

  photoCellRead = analogRead(photoCellIn);
  int autoBright1 = map(autoBrightAverage, 300, 1023, 1, 15);
  int autoBright2 = map(autoBrightAverage, 300, 1023, 50, 255);

  autoBrightState = digitalRead(brightSwitchIn);                 // Automatically Adjust Brightness (if Switched ON)
  if (autoBrightState == 1)
  {
    disp.setBrightness(autoBright1);
    alpha4.setBrightness(autoBright1);
    analogWrite(nightLightOut, autoBright2);
  }
  else
  {
    disp.setBrightness(clockBrightness);                          // Use Potentiometers to Set Brightness
    alpha4.setBrightness(clockBrightness);
    analogWrite(nightLightOut, lightBrightness);
  }
}


void fourTwentyCheck()                                 // 420 Check, Blink & Buzz
{
  int decimalTime = getDecimalTime();
  if (decimalTime == 420)
  {
    disp.blinkRate(2);
    disp.print(getDecimalTime());                      // Write time again, so 4:19 isn't displayed
    disp.writeDisplay();
    fourTwentyWords();
    themeMusic();
  }
  else
  {
    analogWrite(fourTwentyLED, 0);
    noTone(piezoOut);
    disp.blinkRate(0);
    alpha4.blinkRate(0);
  }
}


void fourTwentyWords()                                     // Writes "HIGH" & Blinks Alphanumeric Display
{
  alpha4.blinkRate(2);
  alpha4.writeDigitAscii(0, 'H');
  alpha4.writeDigitAscii(1, 'I');
  alpha4.writeDigitAscii(2, 'G');
  alpha4.writeDigitAscii(3, 'H');
  alpha4.writeDisplay();
  alpha4.clear();
}


int dstHold()                                           // Holds button press
{
  dstState = digitalRead(dstSwitchIn);
  if (dstState != dstLastState)
    if (dstState == HIGH) dstButtonCount++;               // Compare the dstState to its previous state
  dstLastState = dstState;                              // Save current state for next loop

  if (dstButtonCount == 1);
  else dstButtonCount = 0;
}


// ================================================================================== //
//                             *** LED FUNCTIONS ***
// ================================================================================== //


void reminderSwitch()                                   // Checks Switch & Activates LED
{
  reminderState = digitalRead(reminderSwitchIn);        // Check Reminder Switch
  if (reminderState == 1) rainbowCycle(25);
  else
  {
    strip.setBrightness(0);                              // Turning off the shield requires...
    strip.show();                                        // ... both of these functions
  }
}


void beep(int note, int duration)                        // Creates Individual Notes & Alternates LED's
{
  long interval = 50;
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis > interval)
  {
    previousMillis = currentMillis;

    tone(piezoOut, note, duration);                          //Play tone on buzzerPin
    if (noteCounter % 2 == 0)                                // Alternate Green then UV LED's
    {
      digitalWrite(fourTwentyLED, HIGH);
      delay(duration);
      digitalWrite(fourTwentyLED, LOW);
    }
    else
    {
      digitalWrite(uvLED, HIGH);
      delay(duration);
      digitalWrite(uvLED, LOW);
    }
    noTone(piezoOut);
    noteCounter++;
  }
}


void rainbowCycle(uint8_t wait)                             // Runs rainbow led setup
{
  uint16_t i, j;
  int neoKnob = analogRead(neoLightIn);
  int neoBrightness = map(neoKnob, 0, 1023, 1, 225);
  strip.setBrightness(neoBrightness);                        // Set Neopixel Brightness

  for (j = 0; j < 256 ; j++)                                 // *1 cycles of all colors on wheel
  {
    for (i = 0; i < strip.numPixels(); i++)
    {
      strip.setPixelColor(i, Wheel(((i * 256 / strip.numPixels()) + j) & 255));
    }
    strip.show();
    delay(wait);
  }
}


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


void smooth()                                         // Averages photocell readings
{
  total = total - readings[index];                    // subtract the last reading
  readings[index] = analogRead(photoCellIn);          // read from the sensor
  total = total + readings[index];                    // add the reading to the total
  index = index + 1;                                  // advance to next position in array
  if (index >= numReadings) index = 0;                // if we're at the end of the array...
  average = total / numReadings;

  autoBrightAverage = average;

  delay(1);                                           // stability delay in between reads
}

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


