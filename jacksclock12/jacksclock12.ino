/* ================================================================================== //
//    
//    Jack's 420 Clock
//    
//    *** TO DO ***
//    * Clean up both switch functions
//    * Auto Dim: Debounce / Smoothing
//    * Fix RGB LED
//
// ================================================================================== */

// Libraries
#include "Wire.h"
#include "Adafruit_LEDBackpack.h"
#include "Adafruit_GFX.h"
#include "RTClib.h"
#include "pitches.h"
#include "colordefs.h"

// Input/Output Pins
int photoCellIn = A1;         // Auto Dimming
int nightLightIn = A2;        // Backlight
int nightLightOut = 11;       // Backlight
int clockLightIn = A3;        // 7-Sebment
int greenOut = 6;             // RGB-LED: Green
int redOut = 3;               // RGB-LED: Red
int blueOut = 5;              // RGB-LED: Blue
int piezoOut = 10;            // Speaker
int dstSwitchIn = 13;         // Savings Time Switch
int reminderSwitchIn = 8;     // Reminder Switch *** NOT WORKING ***
int brightSwitchIn = 12;      // Auto Brightness
int fourTwentyLED = 9;        // LED: Green
int uvLED = 7;                // LEDs: UV

// Variables
int clockKnob;                // Potentiometer 1
int lightKnob;                // Potentiometer 2
int clockBrightness;          // 7-Segment & Alphanumeric
int lightBrightness;          // Backlight
int dstState;                 // DST
int reminderState;            // Switch (Reminder)
int autoBrightState;          // Switch (Auto-Dimmer)
int photoCellRead;            // Photo Cell
int autoBrightAverage;        // Photo Cell
long previousMillis;          // For counting of xfade timer
int fourTwentyBright = 0;     // 420 LED
int fadeAmount = 2;           // 420 LED
boolean running = false;      // Colon On/Off
int counter = 0;              // Counts notes played
int numReadings = 25;         // Smoothing function: # of Readings

// Fading Variables
unsigned long TIME_LED = 0;
unsigned long TIME_COLOUR = 0;
byte RED, GREEN, BLUE;
byte RED_A = 0;
byte GREEN_A = 0;
byte BLUE_A = 0;
int led_delay = 0;
byte colour_count = 1;

// Setup RTC Clock and Displays
RTC_DS1307 RTC;
Adafruit_7segment disp = Adafruit_7segment();
Adafruit_AlphaNum4 alpha4 = Adafruit_AlphaNum4();

// ================================================================================== //
//                                  *** SETUP ***
// ================================================================================== //

void setup()
{
  pinMode (dstSwitchIn, INPUT);         // DST Button
  pinMode (nightLightIn, INPUT);        // Nightlight Potentiometer
  pinMode (reminderSwitchIn, INPUT);    // Reminder Switch
  pinMode (brightSwitchIn, INPUT);      // Auto Brightness Switch
  pinMode (greenOut, OUTPUT);           // RGB LED
  pinMode (redOut, OUTPUT);             // RGB LED
  pinMode (blueOut, OUTPUT);            // RGB LED
  pinMode (fourTwentyLED, OUTPUT);      // Green LED
  pinMode (uvLED, OUTPUT);              // UV LED

  Serial.begin(9600);
  Wire.begin();
  RTC.begin();
  RTC.adjust(DateTime(F(__DATE__), F(__TIME__)));         // Set time at compile
  disp.begin(0x70);
  alpha4.begin(0x71);
  
  // Setup Fading LED
  RED = C1_R;
  GREEN = C1_G;
  BLUE = C1_B;
  led_delay = (colour_delay - time_at_colour) / 255;      // Get Delay Time
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
  Serial.println(reminderState);         //Turn back on when ready to test this function

      if (millis() - TIME_LED >= led_delay)
    {
      TIME_LED = millis();
      LED();                                            // Run LED Function to check & adjust values
    }
    if (millis() - TIME_COLOUR >= colour_delay)
    {
      TIME_COLOUR = millis();
      COLOUR();                                         // Run Colour Change function
    }
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


void smooth()
{
  int readings[numReadings];      // the readings from the analog input
  int index = 0;                  // the index of the current reading
  int total = 0;                  // the running total
  int average = 0;                // the average

  for (int thisReading = 0; thisReading < numReadings; thisReading++)
    readings[thisReading] = 0;
  {
    total = total - readings[index];                // subtract the last reading
    readings[index] = analogRead(photoCellIn);      // read from the sensor
    total = total + readings[index];                // add the reading to the total
    index = index + 1;                              // advance to next position in array

    if (index >= numReadings)                       // if we're at the end of the array...
      index = 0;

    average = total / numReadings;
    autoBrightAverage = average;

    delay(1);        // delay in between reads for stability
  }
}


void reminderSwitch()                                   // Checks Switch & Activates LED
{
  reminderState = digitalRead(reminderSwitchIn);        // Check Reminder Switch
  if (reminderState == 1)                               // If On: Run Crossfade Part 1 / 3
  {
    if (millis() - TIME_LED >= led_delay)
    {
      TIME_LED = millis();
      LED();                                            // Run LED Function to check & adjust values
    }
    if (millis() - TIME_COLOUR >= colour_delay)
    {
      TIME_COLOUR = millis();
      COLOUR();                                         // Run Colour Change function
    }
  }
  else
  {
    analogWrite(redOut, LOW);
    analogWrite(greenOut, LOW);
    analogWrite(blueOut, LOW);
  }
}


void LED()                                                // Crossfader Part 2 / 3
{ // Check Values and adjust "Active" Value
  if (RED != RED_A) {
    if (RED_A > RED) RED_A = RED_A - 1;
    if (RED_A < RED) RED_A++;
  }
  if (GREEN != GREEN_A) {
    if (GREEN_A > GREEN) GREEN_A = GREEN_A - 1;
    if (GREEN_A < GREEN) GREEN_A++;
  }
  if (BLUE != BLUE_A) {
    if (BLUE_A > BLUE) BLUE_A = BLUE_A - 1;
    if (BLUE_A < BLUE) BLUE_A++;
  }
  analogWrite(redOut, RED_A);
  analogWrite(greenOut, GREEN_A);
  analogWrite(blueOut, BLUE_A);
}


void COLOUR()                                               // Crossfader Part 3 of 3
{
  if (colour_count < colour_count_max) colour_count++;
  if (colour_count == 1 || colour_count == 7) colour_count = -colour_count;
  if (colour_count == 1)
  {
    RED = C1_R;
    GREEN = C1_G;
    BLUE = C1_B;
  } else if (colour_count == 2) {
    RED = C2_R;
    GREEN = C2_G;
    BLUE = C2_B;
  } else if (colour_count == 3) {
    RED = C3_R;
    GREEN = C3_G;
    BLUE = C3_B;
  } else if (colour_count == 4) {
    RED = C4_R;
    GREEN = C4_G;
    BLUE = C4_B;
  } else if (colour_count == 5) {
    RED = C5_R;
    GREEN = C5_G;
    BLUE = C5_B;
  } else if (colour_count == 6) {
    RED = C6_R;
    GREEN = C6_G;
    BLUE = C6_B;
  } else if (colour_count == 7) {
    RED = C7_R;
    GREEN = C7_G;
    BLUE = C7_B;
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
    if (counter % 2 == 0)
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

    counter++;    //Increment counter
  }
}

// ================================================================================== //
//                             *** OTHER FUNCTIONS ***
// ================================================================================== //


void adjustBrightness()                                // Brightness Check & Adjust
{
  smooth();                                            // Averages photocell readings
  
  clockKnob = analogRead(clockLightIn);                // Check & Map Potentiometers/Photocell
  clockBrightness = map(clockKnob, 0, 1023, 2, 15);
  lightKnob = analogRead(nightLightIn);
  lightBrightness = map(lightKnob, 0, 1023, 50, 255);
//  photoCellRead = analogRead(photoCellIn);
  int autoBright1 = map(autoBrightAverage, 1, 1023, 2, 15);
  int autoBright2 = map(autoBrightAverage, 1, 1023, 50, 255);

  autoBrightState = digitalRead(brightSwitchIn);          // Adjust Brightness
  if (autoBrightState == 1)
  {
    disp.setBrightness(autoBright1);
    alpha4.setBrightness(autoBright1);
    analogWrite(nightLightOut, autoBright2);
  }
  else
  {
    disp.setBrightness(clockBrightness);
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
  }
    beep(NOTE_G4, 500);
    beep(NOTE_C4, 250);
    beep(NOTE_E4, 250);
    beep(NOTE_F4, 250);
  for (int i = 0; i < 3; i++)
  {
    beep(NOTE_G3, 500);
    beep(NOTE_AS3, 250);
    beep(NOTE_C4, 250);
    beep(NOTE_D4, 500);
  }
}

