/*
 * Copyright Chris Petrich, 2024
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License. 
 *   
 *   File:    discover-classify_fake_DS18B20.ino
 *   Author:  Chris Petrich
 *   Version: 9 Nov 2024
 *   
 *   Source:  https://github.com/cpetrich/counterfeit_DS18B20/
 *   Documentation:  https://github.com/cpetrich/counterfeit_DS18B20/
 *   
 * This demonstration script performs tests on DS18B20 sensors to identify
 * differences from Dallas / Maxim / Analog DS18B20+ sensors.
 * It does not test the power-up state and it does not write to or test the EEPROM.
 * Tests 0, 1, and 2 use only documented commands and are safe to execute.
 * Test 3 sends undocumented commands and could conceivably mess up
 * calibration parameters of clones.
 * 
 * The sketch is INTENDED for EDUCATIONAL PURPOSES, only.
 * There may be circumstances under which the sketch permanently damages one-wire 
 * sensors in obvious or non-obvious ways.
 * (I don't think it does that to authentic sensors, but I won't guarantee
 * anything. See licence text for details.)
 * 
 * 
 * This sketch was designed for Arduino Uno. Wiring:
 * 
 * CPU Vcc         -------------- DS18B20 Vcc
 * 
 * CPU Vcc         -------\
 *                        |
 *                       [R]  <- choose resistor appropriate for supply voltage and current that the microcontroller is able to sink.
 *                        |
 * CPU pin_onewire -------o------ DS18B20 data
 * 
 * CPU GND         -------------- DS18B20 GND
 * 
 */

// Tested with OneWire Version 2.3
// https://github.com/PaulStoffregen/OneWire
#include "OneWire.h"

#define pin_onewire 7
#define pin_LED 13
#define Comm Serial

const int ms750 = 750;

OneWire *ds;

void print_hex(uint8_t value) {
  if (value < 16) Comm.write('0');
  Comm.print(value, HEX);
}

void print_array(uint8_t *data, int n, char sep = ',') {  
  int idx;
  for (idx=0; idx<n; idx++) {
    print_hex(data[idx]);
    if (idx != n-1)
      Comm.write(sep);
  }
}

void dump_w1_address_format(uint8_t *addr) {
  // output address in w1 subsystem format, i.e. without CRC  
  print_hex(addr[0]);
  Comm.print(F("-"));
  for (int i=6; i>0; i--)
    print_hex(addr[i]);
}

bool read_scratchpad(uint8_t *addr, uint8_t *buff9) {
  ds->reset();
  ds->select(addr);
  ds->write(0xBE); // read scratchpad
  int idx;
  for (idx=0; idx<9; idx++)
    buff9[idx] = ds->read();
  return 0 == OneWire::crc8(buff9, 9);
}

void setup() {
  Comm.begin(115200);

  digitalWrite(pin_LED, HIGH);
  pinMode(pin_LED, OUTPUT);

  ds = new OneWire(pin_onewire);
  
  {
    // output file name without leading path
    char file[] = __FILE__;
    int i;
    for (i = strlen(file); i > 0; i--)
      if ((file[i] == '\\') || (file[i] == '/')) {
        i++;
        break;  
      }    
    Comm.print(F("\n--- # "));
    Comm.println(&file[i]);
  }
  digitalWrite(pin_LED, LOW);
  Comm.println(F("This is the November 2024 version of discover-classify_fake_DS18B20."));
  Comm.println(F("  We are in a game of whack-a-mole. At least six new clones have"));
  Comm.println(F("  appeared on the market between 2019 and 2024. Please consider"));
  Comm.println(F("  reporting suspicious test results to help keep the sketch current."));
  Comm.println(F("This sketch will test DS18B20 sensors attached to"));
  Comm.print(F("  pin "));
  Comm.print(pin_onewire, DEC);
  Comm.println(F(" for differences with the DS18B20 produced by"));
  Comm.println(F("  Analog Devices / Maxim Integrated / Dallas Semiconductor."));
  Comm.println(F("  Details: https://github.com/cpetrich/counterfeit_DS18B20"));
  Comm.println();
}

void loop() {
  Comm.println();
  Comm.print(F("Test DS18B20 sensors attached to"));
  Comm.print(F(" pin "));
  Comm.print(pin_onewire, DEC);
  Comm.println(F("."));
  Comm.println(F("Select the test:"));
  Comm.println(F("  0. Enumerate attached sensors."));
  Comm.println();
  Comm.println(F("  1. Check temperature alarm function with a safe test according to datasheet."));
  Comm.println();
  Comm.println(F("  2. Discover clones with a safe test, using only function codes documented in the datasheet."));
  Comm.println(F("     This may be sufficient to detect clones."));
  Comm.println();
  Comm.println(F("  3. Classify clones with an agressive test, using undocumented function codes."));
  Comm.println(F("     This could conceivably mess up calibration parameters. Only use this on sensors"));
  Comm.println(F("     dedicated to testing."));
  Comm.println();
  Comm.print(F("Your choice> "));
  Comm.flush();
  while (Comm.available())
    Comm.read();

  for (;;) {
    uint32_t start = millis(); 
    while ((!Comm.available()) && (millis()-start < 1000))
      delay(50);

    digitalWrite(pin_LED, digitalRead(pin_LED) == HIGH ? LOW : HIGH);
    
    if (Comm.available()) {
      int select = Comm.read();
      Comm.write(select);
      Comm.write('\n');
      Comm.write('\n');
      Comm.flush();

      if (select == '0') return loop_enumerate();
      if (select == '1') return loop_test_alarm();
      if (select == '2') return loop_discover();
      if (select == '3') return loop_classify();
      delay(150);  // let read buffer fill
      Comm.println(F("Unknown choice."));
      return;
    }
  }
}

void loop_enumerate() {
  uint8_t addr[8];
  uint8_t buffer[9];

  {
    Comm.println(F("Sensor ROM and Current Scratchpad Content:"));
    int count = 0;
    ds->reset_search();
    while (ds->search(addr)) {
      if (!read_scratchpad(addr, buffer)) read_scratchpad(addr, buffer);
      count ++;
      Comm.print(F("  "));
      if (count < 10)
        Comm.write(' ');
      Comm.print(count, DEC);
      Comm.print(F(". "));
      print_array(addr, 8, '-');
      Comm.print(F(", "));
      dump_w1_address_format(addr);

      {
        ds->reset();
        ds->select(addr);
        ds->write(0xB4);
        uint8_t normal_power = ds->read_bit();
        if (!normal_power) {
          Comm.print(F(" (Parasite Power Mode)"));
        }
      }
      Comm.print(F(": "));

      print_array(buffer, 9, '/');
      Comm.print(F(" "));
      float T = (int16_t)((uint16_t)buffer[0] + 256 * (uint16_t)buffer[1]) / 16.0f;
      Comm.print(T);
      Comm.println(F(" oC"));
    }
    Comm.print(F("  Number of Sensors: "));
    Comm.print(count, DEC);
    Comm.println(F(".\n"));
  }
  Comm.println(F("-------------")); // indicate end
}

void loop_test_alarm() {
  const int long_wait_ms = ms750;
  uint8_t addr[8];
  uint8_t buffer[9];
  int16_t current_min_T, current_max_T; // stores only H-byte
  const int16_t window_T = 3; // allowance for temperature change. Hast to be at least 1 if resolution > 9 bit.
  int attached_sensor_count = 0;
  bool detected_alarm_implementation_error = false;

  // we set the alarm thresholds based on the current temperature
  // rather than using hard-coded values to be able to verify if
  // the chips actually do a comparisons.
  // For this test the temperatures need to be reasonably constant.
  //
  // As of 2024, I am not aware of sensors that fail this test.

  {
    Comm.println(F("1. Trigger Temperature Conversion of All Sensors"));
    ds->reset();
    ds->write(0xCC); // skip ROM ==> all sensors
    ds->write(0x44, 1); // perform temperature conversion
    delay(long_wait_ms);
    ds->depower();
  }

  {
    Comm.println(F("2. Find Current Temperature Range"));
    current_min_T = (int8_t)0x7F;
    current_max_T = (int8_t)0x80;
    int count = 0;
    ds->reset_search();
    while (ds->search(addr)) {
      if (!read_scratchpad(addr, buffer)) read_scratchpad(addr, buffer);
      current_min_T = min(current_min_T, (int16_t)(buffer[1]*16 + buffer[0] / 16));
      current_max_T = max(current_max_T, (int16_t)(buffer[1]*16 + buffer[0] / 16));

      Comm.print(F("  "));
      print_array(addr, 8, '-');
      Comm.print(F(": "));

      print_array(buffer, 9, '/'); // dump scratchpad
      Comm.print(F(" "));

      float T = (int16_t)(buffer[0] + 256 * buffer[1]) / 16.0f;
      Comm.print(T);
      Comm.print(F(" oC"));
      Comm.println();

      count ++;
    }
    attached_sensor_count = count;
    Comm.print(F("  Number of Sensors: "));
    Comm.print(attached_sensor_count, DEC);
    Comm.println(F(".\n"));
  }

  {
    Comm.println(F("3. Set Alarm Registers to Trigger High Temperature Alarm"));
    // trigger high temperature alarm
    uint8_t alarm_H = ((current_min_T - window_T) & 0xFF);
    uint8_t alarm_L = alarm_H - 1;
    int count = 0;
    ds->reset_search();
    while (ds->search(addr)) {
      ds->reset();
      ds->select(addr);
      ds->write(0x4E);
      ds->write(alarm_H);
      ds->write(alarm_L);
      ds->write(0x7F); // 12-bit conversion

      Comm.print(F("  "));
      print_array(addr, 8, '-');
      Comm.print(F(": "));

      if (!read_scratchpad(addr, buffer)) read_scratchpad(addr, buffer);

      print_array(&buffer[2], 3, '/');

      if ((buffer[2] == alarm_H) && (buffer[3] == alarm_L) && (buffer[4] == 0x7F))
        Comm.println(F(" Ok."));
      else
        Comm.println(F(" Unsuccessful Write. Error."));
      
      count ++;
    }
    if (count != attached_sensor_count)
      Comm.println(F("  Inconsistent Sensor Count, Check Connections!\n"));
  }

  {
    Comm.println(F("4. Trigger Temperature Conversion of All Sensors"));
    ds->reset();
    ds->write(0xCC); // skip ROM ==> all sensors
    ds->write(0x44, 1); // perform temperature conversion
    delay(long_wait_ms);
    ds->depower();
  }

  {
    Comm.println(F("5. Find Sensors Signalling Alarm"));
    Comm.println(F("   (this should be all sensors)"));
    int count = 0;
    ds->reset_search();
    while (ds->search(addr, 0)) {
      if (!read_scratchpad(addr, buffer)) read_scratchpad(addr, buffer);

      Comm.print(F("  "));
      print_array(addr, 8, '-');
      Comm.print(F(": "));
      print_array(buffer, 9, '/'); // dump scratchpad
      Comm.print(F(" "));
      float T = (int16_t)(buffer[0] + 256 * buffer[1]) / 16.0f;
      Comm.print(T);
      Comm.println(F(" oC"));

      count++;
    }
    Comm.print(F("  Number of Sensors: "));
    Comm.print(count, DEC);
    Comm.print(F("."));

    if (count == attached_sensor_count)
      Comm.println(F(" Ok."));
    else if (count < attached_sensor_count) {
      Comm.println(F("  ** Not all sensors raised alarm!!! **"));
      detected_alarm_implementation_error = true;
    }
    else if (count > attached_sensor_count)
      Comm.println(F("  Inconsistent sensor count, check connections!"));
    Comm.println();
  }

{
    Comm.println(F("6. Set Alarm Registers to Trigger Low Temperature Alarm"));
    // trigger low temperature alarm
    uint8_t alarm_L = ((current_max_T + window_T) & 0xFF);
    uint8_t alarm_H = alarm_L + 1;
    int count = 0;
    ds->reset_search();
    while (ds->search(addr)) {
      ds->reset();
      ds->select(addr);
      ds->write(0x4E);
      ds->write(alarm_H);
      ds->write(alarm_L);
      ds->write(0x7F); // 12-bit conversion

      Comm.print(F("  "));
      print_array(addr, 8, '-');
      Comm.print(F(": "));

      if (!read_scratchpad(addr, buffer)) read_scratchpad(addr, buffer);

      print_array(&buffer[2], 3, '/');

      if ((buffer[2] == alarm_H) && (buffer[3] == alarm_L) && (buffer[4] == 0x7F))
        Comm.println(F(" Ok."));
      else
        Comm.println(F(" Unsuccessful Write. Error."));
      
      count ++;
    }
    if (count != attached_sensor_count)
      Comm.println(F("  Inconsistent Sensor Count, Check Connections!\n"));
  }

  {
    Comm.println(F("7. Trigger Temperature Conversion of All Sensors"));
    ds->reset();
    ds->write(0xCC); // skip ROM ==> all sensors
    ds->write(0x44, 1); // perform temperature conversion
    delay(long_wait_ms);
    ds->depower();
  }

  {
    Comm.println(F("8. Find Sensors Signalling Alarm"));
    Comm.println(F("   (this should be all sensors)"));
    int count = 0;
    ds->reset_search();
    while (ds->search(addr, 0)) {
      if (!read_scratchpad(addr, buffer)) read_scratchpad(addr, buffer);

      Comm.print(F("  "));
      print_array(addr, 8, '-');
      Comm.print(F(": "));
      print_array(buffer, 9, '/'); // dump scratchpad
      Comm.print(F(" "));
      float T = (int16_t)(buffer[0] + 256 * buffer[1]) / 16.0f;
      Comm.print(T);
      Comm.println(F(" oC"));

      count++;
    }
    Comm.print(F("  Number of Sensors: "));
    Comm.print(count, DEC);
    Comm.print(F("."));

    if (count == attached_sensor_count)
      Comm.println(F(" Ok."));
    else if (count < attached_sensor_count) {
      Comm.println(F("  ** Not all sensors raised alarm!!! **"));
      detected_alarm_implementation_error = true;
    }
    else if (count > attached_sensor_count)
      Comm.println(F("  Inconsistent sensor count, check connections!"));
    Comm.println();
  }

  {
    Comm.println(F("9. Set Alarm Registers to Not Trigger"));
    uint8_t alarm_L = ((current_min_T - window_T) & 0xFF);
    uint8_t alarm_H = ((current_max_T + window_T) & 0xFF);
    ds->reset_search();
    while (ds->search(addr)) {
      ds->reset();
      ds->select(addr);
      ds->write(0x4E);
      ds->write(alarm_H);
      ds->write(alarm_L);
      ds->write(0x7F); // 12-bit conversion

      Comm.print(F("  "));
      print_array(addr, 8, '-');
      Comm.print(F(": "));

      if (!read_scratchpad(addr, buffer)) read_scratchpad(addr, buffer);

      print_array(&buffer[2], 3, '/');

      if ((buffer[2] == alarm_H) && (buffer[3] == alarm_L) && (buffer[4] == 0x7F))
        Comm.println(F(" Ok."));
      else
        Comm.println(F(" Unsuccessful Write. Error."));
    }
    Comm.println();
  }

  {
    Comm.println(F("10. Trigger Temperature Conversion of All Sensors"));
    ds->reset();
    ds->write(0xCC); // skip ROM ==> all sensors
    ds->write(0x44, 1); // perform temperature conversion
    delay(long_wait_ms);
    ds->depower();
  }

  {
    Comm.println(F("11. Find Sensors Signalling Alarm"));
    Comm.println(F("   (this should be no sensors)"));
    int count = 0;
    ds->reset_search();
    while (ds->search(addr, 0)) {
      if (!read_scratchpad(addr, buffer)) read_scratchpad(addr, buffer);

      Comm.print(F("  "));
      print_array(addr, 8, '-');
      Comm.print(F(": "));
      print_array(buffer, 9, '/'); // dump scratchpad
      Comm.print(F(" "));
      float T = (int16_t)(buffer[0] + 256 * buffer[1]) / 16.0f;
      Comm.print(T);
      Comm.println(F(" oC"));

      count++;
    }
    Comm.print(F("  Number of Sensors: "));
    Comm.print(count, DEC);
    Comm.print(F("."));

    if (count == 0)
      Comm.println(F(" Ok."));
    else {
      Comm.println(F("  ** Sensors raised alarm even though they should not!!! **"));
      detected_alarm_implementation_error = true;
    }
    Comm.println();
  }

  {
    Comm.println(F("12. Request All Sensors to Recall EEPROM"));
    ds->reset();
    ds->write(0xCC); // skip ROM ==> all sensors
    ds->write(0xB8); // recall alarm register and config from EEPROM
    delay(10);
  }

  Comm.println();
  if (detected_alarm_implementation_error) {
    Comm.println(F("** Test Revealed Alarm Implementation Errors **"));
    Comm.println(F("   (assuming sensor temperatures did not change too much)"));
  } else
    Comm.println(F("No Alarm Implementation Errors Found"));

  Comm.println();

  Comm.println(F("-------------")); // indicate end
}

void loop_discover() { // this is the safe choice
  // ROM address of current sensor
  uint8_t addr[8];
  // buffers for scratchpad register
  uint8_t buffer0[9];
  uint8_t buffer1[9];
  uint8_t buffer2[9];
  uint8_t buffer3[9];
  // flag to indicate if validation
  //  should be repeated at a different
  //  sensor temperature
  bool t_ok;

  ds->reset_search();
  while (ds->search(addr)) {
    int fake_flags = 0;
    
    print_array(addr, 8, '-');
    if (0 != OneWire::crc8(addr, 8)) {
      // some clones can have their ROM overwritten with
      // arbitrary nonsense, so we don't expect anything good
      // if the ROM doesn't check out
      fake_flags += 1;
      Comm.print(F(" (CRC Error -> Error.)"));
    }

    if ((addr[6] != 0) || (addr[5] != 0) || (addr[0] != 0x28)) {
      // as of 2024: catches all families but A1 and A3
      fake_flags += 1;
      Comm.print(F(": ROM does not follow expected pattern 28-xx-xx-xx-xx-00-00-crc. Error."));
    } else if ((addr[6] == 0) && (addr[5] == 0) && (addr[4] == 0) && (addr[0] == 0x28)) {
      // catches Family A3
      fake_flags += 1;
      Comm.print(F(": ROM pattern pre-dates C4 die version, suggesting the chip is either"));
      Comm.print(F(" over 15 years old or a clone.\n                         Assuming sensor is a clone. Error."));
    } else {
      Comm.print(F(": ROM ok."));
    }    
    Comm.println();
    
    if (!read_scratchpad(addr, buffer0)) read_scratchpad(addr, buffer0);
    
    Comm.print(F("  Scratchpad Register: "));
    print_array(buffer0, 9, '/');
    if (0 != OneWire::crc8(buffer0, 9)) {
      // Unlikely that a sensor will mess up the CRC of the scratchpad.
      // --> Assume we're dealing with a bad connection rather than a bad 
      //     sensor, dump data, and move on to next sensor.
      Comm.println(F("  CRC Error. Check connections or replace sensor."));
      continue;      
    }
    Comm.println();

    {
      // query sensor whether it believes to be in parasite power mode
      ds->reset();
      ds->select(addr);
      ds->write(0xB4);
      uint8_t normal_power = ds->read_bit();
      Comm.print(F("  Info only: "));
      if (normal_power) {
        // this is good because it means we can perform timing measurements toward the end
        Comm.println(F("Sensor is not reporting Parasite Power Mode. Ok."));
      } else {
        Comm.println(F("Sensor is reporting to be in Parasite Power Mode."));
        Comm.println(F("             Note that some parts of this need the sensor"));
        Comm.println(F("             to not be in Parasite Power Mode. Hence:"));
        Comm.println(F("             ** THE RESULT OF THIS TEST IS INVALID **"));
      }
    }

    // Check content of values loaded from EEPROM. Since the EEPROM may have been
    // programmed by the user earlier we do not use this as a test. Rather, we dump this as info.
    Comm.print(F("  Info only: Scratchpad bytes 2,3,4 ("));
    print_array(buffer0+2,3,'/');
    Comm.print(F("):"));
    if ((buffer0[2] != 0x4b) || (buffer0[3] != 0x46) || (buffer0[4] != 0x7f))
      Comm.println(F(" not DS18B20 default values 4B/46/7F."));
    else
      Comm.println(F(" DS18B20 default values."));

    Comm.print(F("  Scratchpad <byte 5> = 0x"));
    print_hex(buffer0[5]);
    Comm.print(F(":"));
    if (buffer0[5] != 0xff) {
      // potentially catches Families B1 (but not B1v2), B2, D1, and D2
      fake_flags += 1;
      Comm.println(F(" should have been 0xFF according to datasheet. Error."));
    } else {
      Comm.println(F(" ok."));
    }

    Comm.print(F("  Scratchpad <byte 6> = 0x"));
    print_hex(buffer0[6]);
    Comm.print(F(":"));
    if ( ((buffer0[6] == 0x00) || (buffer0[6] > 0x10)) || // totall wrong value
         ( ((buffer0[0] != 0x50) || (buffer0[1] != 0x05)) && ((buffer0[0] != 0xff) || (buffer0[1] != 0x07)) && // check for valid conversion...
           (buffer0[6] != (0x10 - (buffer0[0] & 0x0f))) ) ) { //...before assessing DS18S20 compatibility.
      // this will typically catch Families B1 (but not B1v2), B2, C, D1, D2, F, G after a temperature conversion
      fake_flags += 1;
      Comm.println(F(" unexpected value. Error."));
    } else
      Comm.println(F(" ok."));
    
    Comm.print(F("  Scratchpad <byte 7> = 0x"));
    print_hex(buffer0[7]);
    Comm.print(F(":"));
    if (buffer0[7] != 0x10) {
      // this may catch Family B1, B2, D1, D2 (but not B1v2)
      fake_flags += 1;
      Comm.println(F(" should have been 0x10 according to datasheet. Error."));
    } else {
      Comm.println(F(" ok."));
    }

    {
      // send 5 bytes into scratchpad and read back
      ds->reset();
      ds->select(addr);
      ds->write(0x4E); // write scratchpad. MUST be followed by 3 bytes as per datasheet.
      ds->write(buffer0[2]);
      ds->write(buffer0[3]);
      ds->write(0x7F);
      ds->write(0x11);
      ds->write(0x22);
      ds->reset();
      if (!read_scratchpad(addr, buffer1)) read_scratchpad(addr, buffer1);
      Comm.print(F("  Last three bytes of scratchpad after sending 5 bytes: "));
      print_array(&buffer1[5],3,'/');
      // bytes 5 and 7 should be fixed default values, and byte 6 should not have changed.
      if ((buffer1[5] != 0xFF) || (buffer1[6] != buffer0[6]) || (buffer1[7] != 0x10)) {
        // catching Families B1 (but not B1v2) and B2
        fake_flags += 1;
        Comm.println(F(" unexpected (expected FF/<unchanged>/10). Error."));
      } else {
        Comm.println(F(" ok."));
      }
    }

    {
      ds->reset();
      ds->select(addr);
      ds->write(0x4E); // write scratchpad. MUST be followed by 3 bytes as per datasheet.
      ds->write(buffer0[2]);
      ds->write(buffer0[3]);
      ds->write(0x00);
      ds->reset();
      if (!read_scratchpad(addr, buffer1)) read_scratchpad(addr, buffer1);
      Comm.print(F("  Scratchpad config register <byte 4> after writing 0x00: 0x"));
      print_hex(buffer1[4]);
      if (buffer1[4] != 0x1F) {
        // catches Families C and F
        fake_flags += 1;
        Comm.println(F(" unexpected (expected 1F). Error."));
      } else {
        Comm.println(F(" ok."));
      }

      ds->reset();
      ds->select(addr);
      ds->write(0x4E); // write scratchpad. MUST be followed by 3 bytes as per datasheet.
      ds->write(buffer0[2]);
      ds->write(buffer0[3]);
      ds->write(0xFF);
      ds->reset();
      if (!read_scratchpad(addr, buffer1)) read_scratchpad(addr, buffer1);
      Comm.print(F("  Scratchpad config register <byte 4> after writing 0xFF: 0x"));
      print_hex(buffer1[4]);
      if (buffer1[4] != 0x7F) {
        // catches Family F
        fake_flags += 1;
        Comm.println(F(" unexpected (expected 7F). Error."));
      } else {
        Comm.println(F(" ok."));
      }
    }

    // set the resolution to 10 bit and modify alarm registers    
    ds->reset();
    ds->select(addr);
    ds->write(0x4E); // write scratchpad. MUST be followed by 3 bytes as per datasheet.
    ds->write(buffer0[2] ^ 0xff);
    ds->write(buffer0[3] ^ 0xff);
    ds->write(0x3F);
    ds->reset();

    if (!read_scratchpad(addr, buffer1)) read_scratchpad(addr, buffer1);
    
    Comm.print(F("  0x4E modifies alarm registers:"));
    if ((buffer1[2] != (buffer0[2] ^ 0xff)) || (buffer1[3] != (buffer0[3] ^ 0xff))) {
      fake_flags += 1;
      Comm.print(F(" cannot modify content as expected (want: "));
      print_hex(buffer0[2] ^ 0xff);
      Comm.write('/');
      print_hex(buffer0[3] ^ 0xff);
      Comm.print(F(", got: "));
      print_array(buffer1+2, 2, '/');
      Comm.println(F("). Error."));      
    } else
      Comm.println(F(" ok."));

    Comm.print(F("  0x4E accepts 10 bit resolution:"));
    if (buffer1[4] != 0x3f) {
      // catches Families C, F
      fake_flags += 1;
      Comm.print(F(" rejected (expected: 0x3F, got: 0x"));
      print_hex(buffer1[4]);
      Comm.println(F("). Error."));
    } else
      Comm.println(F(" ok."));

    Comm.print(F("  0x4E preserves reserved bytes:"));
    if ((buffer1[5] != buffer0[5]) || (buffer1[6] != buffer0[6]) || (buffer1[7] != buffer0[7])) {
      // this may catch Family B1, B2, D1, D2 (but not B1v2)
      fake_flags += 1;
      Comm.print(F(" no, got: "));
      print_array(buffer1+5, 3, '/');
      Comm.println(F(". Error."));
    } else
      Comm.println(F(" ok."));    

    // set the resolution to 12 bit
    ds->reset();
    ds->select(addr);
    ds->write(0x4E); // write scratchpad. MUST be followed by 3 bytes as per datasheet.
    ds->write(buffer0[2]);
    ds->write(buffer0[3]);
    ds->write(0x7f);
    ds->reset();

    if (!read_scratchpad(addr, buffer2)) read_scratchpad(addr, buffer2);
    
    Comm.print(F("  0x4E accepts 12 bit resolution:"));
    if (buffer2[4] != 0x7f) {
      // no sensor should fail this
      fake_flags += 1;
      Comm.print(F(" rejected (expected: 0x7F, got: 0x"));
      print_hex(buffer2[4]);
      Comm.println(F("). Error."));
    } else
      Comm.println(F(" ok."));

    Comm.print(F("  0x4E preserves reserved bytes:"));
    if ((buffer2[5] != buffer1[5]) || (buffer2[6] != buffer1[6]) || (buffer2[7] != buffer1[7])) {
      // again, Familes B1 and B2 would fail this
      fake_flags += 1;
      Comm.print(F(" no, got: "));
      print_array(buffer2+5, 3, '/');
      Comm.println(F(". Error."));
    } else
      Comm.println(F(" ok."));

    Comm.print(F("  Checking <byte 6> upon temperature change:"));
    {
      // We'll do a few temperature conversions in a row.
      // Usually, the temperature rises slightly if we do back-to-back
      //   conversions, so we can check <byte 6> at different temperatures.
      int count = 5;
      do {
        count -- ;
        if (count < 0)
          break;
        // perform temperature conversion
        ds->reset();
        ds->select(addr);
        ds->write(0x44, 1); // keep line high in case of parasite power
        delay(ms750);
        
        if (!read_scratchpad(addr, buffer3)) read_scratchpad(addr, buffer3);
        
      } while ( ((buffer3[0] == 0x50) && (buffer3[1] == 0x05)) || ((buffer3[0] == 0xff) && (buffer3[1] == 0x07)) ||
                ((buffer3[6] == 0x0c) && (((buffer3[0] + buffer3[6]) & 0x0f) == 0x00)) );
      if (count < 0) {
        Comm.println(F(" Inconclusive. Please change sensor temperature and repeat."));
        t_ok = false;
      } else {
        t_ok = true;
        const uint8_t is_value = buffer3[6];
        const uint8_t expect_value = 0x10 - (buffer3[0] & 0x0F);
        if ((buffer3[6] != 0x0c) && (is_value == expect_value)) {
          Comm.println(F(" ok."));
        } else {
          fake_flags += 1;
          Comm.print(F(" <byte 0> = 0x"));
          print_hex(buffer3[0]);
          Comm.print(F(" but <byte 6> = 0x"));
          print_hex(buffer3[6]);
          Comm.print(F(" rather than 0x"));
          print_hex(expect_value);
          Comm.println(F(". Error."));
        }
      }
      {
        Comm.print(F("    Info only: Temperature reading is "));
        float T_C = (int16_t)((uint16_t)buffer3[0] + 256*(uint16_t)buffer3[1]) / 16.0f;
        Comm.print(T_C);
        Comm.println(F(" oC."));
      }
    }

    Comm.print(F("  Testing if end of temperature conversion can be polled: "));
    bool reports_conversion_progress = true;
    {
      const int short_ms = 3;
      // test if sensor reports conversion completion
      //   test if sensor returns LOW after 3 ms, and HIGH after 750 ms.
      // perform temperature conversion
      ds->reset();
      ds->select(addr);
      ds->write(0x44);
      // we assume that we won't see a sensor that completes conversion in less than short_ms.
      delay(short_ms);
      const uint8_t after_short = ds->read_bit();
      delay(ms750 - short_ms);
      const uint8_t after_750ms = ds->read_bit();
      if ((after_short == 0) && (after_750ms != 0)) {
        Comm.println(F("ok."));
      } else {
        // this catches Family F
        fake_flags += 1;
        Comm.print(F("\n      "));
        Comm.print(F("Read "));
        Comm.print(after_short ? F("HIGH ") : F("LOW "));
        Comm.print(F("after "));
        Comm.print(short_ms, DEC);
        Comm.print(F(" ms (expected LOW), "));
        Comm.print(F("read "));
        Comm.print(after_750ms ? F("HIGH ") : F("LOW "));
        Comm.print(F("after "));
        Comm.print(ms750, DEC);
        Comm.println(F(" ms (expected HIGH). Error."));
        reports_conversion_progress = false;
      }
    }

    Comm.print(F("    Polling status valid immediately: "));
    if (reports_conversion_progress) {
      // test if sensor reports conversion completion without delay
      const int n_samples = 4;
      uint8_t result[n_samples];
      uint8_t ok_count = 0;
      for (int i=0; i<n_samples; i++) {
        ds->reset();
        ds->select(addr);
        ds->write(0x44);
        result[i] = ds->read_bit();
        if (result[i] == 0)
          ok_count ++;
        delay(ms750); // let conversion finish
      }
      if (ok_count == sizeof result) {
        Comm.println(F("ok."));
      } else {
        // catches Family A3
        fake_flags += 1;
        Comm.print(F("Sensor failed to signal immediately "));
        Comm.print(sizeof result - ok_count, DEC);
        Comm.print(F(" out of "));
        Comm.print(sizeof result, DEC);
        Comm.println(F(" times. Error."));
      }
    } else {
      fake_flags ++;
      Comm.println(F("test not possible. Error."));
    }

    Comm.print(F("    Conversion speed at different resolutions: "));
    if (reports_conversion_progress) {
      uint8_t bit_set_ok = 0;
      uint32_t conv_time[4];
      for (int idx = 0; idx < 4; idx++) {
        const int timeout_ms = 3000;
        ds->reset();
        ds->select(addr);
        ds->write(0x4E);
        ds->write(buffer0[2]);
        ds->write(buffer0[3]);
        ds->write(idx * 32 + 0x1f);
        delay(1);
        // check for set resolution
        if (!read_scratchpad(addr, buffer2)) read_scratchpad(addr, buffer2);
        const int true_idx = buffer2[4] / 32;
        if (true_idx == idx)
          bit_set_ok += 1;
        Comm.print(true_idx+9, DEC);
        Comm.print(F("-bit: "));

        ds->reset();
        ds->select(addr);
        ds->write(0x44); // start conversion

        uint32_t start = millis();
        delay(1); // wait a bit in case the sensor is just slow to pull down the line (e.g. Family A3)
        while ((ds->read_bit() == 0) && (millis()-start < timeout_ms)); // wait for "done" signal
        conv_time[idx] = millis() - start;
        if (conv_time[idx] < 3000) {
          Comm.print(conv_time[idx], DEC);
          Comm.print(F(" ms"));
        } else {
          Comm.print(F("(timeout)"));
        }
        if (idx != 3)
          Comm.print(F(", "));
        // one could check if the returned temperature is actually of reduced discretization
      }
      Comm.print(F(".\n      "));

      bool div_resolution = true;
      if (bit_set_ok != 4) {
        // catches Families C, F
        fake_flags ++;
        Comm.print(F("Could not change resolution. Error.\n      "));
        div_resolution = false;
      } else {
        Comm.print(F("Changing resolution possible. Ok.\n      "));
      }
      if (div_resolution) {
        float r1 = (float)conv_time[1] / (float)conv_time[0];
        float r2 = (float)conv_time[2] / (float)conv_time[1];
        float r3 = (float)conv_time[3] / (float)conv_time[2];
        float r_avg = (r1+r2+r3)/3.0f;
        if (fabs(r_avg-2.0f) < 0.3f) {
          Comm.print(F("Conversion time doubles with each bit in resolution. Ok.\n      "));
        } else if (fabs(r_avg-1.41f) < 0.2f) { // test for sqrt(2)
          // catches Family H
          fake_flags ++;
          Comm.print(F("Conversion time increases by factor 1.41 with each bit in resolution. Error.\n      "));
        } else if (fabs(r_avg-1.0f) < 0.15f) {
          // catches Families C, D1, D2, E, F.
          fake_flags ++;
          Comm.print(F("Conversion time independent of resolution. Error.\n      "));
        } else {
          fake_flags ++;
          Comm.print(F("Conversion time does not scale as expected with resolution. Error.\n      "));
        }
      }
      if (conv_time[3] < 550) {
        // catches Families A2, C, D1, D2, E, G
        fake_flags ++;
        Comm.println(F("12-bit conversion faster than expected. Error."));
      } else if (conv_time[3] > 660) {
        // some sensors of Family B1, B1v2, and B2 may fail this test
        fake_flags ++;
        Comm.println(F("12-bit conversion slower than expected. Error."));
      } else {
        // NB: the time bounds are established empirically and are neither documented nor guaranteed.
        // note that we're not testing whether this actually was 12 bit
        Comm.println(F("12-bit conversion time as expected. Ok."));
      }
    } else {
      fake_flags ++;
      Comm.println(F("test not possible. Error."));
    }

    Comm.print(F("  --> "));
    if (!t_ok) {
      Comm.print(F("Temperature test not completed, otherwise sensor"));
    } else 
      Comm.print(F("Sensor"));
      
    if (fake_flags == 0) {
      Comm.println(F(" responded like a genuine DS18B20."));
      Comm.println(F("  Not tested: EEPROM, Parasite Power, and undocumented commands."));
    } else {
      Comm.print(F(" is not a genuine DS18B20 based on "));
      Comm.print(fake_flags, DEC);
      Comm.println(F(" deviations."));
    }
    Comm.println();
  } // done with all sensors
  Comm.println(F("------------------------------------------------"));
}

// ****************************************************************************

int16_t time_conversion(uint8_t *addr) {
  const uint32_t timeout_ms = 3000;
  ds->reset();
  ds->select(addr);
  ds->write(0x44); // start conversion

  uint32_t start = millis();
  delay(1); // wait a bit in case the sensor is just slow to pull down the line (e.g. Family A3)
  while ((ds->read_bit() == 0) && (millis()-start < timeout_ms)); // wait for "done" signal
  uint32_t conv_time = millis() - start;
  if (conv_time >= timeout_ms) return -1;
  return conv_time;
}

void send_reset(uint8_t *addr) {
  ds->reset();
  ds->select(addr);
  ds->write(0x64);
  delay(10);
}

uint8_t one_byte_return(uint8_t *addr, uint8_t cmd) {
  ds->reset();
  ds->select(addr);
  ds->write(cmd);
  return ds->read();
}

void n_byte_return(uint8_t *addr, uint8_t cmd, int count, uint8_t *buffer) {
  ds->reset();
  ds->select(addr);
  ds->write(cmd);
  for (int i=0; i<count; i++)
    buffer[i] = ds->read();
}

uint8_t bit_invert(uint8_t a) {
  uint8_t b = 0;
  int i;
  for (i=0; i<8; i++) {
    b *= 2;
    b += (a & 0x01);
    a /= 2;
  }
  return b;
}

void param2trim(uint16_t offset_param_11bit, uint8_t curve_param_5bit, uint8_t *param1, uint8_t *param2) {
  *param1 = bit_invert(offset_param_11bit & 0x0ff);
  *param2 = curve_param_5bit * 8 + offset_param_11bit / 256;  
}

void trim2param(uint8_t param1, uint8_t param2, uint16_t *offset_param_11bit, uint8_t *curve_param_5bit) {
  *offset_param_11bit = bit_invert(param1) + ((uint16_t)(param2 & 0x07)) * 256;
  *curve_param_5bit = param2 / 8;
}

void get_trim_A(uint8_t *addr, uint8_t *trim1, uint8_t *trim2) {
  ds->reset();
  if (addr) ds->select(addr);
  else ds->write(0xCC);
  ds->write(0x93);
  *trim1 = ds->read();
  ds->reset();
  if (addr) ds->select(addr);
  else ds->write(0xCC);
  ds->write(0x68);
  *trim2 = ds->read();  
}

void set_trim_A(uint8_t *addr, uint8_t trim1, uint8_t trim2) {
  ds->reset();
  ds->select(addr);
  ds->write(0x95);
  ds->write(trim1);

  ds->reset();
  ds->select(addr);
  ds->write(0x63);
  ds->write(trim2);
  // don't store permanently, don't call reset  
}

void get_trim_params_A(uint8_t *addr, uint16_t *offset_param_11bit, uint8_t *curve_param_5bit) {
  uint8_t trim1, trim2;
  get_trim_A(addr, &trim1, &trim2);
  trim2param(trim1, trim2, offset_param_11bit, curve_param_5bit);  
}

void set_trim_params_A(uint8_t *addr, uint16_t offset_param_11bit, uint8_t curve_param_5bit) {
  uint8_t trim1, trim2;
  param2trim(offset_param_11bit, curve_param_5bit, &trim1, &trim2);
  set_trim_A(addr, trim1, trim2);  
}

bool is_valid_A_scratchpad(uint8_t *buff) {
  if ((buff[4] != 0x7f) && (buff[4] != 0x5f) && (buff[4] != 0x3f) && (buff[4] != 0x1f)) return false;
  if ((buff[0] == 0x50) && (buff[1] == 0x05) && (buff[6] == 0x0C)) return true; // power-up
  if ((buff[0] == 0xff) && (buff[1] == 0x07) && (buff[6] == 0x0C)) return true; // unsuccessful conversion
  return buff[6] == (0x10 - (buff[0] & 0x0f));
}

bool is_all_00(uint8_t *buff, int N) {
  int i;
  for (i=0; i<N; i++)
    if (buff[i] != 0x00) return false;
  return true;
}

void trigger_convert(uint8_t *addr, uint8_t conf, uint16_t wait) {
  ds->reset();
  ds->select(addr);
  ds->write(0x4E);
  ds->write(0xaf);
  ds->write(0xfe);
  ds->write(conf);
    
  ds->reset();
  ds->select(addr);
  ds->write(0x44, 1); // start conversion and keep data line high in case we need parasite power
  delay(wait);
  ds->depower();
}

int offset_param_range(uint8_t *addr) {
  uint16_t offset0, offset1;
  uint8_t curve0, curve1;
  uint8_t buff[9];
  get_trim_params_A(addr, &offset0, &curve0);
  const uint16_t test_offset_0 = 0x300 + ((offset0 ^ 0x18) & 0xFF);
  const uint16_t test_offset_1 = test_offset_0 + 0x100;
  set_trim_params_A(addr, test_offset_0, curve0);
  get_trim_params_A(addr, &offset1, &curve1);
  if (offset1 != test_offset_0) return 0;
  trigger_convert(addr, 0x7f, ms750 + 50);
  if (!read_scratchpad(addr, buff)) read_scratchpad(addr, buff);
  int16_t temp_offset_0 = buff[0] + 256*buff[1];
  set_trim_params_A(addr, test_offset_1, curve0);
  trigger_convert(addr, 0x7f, ms750 + 50);
  if (!read_scratchpad(addr, buff)) read_scratchpad(addr, buff);
  set_trim_params_A(addr, offset0, curve0);
  int16_t temp_offset_1 = buff[0] + 256*buff[1];
  if (temp_offset_1-temp_offset_0 > 128) return 1;
  return 0;
}

int curve_param_prop(uint8_t *addr) {
  // check if the curve parameter is unsigned (1) signed (2), temperature range inconclusive (-1), 
  // doesn't seem to be A (-2), CRC error (-3), or doesn't seem to do anything (0)
  uint8_t buff[9];
  uint16_t off = 0x3fe; // approx. half way but use LSB != 0xFF.

  set_trim_params_A(addr, off, 0x0f);
  trigger_convert(addr, 0x7f, ms750 + 50);
  if (!read_scratchpad(addr, buff)) read_scratchpad(addr, buff);
  if (0 != OneWire::crc8(buff,9)) return -3;
  if (!is_valid_A_scratchpad(buff)) return -2;
  int16_t r0f = buff[0] + 256*buff[1];
  {
    // check if we can set and read trim parameters
    uint16_t o;
    uint8_t c;
    get_trim_params_A(addr, &o, &c);
    if ((o != off) || (c != 0x0f)) return 0;
  }
  
  set_trim_params_A(addr, off, 0x00);
  trigger_convert(addr, 0x7f, ms750 + 50);
  if (!read_scratchpad(addr, buff)) read_scratchpad(addr, buff);
  if (0 != OneWire::crc8(buff,9)) return -3;
  if (!is_valid_A_scratchpad(buff)) return -2;
  int16_t r00 = buff[0] + 256*buff[1];
  
  set_trim_params_A(addr, off, 0x1f);
  trigger_convert(addr, 0x7f, ms750 + 50);
  if (!read_scratchpad(addr, buff)) read_scratchpad(addr, buff);
  if (0 != OneWire::crc8(buff,9)) return -3;
  if (!is_valid_A_scratchpad(buff)) return -2;
  int16_t r1f = buff[0] + 256*buff[1];
    
  set_trim_params_A(addr, off, 0x10);
  trigger_convert(addr, 0x7f, ms750 + 50);
  if (!read_scratchpad(addr, buff)) read_scratchpad(addr, buff);
  if (0 != OneWire::crc8(buff,9)) return -3;
  if (!is_valid_A_scratchpad(buff)) return -2;
  int16_t r10 = buff[0] + 256*buff[1];
  
  int16_t mini = min(r00, min(r0f, min(r10, r1f)));  
  int16_t maxi = max(r00, max(r0f, max(r10, r1f)));
  bool is_signed = (r0f-r10 > r1f-r00);
  bool is_unsigned = (r0f-r10 < r1f-r00);

  if (is_signed && (maxi-mini > 20*16)) return 2; // A2
  if (is_unsigned && (maxi-mini > 1*16) && (maxi-mini < 6*16)) return 1; // A1

  return -1;
}  

void loop_classify() {
  // ROM address of current sensor
  uint8_t addr[8];  

  int sensor_count = 0;
  ds->reset_search();
  while (ds->search(addr)) {
    sensor_count ++;

    // dump ROM    
    print_array(addr, 8, '-');
    if (0 != OneWire::crc8(addr, 8)) {
      Comm.print(F(" (CRC Error)"));      
    }
    
    uint8_t normal_power;
    {
      ds->reset();
      ds->select(addr);
      ds->write(0xB4);
      normal_power = ds->read_bit();
      if (!normal_power) {
        Comm.print(F(" (Parasite Power Mode)"));
      }
    }

    Comm.print(F(":"));
    
    int identified = 0;

    { // test for family A
      uint8_t r68 = one_byte_return(addr, 0x68);
      uint8_t r93 = one_byte_return(addr, 0x93);
      if (r68 != 0xff) {
        int opr = offset_param_range(addr);
        int cpp = curve_param_prop(addr);
        if ((cpp == 1) && (opr)) {
          Comm.print(F(" Family A1 (Genuine)."));
          identified++;
        }
        else if (cpp == 2) {
          Comm.print(F(" Family A2 (Clone).")); // signed and 32 oC range
          identified++;
        }
        else if ((cpp == 1) && (!opr)) {
          // Family A3 implements Trim1 and Trim2. Curve parameter is correct, but offset parameter is not.
          // Family A3 is also late to indicate ongoing temperature conversion.
          ds->reset();
          ds->select(addr);
          ds->write(0xB4);
          uint8_t normal_power = ds->read_bit();
          if (normal_power) {
            // if we are not in Parasite Power Mode then we can test for
            // delayed reporting
            int ok_count = 0;
            const int n_rounds = 4;
            for (int i=0; i<n_rounds; i++) {
              ds->reset();
              ds->select(addr);
              ds->write(0x44);
              uint8_t result = ds->read_bit();
              if (result == 0)
                ok_count ++;
              delay(ms750);
            }
            if (ok_count != n_rounds) {
              Comm.print(F(" Family A3 (Clone)."));
            } else
              Comm.print(F(" Family A3 variant (Clone). Please report."));
          } else {
            Comm.print(F(" Family A3 (Clone)."));
          }
          identified++;
        }
        else if (cpp == -3) Comm.print(F(" (Error reading scratchpad register [A].)"));
        else if (cpp == -1) {
          Comm.print(F(" Family A, unknown subtype (0x93="));
          print_hex(r93);
          Comm.print(F(", 0x68="));
          print_hex(r68);
          Comm.print(F("). Please report."));
          identified++;
        }
        // cpp==0 or cpp==-2: these aren't Family A as we know them. So, assume sensor is something else.
      }
      send_reset(addr);
    }

    { // test for family B
      // the difference between the original B1 and version B1v2 is in
      // the behavior of the scratchpad register.
      uint8_t r97 = one_byte_return(addr, 0x97);
      if (r97 != 0xFF) {
        uint8_t new6, new7;
        bool version2 = true;
        // check if we can store custom bytes in scratchpad
        uint8_t buffer0[9];
        uint8_t buffer1[9];
        if (!read_scratchpad(addr, buffer0)) read_scratchpad(addr, buffer0);
        new6 = buffer0[6] ^ 0xFF;
        new7 = buffer0[7] ^ 0xFF;
        ds->reset();
        ds->select(addr);
        ds->write(0x4E); // write scratchpad. MUST be followed by 3 bytes as per datasheet.
        ds->write(buffer0[2]);
        ds->write(buffer0[3]);
        ds->write(0x7F);
        ds->write(new6);
        ds->write(new7);
        ds->reset();
        if (!read_scratchpad(addr, buffer1)) read_scratchpad(addr, buffer1);
        if ((buffer1[6] == new6) || (buffer1[7] == new7))
          version2 = false; // sensor shows original behavior of 2019

        if (r97 == 0x22) Comm.print(F(" Family B1"));
        else if (r97 == 0x31) Comm.print(F(" Family B2"));
        else Comm.print(F(" Family B")); 
        if (version2)
          Comm.print(F("v2")); 
        Comm.print(F(" (Clone)"));
        if ((r97 != 0x22) && (r97 != 0x31)) {
          Comm.print(F(", unknown subtype (0x97="));
          print_hex(r97);
          Comm.print(F(")"));
        }
        Comm.print(F("."));
        identified++;
      }
    }
    
    {
      // Family C
      // Don't have a test for response to undocumented codes, so check if
      // config register is constant, which is a unique property (as Family F allows setting the MSB)
      uint8_t buff[9];
      uint8_t cfg1, cfg2;
      ds->reset();
      ds->select(addr);
      ds->write(0x4E);
      ds->write(0xaa);
      ds->write(0x55);
      ds->write(0x00);
      if (!read_scratchpad(addr, buff)) read_scratchpad(addr, buff);
      if (is_all_00(buff, 9) || (0 != OneWire::crc8(buff,9))) goto err_C;
      cfg1 = buff[4];
      ds->reset();
      ds->select(addr);
      ds->write(0x4E);
      ds->write(0xaa);
      ds->write(0x55);
      ds->write(0xff);
      if (!read_scratchpad(addr, buff)) read_scratchpad(addr, buff);
      if (is_all_00(buff, 9) || (0 != OneWire::crc8(buff,9))) goto err_C;
      cfg2 = buff[4];
      ds->reset();
      ds->select(addr);
      ds->write(0x64);
      delay(10);
      if (cfg1 == cfg2) {
        Comm.print(F(" Family C (Clone)."));
        identified++;
      }
      if (0) {
err_C:
        Comm.print(F(" (Error reading scratchpad register [C].)"));
      }
    }

    { // test for family D
      uint8_t r8B = one_byte_return(addr, 0x8B);
      // mention if parasite power is known not to work.
      if (r8B == 0x06) Comm.print(F(" Family D1 (Clone w/o parasite power mode)."));
      else if (r8B == 0x02) Comm.print(F(" Family D1 (Clone)."));
      else if (r8B == 0x00) Comm.print(F(" Family D2 (Clone w/o parasite power mode)."));
      else if (r8B != 0xFF) {
        Comm.print(F(" Family D (Clone), unknown subtype (0x8B="));
        print_hex(r8B);
        Comm.print(F(")."));
      }
      if (r8B != 0xff) identified++;
    }

    { // test for family E:
      // testing for the existence of a 2-byte scratchpad register by reading and writing to it.
      uint8_t sp21, sp22;
      uint8_t sp21alt, sp22alt;
      uint8_t sp21b, sp22b;
      ds->reset();
      ds->select(addr);
      ds->write(0xDE);
      sp21 = ds->read();
      sp22 = ds->read();
      ds->reset();
      ds->select(addr);
      ds->write(0x2E);
      sp21alt = ~sp21;
      sp22alt = ~sp22;
      ds->write(sp21alt);
      ds->write(sp22alt);
      ds->reset();
      ds->select(addr);
      ds->write(0xDE);
      sp21b = ds->read();
      sp22b = ds->read();
      if ((sp21b == sp21alt) && (sp22b == sp22alt)) {
        Comm.print(F(" Family E (Clone)."));
        identified++;
      }
    }

    {
      // Family F returns 3 bytes on 0x19, does not implement EEPROM,
      //  allows to set MSB in Config byte, and has <byte 6> fixed at 0x0C.
      uint8_t buffer[3];
      n_byte_return(addr, 0x19, 3, buffer);
      if ((buffer[0] != 0xFF) || (buffer[1] != 0xFF) || (buffer[2] != 0xFF)) {
        Comm.print(F(" Family F (Clone)."));
        identified++;
      }
    }

    {
      // Family G responds to 0x8E and has peculiar bug in <byte 6>
      uint8_t r8E = one_byte_return(addr, 0x8E);
      if (r8E != 0xFF) {
        uint8_t buffer[9];
        ds->reset();
        ds->select(addr);
        ds->write(0x44, 1); // start conversion
        delay(ms750 + 50);
        if (!read_scratchpad(addr, buffer)) read_scratchpad(addr, buffer);
        if (buffer[6] == 0x20 - (buffer[0] & 0x0f)) {
          // this looks like an implementation mistake
          Comm.print(F(" Family G (Clone)."));
          identified++;
        } else {
          // let's assume these guys correct their mistake some time in the
          //   future
          Comm.print(F(" Possibly Family G variant (Clone). Please report."));
          identified++;
        }
      }
    }

    {
      // Family H scales conversion times by sqrt(2) rather than 2 (or 1)
      // responds with 0xFF to all undocumented function codes
      // set to 9 bit, convert
      // This test does not work in parasite power mode.
      uint8_t buffer0[9];
      uint8_t buffer1[9];
      int idx, true_idx;
      if (!read_scratchpad(addr, buffer0)) read_scratchpad(addr, buffer0);

      for (int run=0; run<1; run++) { // dummy loop to allow exit of test

        // set to 9 bit, convert
        ds->reset();
        ds->select(addr);
        ds->write(0x4E);
        ds->write(buffer0[2]);
        ds->write(buffer0[3]);
        idx = 0;
        ds->write(idx * 32 + 0x1f);
        delay(1);
        // check for set resolution
        if (!read_scratchpad(addr, buffer1)) read_scratchpad(addr, buffer1);
        true_idx = buffer1[4] / 32;
        if (true_idx != idx) break;

        float time_9bit_ms = max(1.0f, (float)time_conversion(addr));
        // set to 10 bit, convert
        ds->reset();
        ds->select(addr);
        ds->write(0x4E);
        ds->write(buffer0[2]);
        ds->write(buffer0[3]);
        idx = 1;
        ds->write(idx * 32 + 0x1f);
        delay(1);
        // check for set resolution
        if (!read_scratchpad(addr, buffer1)) read_scratchpad(addr, buffer1);
        true_idx = buffer1[4] / 32;
        if (true_idx != idx) break;

        float time_10bit_ms = max(1.0f, (float)time_conversion(addr));
        // evaluate
        float ratio = time_10bit_ms / time_9bit_ms;
        if (fabs(ratio-1.41f) <= 0.2f) {
          Comm.print(F(" Family H (Clone)."));
          identified++;
        } else if ((fabs(ratio-2.0f) <= 0.2f) || (fabs(ratio-1.0f) <= 0.2f)) {
          // known ratios seen in other Families
        } else {
          // not seen as of 2024
          identified++;
          Comm.print(F(" Possibly Family H variant (Clone). Please report."));
        }
      }
      // (re-)set to 12 bit.
      ds->reset();
      ds->select(addr);
      ds->write(0x4E);
      ds->write(buffer0[2]);
      ds->write(buffer0[3]);
      idx = 3;
      ds->write(idx * 32 + 0x1f);
    }

    if (identified == 0)
      Comm.print(F(" (Could not identify Family.)"));

    if (identified > 1)
      Comm.print(F(" (Part may belong to a Family not seen in 2024.)"));
    
    if (identified != 1) {
      if (normal_power)
        Comm.print(F("  Please report."));
      else {
        if (identified == 0)
          Comm.print(F("  Might be Family H: please repeat test not in parasite power mode."));
        else
          Comm.print(F("  Please repeat test not in parasite power mode."));
      }
    }
    Comm.println();
    
  } // end iterate over all sensors

  if (sensor_count == 0) {
    // produce output so the user knows we tried (and failed)
    // to detect a sensor. May have forgotten the pull-up
    // resistor.
    Comm.println(F("No sensors detected."));
  }
  
  Comm.println(F("-------------")); // indicate end
}
