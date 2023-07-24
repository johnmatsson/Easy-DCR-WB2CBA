/* Si5351_SA602N DIGITAL Mode Direct Conversion RECEIVER Preselectable VFO
   This is a fork of WB2CBA's work from February 2020 : WB2CBA Barbaros Asuroglu

   The kit used for my developments was provided by F5KSE radionclub (http://ref31.r-e-f.org/)
   with the help of F4JQT (does not include the low-band filter).

   The software from WB2CBA has been changes to :
   1/ Add support for CAT control over USB serial from WSJT-X and JS8Call
   2/ Rearrange DIP switch confifuration to support multi-band operation

   CAT Control is emulating a Yaesu FT-991.
   WSJT-X and JS8Call should be configured as follows :
   - Radio : Yaesu FT-991
   - Baud rate : 115200
   - Data bits : Defaults
   - Stop bite : Default
   - Handshake : None /!\ 

    // Switch configuration //

    // Switch 0-1 = Band : 
    // Switch   |  0   |   1   |
    // -------------------------
    //   80m    |  0   |   0   |
    //   40m    |  0   |   1   |
    //   30m    |  1   |   0   |
    //   10m    |  1   |   1   |

    // Switch 2-3 = Mode :
    // Switch   |  2   |   3   |
    // -------------------------
    //   FT4    |  0   |   0   |
    //   FT8    |  0   |   1   |
    //   WSPR   |  1   |   0   |
    //   JS8    |  1   |   1   |

  ----------------------------------------------------------------------------------------
   This Sketch Uses SI5351 arduino library of Jason Milldrum NT7S

   Copyright 2015 - 2018 Paul Warren <pwarren@pwarren.id.au>
                         Jason Milldrum <milldrum@gmail.com>

  ----------------------------------------------------------------------------------------
   This sketch  is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.
   Foobar is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
   GNU General Public License for more details.
   You should have received a copy of the GNU General Public License.
   If not, see <http://www.gnu.org/licenses/>.
*/

#include <si5351.h>
#include "Wire.h"

// VFO Frequencies : If you want to use this VFO for diiferent bands than 20m Change these frequencies to corresponding bands you wish to operate on
// 80 meter
#define FT4_80M   3575000UL // FT-4
#define FT8_80M   3573000UL // FT-8
#define WSPR_80M  3568600UL // WSPR
#define JS8_80M   3578000UL // JS-8
// 40 meter
#define FT4_40M   7047500UL // FT-4
#define FT8_40M   7074000UL // FT-8
#define WSPR_40M  7038600UL // WSPR
#define JS8_40M   7078000UL // JS-8
// 30 meter
#define FT4_30M   10140000UL // FT-4
#define FT8_30M   10136000UL // FT-8
#define WSPR_30M  10138700UL // WSPR
#define JS8_30M   10130000UL // JS-8
// 20 meter
#define FT4_20M   14080000UL // FT-4
#define FT8_20M   14074000UL // FT-8
#define WSPR_20M  14095600UL // WSPR
#define JS8_20M   14078000UL // JS-8

unsigned long frequencyTable[4][4] = 
{
  {FT4_80M,FT8_80M,WSPR_80M,JS8_80M},
  {FT4_40M,FT8_40M,WSPR_40M,JS8_40M},
  {FT4_30M,FT8_30M,WSPR_30M,JS8_30M},
  {FT4_20M,FT8_20M,WSPR_20M,JS8_20M}
};

// SI5351 Crystal frequency
#define SI5351_REF     25000000UL  //change this to the frequency of the crystal on your si5351’s PCB, usually 25 or 27 MHz

// Hardware DIP switches
#define DIG0  9
#define DIG1  10
#define DIG2  11
#define DIG3  12

// Current VFO frequency
unsigned long currentFreq = 0;

// Calibration constant_ For exact frequency obtain this value from Calibration sketch and substitute.
int32_t cal_factor = 0;

// SI5351 driver instance
Si5351 si5351;

// Buffer for serial port reading
String inputString = "";      // a String to hold incoming data
bool commandComplete = false;  // whether the command is complete
// Buffer for serial port writing
char outputBuffer[20];

// DIP switces state
bool switch0State = false;
bool switch1State = false;
bool switch2State = false;
bool switch3State = false;

// Switch configuration //

// Switch 0-1 = Band : 
// Switch   |  0   |   1   |
// -------------------------
//   80m    |  0   |   0   |
//   40m    |  0   |   1   |
//   30m    |  1   |   0   |
//   10m    |  1   |   1   |

// Switch 2-3 = Mode :
// Switch   |  2   |   3   |
// -------------------------
//   FT4    |  0   |   0   |
//   FT8    |  0   |   1   |
//   WSPR   |  1   |   0   |
//   JS8    |  1   |   1   |

// Debounce dip switch reading
unsigned long lastDebounceTime = 0;   // Last time output pins was read
unsigned long debounceDelay = 200;    // The debounce time; increase if the output flickers

void setup()
{
  Serial.begin(115200); while (!Serial);
  delay(10);

  pinMode(DIG0, INPUT);
  pinMode(DIG1, INPUT);
  pinMode(DIG2, INPUT);
  pinMode(DIG3, INPUT);

  // Initialize the Si5351
  // The crystal load value needs to match in order to have an accurate calibration
  si5351.init(SI5351_CRYSTAL_LOAD_8PF, 0, 0);

  // Start VFO on FT-8 target frequency
  si5351.set_correction(cal_factor, SI5351_PLL_INPUT_XO);
  si5351.drive_strength(SI5351_CLK0, SI5351_DRIVE_2MA); // Set power
  si5351.set_clock_pwr(SI5351_CLK0, 1); // Enable the clock initially

  // reserve 200 bytes for the inputString:
  inputString.reserve(200);
  
  // Set default frequency
  setFrequency(FT8_40M);
}

void setFrequency(unsigned long newFrequency)
{
  if(newFrequency != currentFreq)
  {
    currentFreq = newFrequency;
    si5351.set_freq(currentFreq * 100, SI5351_CLK0);
    announceFrequency();
  }
}

void announceFrequency()
{ // See FT-991 CAT Operation Reference Manual for CAT communication protocol :
  // https://www.yaesu.com/downloadFile.cfm?FileID=10604&FileCatID=158&FileName=FT%2D991%5FCAT%5FOM%5FENG%5F1612%2DD0.pdf&FileContentType=application%2Fpdf  
  sprintf(outputBuffer, "FA%09lu;", currentFreq);
  Serial.println(outputBuffer);
}

void announceInfo()
{
  sprintf(outputBuffer, "IF001%09lu+000000200000;", currentFreq);
  Serial.println(outputBuffer);
}

void loop()
{
  if(readSwitches())
  { // Something changed
    int band = (((byte)switch0State)<<1)+(byte)switch1State;
    int mode = (((byte)switch2State)<<1)+(byte)switch3State;
    setFrequency(frequencyTable[band][mode]);
  }

  // Parse CAT command
  if (commandComplete) {
    if(inputString.startsWith("PS"))
    {
      Serial.print("PS1;");
    }
    else if(inputString.startsWith("AI0"))
    { // Ignore
    }
    else if(inputString.startsWith("AI"))
    {
      Serial.print("AI0;");
    }
    else if(inputString.startsWith("ID"))
    {
      Serial.print("ID0670;");
    }
    else if(inputString.startsWith("MD0;"))
    {
      Serial.print("MD02;");
    }
    else if(inputString.startsWith("NA0;"))
    {
      Serial.print("NA01;");
    }
    else if(inputString.startsWith("EX032;"))
    {
      Serial.print("EX0321;");
    }
    else if(inputString.startsWith("SH0;"))
    {
      Serial.print("SH000;");
    }
    else if(inputString.startsWith("IF;"))
    {
      announceInfo();
    }
    else if(inputString.startsWith("FA;"))
    {
      announceFrequency();
    }
    else if(inputString.startsWith("FA"))
    { // Actually set requested frequency
      unsigned long newFrequency = atol(inputString.substring(2,inputString.length()-1).c_str());
      setFrequency(newFrequency);
    }
    else
    { // Default by a simple echo of the received command
      Serial.print(inputString);
    }
    // clear the string:
    inputString = "";
    commandComplete = false;
  }
}

bool readSwitches()
{
  // Debounce
  if ((millis() - lastDebounceTime) <= debounceDelay) return false;
  lastDebounceTime = millis();

  bool switchChanged = false;

  bool curSwitch;

  // Read switch 0
  curSwitch = (digitalRead(DIG0) == LOW);
  if(curSwitch != switch0State)
  {
    switchChanged = true;
    switch0State = curSwitch;
  }

  // Read switch 1
  curSwitch = (digitalRead(DIG1) == LOW);
  if(curSwitch != switch1State)
  {
    switchChanged = true;
    switch1State = curSwitch;
  }

  // Read switch 2
  curSwitch = (digitalRead(DIG2) == LOW);
  if(curSwitch != switch2State)
  {
    switchChanged = true;
    switch2State = curSwitch;
  }

  // Read switch 3
  curSwitch = (digitalRead(DIG3) == LOW);
  if(curSwitch != switch3State)
  {
    switchChanged = true;
    switch3State = curSwitch;
  }

  return switchChanged;
}

/*
  SerialEvent occurs whenever a new data comes in the hardware serial RX. This
  routine is run between each time loop() runs, so using delay inside loop can
  delay response. Multiple bytes of data may be available.
*/
void serialEvent() {
  while (Serial.available()) {
    // get the new byte:
    char inChar = (char)Serial.read();
    // add it to the inputString:
    inputString += inChar;
    // Yaesu CAT commands are seperated by ";" character
    if (inChar == ';') {
      commandComplete = true;
    }
  }
}