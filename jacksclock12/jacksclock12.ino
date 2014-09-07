/* ================================================================================== //
//
//    Jack's 420 Clock - 2014
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
#include <Adafruit_NeoPixel.h>

// Input/Output Pins
int photoCellIn =       A1;      // Auto Dimming
int nightLightIn =      A2;      // Backlight
int clockLightIn =      A3;      // 7-Sebment
int NeoPixel =          6;       // RGB-LED: Green
int uvLED =             7;       // LEDs: UV
int reminderSwitchIn =  8;       // Reminder Switch *** NOT WORKING ***
int fourTwentyLED =     9;       // LED: Green
int piezoOut =          10;      // Speaker
int nightLightOut =     11;      // Backlight
int brightSwitchIn =    12;      // Auto Brightness
int dstSwitchIn =       13;      // Savings Time Switch

// Variables
int dstState;                    // DST
int reminderState;               // Switch (Reminder)
int autoBrightState;             // Switch (Auto-Dimmer)
int photoCellRead;               // Photo Cell
int autoBrightAverage;           // Photo Cell
long previousMillis;             // For counting of xfade timer
int fadeAmount = 2;              // 420 LED
int counter;                     // Counts notes played
int numReadings = 25;            // Smoothing function: # of Readings
int neoBrightness = 100;         // Neopixel Shield
boolean running = false;         // Colon On/Off

// Setup Components
RTC_DS1307 RTC;
Adafruit_7segment disp = Adafruit_7segment();
Adafruit_AlphaNum4 alpha4 = Adafruit_AlphaNum4();
Adafruit_NeoPixel strip = Adafruit_NeoPixel(60, NeoPixel, NEO_GRB + NEO_KHZ800);


// ================================================================================== //
//                                  *** SETUP ***
// ================================================================================== //

void setup()
{
  pinMode (dstSwitchIn, INPUT);         // DST Button
  pinMode (nightLightIn, INPUT);        // Nightlight Potentiometer
  pinMode (reminderSwitchIn, INPUT);    // Reminder Switch
  pinMode (brightSwitchIn, INPUT);      // Auto Brightness Switch
  pinMode (fourTwentyLED, OUTPUT);      // Green LED
  pinMode (uvLED, OUTPUT);              // UV LED
  pinMode (NeoPixel, OUTPUT);           // Neopixel

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
  reminderSwitch();                      // LED / Reminder 
  if (reminderState == 0) strip.show();  // Turn pixels off
}

// ================================================================================== //
//                             *** CLOCK FUNCTIONS ***
// ================================================================================== //


int getDecimalTime()                                    // Calculate and Adjust Hours
{
  DateTime now = RTC.now();
  int decimalTime = now.hour() * 100 + now.minute();
  dstState = digitalRead(dstSwitchIn);                  // Check DST Switch
  if (dstState == 1) decimalTime += 100;                // + / - 1 Hour via Button
  if ((decimalTime > 1159) && !(decimalTime < 1259))
  {
    decimalTime -= 1200;
  }
  return decimalTime;
}


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


void displayDay ()                                  // Convert Day Number & Display Letters
{
  DateTime now = RTC.now();
  int daynumber = now.dayOfWeek();
  switch (daynumber)
  {
    case 0:
      alpha4.writeDigitAscii(0, 'S');
      alpha4.writeDigitAscii(1, 'U');
      alpha4.writeDigitAscii(2, 'N');
      break;

    case 1:
      alpha4.writeDigitAscii(0, 'M');
      alpha4.writeDigitAscii(1, 'O');
      alpha4.writeDigitAscii(2, 'N');
      break;

    case 2:
      alpha4.writeDigitAscii(0, 'T');
      alpha4.writeDigitAscii(1, 'U');
      alpha4.writeDigitAscii(2, 'E');
      alpha4.writeDigitAscii(3, 'S');
      break;

    case 3:
      alpha4.writeDigitAscii(0, 'W');
      alpha4.writeDigitAscii(1, 'E');
      alpha4.writeDigitAscii(2, 'D');
      break;

    case 4:
      alpha4.writeDigitAscii(0, 'T');
      alpha4.writeDigitAscii(1, 'H');
      alpha4.writeDigitAscii(2, 'U');
      alpha4.writeDigitAscii(3, 'R');
      break;

    case 5:
      alpha4.writeDigitAscii(0, 'F');
      alpha4.writeDigitAscii(1, 'R');
      alpha4.writeDigitAscii(2, 'I');
      break;

    case 6:
      alpha4.writeDigitAscii(0, 'S');
      alpha4.writeDigitAscii(1, 'A');
      alpha4.writeDigitAscii(2, 'T');
      break;
  }
  alpha4.writeDisplay();
}


void fourTwentyCheck()                                 // 420 Check, Blink & Buzz
{
  DateTime now = RTC.now();
  int decimalTime = now.hour() * 100 + now.minute();
  if (dstState == 1) decimalTime += 100;
  if (decimalTime == 420)
  {
    disp.print(getDecimalTime());                      // Write time again, so 4:19 isn't displayed
    disp.writeDisplay();
    fourTwentyWords();
    themeMusic();
  }
  else
  {
    analogWrite(fourTwentyLED, 0);
    noTone(piezoOut);
  }
}


void fourTwentyWords()                                     // Writes "HIGH" & "TIME" to Alphanumeric Display
{
  alpha4.writeDigitAscii(0, 'H');
  alpha4.writeDigitAscii(1, 'I');
  alpha4.writeDigitAscii(2, 'G');
  alpha4.writeDigitAscii(3, 'H');
  alpha4.writeDisplay();
  alpha4.clear();
}


// ================================================================================== //
//                             *** LED FUNCTIONS ***
// ================================================================================== //


void reminderSwitch()                                   // Checks Switch & Activates LED
{
  Serial.println(reminderState);                        // Turn back on when ready to test this function
  disp.drawColon(true);
  reminderState = digitalRead(reminderSwitchIn);        // Check Reminder Switch
  if (reminderState == HIGH) rainbowCycle(35);             // If On: Run Rainbow Crossfade
  if (reminderState == LOW) strip.show();                 // Turn pixels off
}


void smooth()                                           // Cleans up photocell input
{
  int readings[numReadings];                            // the readings from the analog input
  int index = 0;                                        // the index of the current reading
  int total = 0;                                        // the running total
  int average = 0;                                      // the average

  for (int thisReading = 0; thisReading < numReadings; thisReading++)
    readings[thisReading] = 0;
  {
    total = total - readings[index];                    // subtract the last reading
    readings[index] = analogRead(photoCellIn);          // read from the sensor
    total = total + readings[index];                    // add the reading to the total
    index = index + 1;                                  // advance to next position in array

    if (index >= numReadings) index = 0;                           // if we're at the end of the array...

    average = total / numReadings;
    autoBrightAverage = average;

    delay(1);                                           // delay in between reads for stability
  }
}


void beep(int note, int duration)                            // Creates Individual Notes & Alternates LED's
{
  long interval = 50;
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis > interval)
  {
    previousMillis = currentMillis;

    tone(piezoOut, note, duration);                          //Play tone on buzzerPin
    if (counter % 2 == 0)                                    // Alternate Green then UV LED's
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
    counter++;    
  }
}


void rainbowCycle(uint8_t wait)                                                            // Runs rainbow led setup
{
  uint16_t i, j;
  for (j = 0; j < 256 ; j++)                                                               // *1 cycles of all colors on wheel
  {
    for (i = 0; i < strip.numPixels(); i++)
    {
      strip.setPixelColor(i, Wheel(((i * 256 / strip.numPixels()) + j) & 255));
    }
    strip.show();
    delay(wait);
  }
}


uint32_t Wheel(byte WheelPos)                                      // The colours are a transition r - g - b - r - g.... & Input value 0 - 255 to get a color value.
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


void adjustBrightness()                                          // Brightness Check & Adjust
{
  smooth();                                                      // Averages photocell readings
  int clockKnob = analogRead(clockLightIn);                      // Check & Map Potentiometers/Photocell
  int clockBrightness = map(clockKnob, 0, 1023, 2, 15);
  int lightKnob = analogRead(nightLightIn);
  int lightBrightness = map(lightKnob, 0, 1023, 50, 255);        // Backlight
  int neoBrightness = map(lightKnob, 0, 1023, 1, 200);
  int autoBright1 = map(autoBrightAverage, 1, 1023, 2, 15);
  int autoBright2 = map(autoBrightAverage, 1, 1023, 50, 255);

  strip.setBrightness(neoBrightness);                            // Set Neopixel Brightness

  autoBrightState = digitalRead(brightSwitchIn);                 // Adjust Brightness (if Auto Switch On)
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


void themeMusic()
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

