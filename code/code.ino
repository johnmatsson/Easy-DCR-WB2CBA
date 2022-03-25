/* Si5351_SA602N DIGITAL Mode Direct Conversion RECEIVER Preselectable VFO
   WB2CBA Barbaros Asuroglu _ February 2020
   Preset for FT-8_WSPR_JS-8_FT-4 on 20m/14Mhz band Frequencies

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

unsigned long freq1 = 14074000UL; // FT-8 - If you want to use this VFO for diiferent bands than 20m Change these frequencies to corresponding bands you wish to operate on
unsigned long freq2 = 14095600UL; // WSPR
unsigned long freq3 = 14078000UL; // JS-8
unsigned long freq4 = 14080000UL; // FT-4

int32_t cal_factor = 0; // Calibration constant_ For exact frequency obtain this value from Calibration sketch and substitute.

#define SI5351_REF 		25000000UL  //change this to the frequency of the crystal on your si5351’s PCB, usually 25 or 27 MHz

// Hardware defines
#define DIG0  9
#define DIG1  10
#define DIG2  11
#define DIG3  12

Si5351 si5351;

void setup()
{
  Serial.begin(115200); while (!Serial);
  Serial.println("Poorman's HF RX VFO _ WB2CBA");
  Serial.println("COM setup successful");
  delay(10);

  pinMode(DIG0, INPUT);
  pinMode(DIG1, INPUT);
  pinMode(DIG2, INPUT);
  pinMode(DIG3, INPUT);

  // Initialize the Si5351
  Serial.println("Start VFO module setup");

  // The crystal load value needs to match in order to have an accurate calibration
  si5351.init(SI5351_CRYSTAL_LOAD_8PF, 0, 0);

  // Start VFO on FT-8 target frequency
  si5351.set_correction(cal_factor, SI5351_PLL_INPUT_XO);
  si5351.set_freq(freq1 * 100, SI5351_CLK0);
  si5351.drive_strength(SI5351_CLK0, SI5351_DRIVE_2MA); // Set power
  si5351.set_clock_pwr(SI5351_CLK0, 1); // Enable the clock initially
  Serial.println("VFO Module initial FT8 setup successful");

}

void loop()
{
  //-------------------- FT-8 Select -------------------
  if (digitalRead(DIG0) == LOW)
  {

    si5351.set_freq(freq1 * 100, SI5351_CLK0);
    Serial.println("VFO Module FT-8 setup successful");

  }

  //-------------------- WSPR Select -------------------
  if (digitalRead(DIG1) == LOW)
  {

    si5351.set_freq(freq2 * 100, SI5351_CLK0);
    Serial.println("VFO Module WSPR setup successful");

  }

  //-------------------- JS-8 Select -------------------
  if (digitalRead(DIG2) == LOW)
  {

    si5351.set_freq(freq3 * 100, SI5351_CLK0);
    Serial.println("VFO Module JS-8 setup successful");

  }

  //-------------------- FT-4 Select -------------------
  if (digitalRead(DIG3) == LOW)
  {

    si5351.set_freq(freq4 * 100, SI5351_CLK0);
    Serial.println("VFO Module FT-4 setup successful");

  }

  delay(2000);
}
