/* ================================================================================== //
//
//    Jack's 4:20 Clock
//    Last Update: 9-7-2014
//    Version 0.8
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

// Input/Output Pins
int neoLightIn =        A0;      // Neopixel Dimming
int photoCellIn =       A1;      // Auto Dimming
int nightLightIn =      A2;      // Backlight
int clockLightIn =      A3;      // 7-Sebment
int neoPixel =          3;       // RGB-LED: Green
int dstLightOut =       5;       // Small LED indicating day light savings
int reminderSwitchIn =  6;       // Switch: Reminder
int uvLED =             7;       // LEDs: UV
int fourTwentyLED =     9;       // LED: Green
int piezoOut =          10;      // Speaker
int nightLightOut =     11;      // LED: Backlight
int brightSwitchIn =    12;      // Switch: Auto Brightness
int dstSwitchIn =       13;      // Switch: Savings Time

// Variables
int dstState = 0;                    // DST
int reminderState;               // Switch (Reminder)
int autoBrightState;             // Switch (Auto-Dimmer)
int photoCellRead;               // Photo Cell
int autoBrightAverage;           // Photo Cell
long previousMillis;             // Timer
int noteCounter;                 // Counts theme music notes
int neoBrightness =      100;    // Neopixel Shield
boolean running =        false;  // Colon ON/OFF
  int buttonPushCounter = 0;   // counter for the number of button presses
  int lastButtonState = 0;     // previous state of the button

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

  pinMode (fourTwentyLED, OUTPUT);      // Green LED
  pinMode (uvLED, OUTPUT);              // UV LED
  pinMode (neoPixel, OUTPUT);           // Neopixel
  pinMode (dstLightOut, OUTPUT);        // DST LED

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
  dstHold();


  Serial.println(autoBrightAverage);


}

// ================================================================================== //
//                             *** CLOCK FUNCTIONS ***
// ================================================================================== //

int dstHold()                                          // Holds button press
{

  dstState = digitalRead(dstSwitchIn);     

  if (dstState != lastButtonState)
  if (dstState == HIGH) buttonPushCounter++;    // compare the dstState to its previous state
  lastButtonState = dstState;    //Save current state for next loop

  if (buttonPushCounter == 1) digitalWrite(dstLightOut, HIGH);
  else
  {
  digitalWrite(dstLightOut, LOW); 
  buttonPushCounter = 0;
  }
}


int getDecimalTime()                                    // Calculate and Adjust Hours
{
  DateTime now = RTC.now();
  int decimalTime = now.hour() * 100 + now.minute();
  if (buttonPushCounter == 1) decimalTime += 100;                // Plus/Minus 1 Hour
  if ((decimalTime > 1159) && !(decimalTime < 1259)) decimalTime -= 1200;

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


void displayDay ()                                      // Convert Day Number & Display Letters
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
  int neoBrightness = map(neoKnob, 0, 1023, 1, 240);
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


void smooth()                                           // Averages photocell readings
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


void adjustBrightness()                                          // Brightness Check & Adjust
{

  smooth();

  int clockKnob = analogRead(clockLightIn);                      // Check & Map Potentiometers/Photocell
  int clockBrightness = map(clockKnob, 0, 1023, 1, 15);
  int lightKnob = analogRead(nightLightIn);
  int lightBrightness = map(lightKnob, 0, 1023, 5, 255);        // Backlight

  photoCellRead = analogRead(photoCellIn);
  int autoBright1 = map(autoBrightAverage, 300, 1000, 2, 15);
  int autoBright2 = map(autoBrightAverage, 300, 1000, 25, 240);

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

