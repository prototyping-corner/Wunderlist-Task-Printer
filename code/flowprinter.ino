/*
 * Code to power the Particle Photon connected to a thermal printer.
 * Waits for a HTTP POST request then  prints it on the thermal printer
 *
 * Project details at
 * prototypingcorner.io/projects/wunderlist-task-printer
 *
 * MIT License
 *
 * Copyright (c) 2016 Prototyping Corner
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

// Include some libraies for thermal printer managment
#include "Adafruit_Thermal/Adafruit_Thermal.h"

// Configure Pins
#define BUZZER  0   // Piezo Buzzer
#define PWRLED  3   // Power Status LED
#define HBLED   2   // Heart Beat LED
#define STATLED 1   // Thermal Printer Status LED
#define BTN     4   // Input Button pin

// Define Printer in Adafruit Thermal printer Library (for sending commands)
Adafruit_Thermal printer;

// Misc. Variables to use globaly in code
const int printerBAUD = 19200;  // Printer Baud rate, see test page (9600 / 19200)
const String sepLine = "________________________________";  // 32 hypens to create boarder line
const int charactersPerLine = 16; // Characters that fit on one line of thermal paper
const int maxBodyLength = 255;

const String boxChar = "[]"; // Character to display at begining of task print mode (printTask)

void setup()
{
    // Configure Pins
    pinMode(BUZZER, OUTPUT);
    pinMode(PWRLED, OUTPUT);
    pinMode(HBLED, OUTPUT);
    pinMode(STATLED, OUTPUT);
    pinMode(BTN, INPUT_PULLUP);

    switchPWR(); // Indicate System is Starting by turning on PWRLED

    initPrinter(); // Initiate Thermal Printer

    // Initialize Particle Functions
    Particle.function("header", printHeading);  // For printing large Headers
    Particle.function("body", printBody);       // Normal text
    Particle.function("task", printTask);       // Printing Lists or Tasks - Includes boxChar [] at beginging of print
}

void loop()
{

    HB(); // Change Heart Beat to indicate system is responding
    delay(50); // Add small delay for HB
}

// Particle Functions
// For printing large headings
int printHeading(String input)
{
    // Set Status LED to indicate Proccessing state
    switchStat();

    fastPrint(sepLine); // Print Seperator Line at header
    //feed(2);

    // Set Text to HEADING
    resetText();
    printer.setSize('L');
    printer.justify('C');

    fastPrint(shorten(charactersPerLine, input));

    resetText();

    // Set Status LED to indicate end of proccessing state
    switchStat();

    return 1;
}

// For Task printer mode. Adds boxChar to begining of String.
int printTask(String input)
{
    // Initiate Printing
    switchStat();
    resetText();

    String toPrint = "";
    toPrint += boxChar;
    toPrint += " ";
    toPrint += input;

    toPrint = shorten(maxBodyLength, toPrint); // Comment out this line for text overflow

    fastPrint(toPrint); // Print string

    switchStat(); // Change Status indicator

    return 1;
}

// For printing ordinary text
int printBody(String input)
{
    // Set Status LED to indicate Proccessing state
    switchStat();

    resetText();

    fastPrint(shorten(maxBodyLength, input)); // Print Shortend input value

    fastPrint(sepLine); // Print Seperator Line

    // Set Status LED to indicate end of proccessing state
    switchStat();

    return 1;
}

// shorten function. Accepts input length (int) and input string (String)
String shorten(int length, String inputString)
{
    String outputString = "";
    int inputSize = inputString.length(); // Get length of String in characters

    // if length of String exceeds threshold
    if(inputSize > length)
    {
        char buffer[inputSize];
        inputString.toCharArray(buffer, inputSize); // Convert input string to char array

        for(int i = 0; i < (length - 3); i++)
        {
            outputString += buffer[i];
        }
        outputString += "..."; // Trailing dots
    }
    else
    {
        outputString = inputString;
    }

    return outputString; // Return formated String
}

// Power LED state switcher
bool pwrState = false; // State of PWRLED
void switchPWR()
{
    if(pwrState)
    {
        pwrState = false;
        digitalWrite(PWRLED, HIGH);
    }
    else
    {
        pwrState = true;
        digitalWrite(PWRLED, LOW);
    }
}

// Status LED state switcher
bool statState = false;
void switchStat()
{
    if(statState)
    {
        statState = false;
        digitalWrite(STATLED, HIGH);
    }
    else
    {
        statState = true;
        digitalWrite(STATLED, LOW);
    }
}

// Heartbeat LED changer (switch state of HBLED to indicate software is responding)
bool heartState = false;
void HB()
{
    if(heartState)
    {
        heartState = false;
        digitalWrite(HBLED, HIGH);
    }
    else
    {
        heartState = true;
        digitalWrite(HBLED, LOW);
    }
}

// Thermal Printer Methods
// Directly send string to thermal printer (Serial1)
void fastPrint(String toPrint)
{
    Serial1.println(toPrint);
}
// fastPrint overload to add feed at end of print
void fastPrint(String toPrint, bool feedAtEnd)
{
    fastPrint(toPrint);
    feed(2);
}

// Print Empty Paper (Feed)
void feed(int amount)
{
    for(int i = 0; i < amount; i++)
    {
        fastPrint(" ");  // Send blank lines to printer
    }
}

// Initiate Printer
void initPrinter()
{
    switchStat();                   // Turn on status LED while setting up printer
    Serial1.begin(printerBAUD);     // Setup Serial1 - RX/TX pins (Serial is USB)
    printer.begin(&Serial1);        // Configure printer in Adafruit Thermal library
    switchStat();                   // Turn off status LED (will turn on again in printerDefault method)

    printerDefault();   // Restore printer to defaults

    char printDensity = 15;
    char printBreakTime = 15;

    Serial1.write(27);
    Serial1.write(55);
    Serial1.write(7);
    Serial1.write(200); // Heat time
    Serial1.write(255); // Heat interval
    Serial1.write(30);
    Serial1.write(35);

    int printSettings = (printDensity<<4) | printBreakTime;
    Serial1.write(printSettings);

}

// Set printer to default config
void printerDefault()
{
    // Turn on status LED while preforming actions on thermal printer
    switchStat();
    printer.setDefault();
    switchStat();
}

// Reset Text Styles
void resetText()
{
    switchStat(); // Switch Status LED

    // Reset all text styles on printer
    printer.inverseOff();
    printer.justify('L');
    printer.doubleHeightOff();
    printer.underlineOff();
    printer.setSize('S');

    switchStat(); // Switch Status LED
}

// END OF CODE
