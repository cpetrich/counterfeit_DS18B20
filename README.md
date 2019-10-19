# Your DS18B20 temperature sensor is likely a fake, counterfeit, clone...
...that is, unless you bought the chips directly from [Maxim Integrated](https://www.maximintegrated.com/en/products/sensors/DS18B20.html) (or Dallas Semiconductor in the old days) or an authorized distributor (DigiKey, RS, Farnell, Mouser, Conrad, etc.), or you took exceptionally good care purchasing waterproofed DS18B20 probes. We bought over 500 "waterproof" probes from two dozen sellers on ebay. All of them contained counterfeit DS18B20 sensors. Also, all sensors we bought on ebay were counterfeit.

> Author: Chris Petrich, October 2019.
> License: CC BY.

## Why should I care?
Besides ethical concerns, some of the counterfeit sensors actually do not contain an EEPROM, do not work in parasitic power mode, have a high noise level or temperature offset outside the advertised ±0.5 °C band, have bugs and unspecified failure rates, or differ in another unknown manner from the specifications in the Maxim datasheet. Clearly, the problems are not big enough to discourage people from buying probes on ebay, but it may be good to know the actual specs when the data are important or measurement conditions are difficult.

## Are they clones?
IMHO, they are not clones, they are counterfeits (fakes). They are not clones because, as of writing (2019), all counterfeits behave differently electrically from the authentic Maxim products and can be distinguished easily from the originals. The manufacturers of the counterfeits have not attempted to disguise their counterfeit nature electrically, and even use topmarks with production date--batch code combinations different from the ones used by Maxim. However, I consider them counterfeits because their topmarks wrongly implies they were produced by ``Dallas`` (i.e., a shorthand for a company bought by Maxim).

## How do I know if I am affected?
These are the simplest tests that I can think of:
* There are at least 3 simple tests that can be done by sending one-wire commands to the sensor:
	1. It is a fake if its ROM address does not follow the pattern 28-xx-xx-xx-xx-00-00-xx.
	2. It is a fake if ``<byte 6> == 0`` or ``<byte 6> > 0x10`` in the scratchpad register, or if the following scratchpad register relationship applies after **any** *successful* temperature conversion: ``(<byte 0> + <byte 6>) & 0x0f != 0`` (*12-bit mode*).
	3. It is a fake if the chip returns data to simple queries of undocumented function codes other than 0x68 and 0x93. (*As of writing (2019), this can actually be simplified to: it is a fake if the return value to sending code 0x68 is ``0xff``.*)
* It is a fake if the date--batch combination printed on the case of the sensor is not in the Maxim database (need to ask Maxim tech support to find out).

Note that none of the points above give certainty that a particular DS18B20 is an authentic Maxim product, but if any of the tests above indicate "fake" then it is most defintely counterfeit. Based on my experience, a sensor that will fail any of the three software tests will fail all three of the software tests.

## What families of DS18B20-like chips can I expect to encounter?
Besides the DS18B20 originally produced by Dallas Semiconductor and continued by Maxim Integrated after they purchased Dallas (Family A, below), similar circuits seem to be produced independently by at least 4 other companies (Families B, C, D, and E).

In our ebay purchases in 2018/19 of waterproof DS18B20 probes from China, Germany, and the UK, most lots had sensors of Family B1 (i.e., seems ok at first glance, but this is not an endorsement), while one in three purchases had sensors of Family D (i.e., garbage for our purposes). None had sensors of Family A. Neither origin nor price were indicators of sensor Family.

In the ROM patterns below, *tt* and *ss* stand for fast-changing and slow-changing values within a production run, and *crc* is the CRC8 checksum defined in the datasheet.

### Family A: Authentic Maxim DS18B20
* ROM pattern: 28-tt-tt-ss-ss-00-00-crc
* Scratchpad register:  ``(<byte 0> + <byte 6>) & 0x0f == 0`` after all successful temperature conversions, and ``0x00 < <byte 6> <= 0x10``.
* Returns "Trim1" and "Trim2" values if queried with function codes 0x93 and 0x68, respectively. The bit patterns are very similar to each other within a production run, and Trim2 is unlikely to equal 0xff.
* Temperature offset of current batches is as shown on the [Maxim FAQ](https://www.maximintegrated.com/en/support/faqs/ds18b20-faq.html) page, i.e. approx. +0.1 °C at 0 °C (*i.e., not as shown on the datasheet. The plot on the datasheet stems from production runs at the time of introduction of the sensor 10+ years ago.*). Very little if any temperature discretization noise.
* Polling after function code 0x44 indicates approx. 600 ms for a 12-bit temperature conversion.

- Example ROM: 28-13-9B-BB-0B **-00-00-** 1F
- Initial Scratchpad: **50**/**05**/4B/46/**7F**/**FF**/0C/**10**/1C
- Example topmark: DALLAS DS18B20 1932C4 +786AB

### Family B1: Matches Datasheet Temperature Offset Curve
* ROM patterns:
	- 28-AA-tt-ss-ss-ss-ss-crc
	- 28-tt-tt-ss-ss-ss-ss-crc
* Scratchpad register ``<byte 6>`` is constant (default ``0x0c``).
* Write scratchpad-bug (0x4E):
	- If 3 data bytes are sent (TH, TL, Config) then ``<byte 6>`` changes to ``0x7f``,
	- 5 data bytes should be sent with function code 0x4E instead where the last two bytes overwrite ``<byte 6>`` and ``<byte 7>``, respectively.
* Does not return data on undocumented function code 0x68. Does return data from codes 0x90, 0x91, 0x92, 0x93, 0x95, and 0x97.
* ROM code can be changed in software with command sequence "96-Cx-Dx-94".
* Temperature offset as shown on the datasheet (-0.15 °C at 0 °C). Very little if any temperature discretization noise.
* Polling after function code 0x44 indicates approx. 650-700 ms for a 12-bit temperature conversion.

- Example ROM: 28 **-AA-** 3C-61-55-14-01-F0
- Example ROM: 28-AB-9C-B1 **-33-14-01-** 81
- Initial Scratchpad: 50/05/4B/46/7F/FF/0C/10/1C
- Example topmark: DALLAS DS18B20 1626C4 +233AA

### Family B2: -0.5 °C Temperature Offset at 0 °C
* ROM patterns: 28-FF-tt-ss-ss-ss-ss-crc
* Scratchpad register ``<byte 6>`` is constant (default ``0x0c``).
* Write scratchpad-bug (0x4E):
	- If 3 data bytes are sent (TH, TL, Config) then ``<byte 6>`` changes to ``0x7f``,
	- 5 data bytes should be sent with function code 0x4E instead where the last two bytes overwrite ``<byte 6>`` and ``<byte 7>``, respectively.
* Does not return data on undocumented function code 0x68. Does return data from codes 0x90, 0x91, 0x92, 0x93, 0x95, and 0x97.
* ROM code can **not** be changed in software with command sequence "96-Cx-Dx-94".
* Typical temperature offset at at 0 °C is -0.5 °C. Very little if any temperature discretization noise.
* Polling after function code 0x44 indicates approx. 650-700 ms for a 12-bit temperature conversion.

- Example ROM: 28 **-FF-** 7C-5A-61-16-04-EE
- Initial Scratchpad: 50/05/4B/46/7F/FF/0C/10/1C
- Example topmark: DALLAS DS18B20 1626C4 +233AA

### Family C: Small Offset at 0 °C
* ROM patterns: 28-ss-64-ss-ss-tt-tt-crc
* Scratchpad register ``<byte 6> == 0x0c``.
* Does not return data on undocumented function code 0x68 or any other undocumented function code.
* Typical temperature offset at 0 °C is +0.05 °C. Very little if any temperature discretization noise.
* EEPROM endures only about eight (8) write cycles (function code 0x48).
* Polling after function code 0x44 indicates 30 ms (thirty) for a 12-bit temperature conversion.

- Example ROM: 28 **-FF-64-** 1D-CD-96-F2-01
- Initial Scratchpad: 50/05/55/00/7F/FF/0C/10/21
- Example topmark: DALLAS DS18B20 1810C4 +158AC

### Family D: Noisy Rubbish
* ROM patterns: 28-tt-tt-ss-ss-ss-ss-crc
* Scratchpad register ``<byte 7> == 0x66``, ``<byte 6> == 0x81`` or ``<byte 6> == 0xA5``, ``<byte 5> != 0xff``.
* Does not return data on undocumented function code 0x68. Responds back with data after codes 0x8B, 0xBA, 0xBB, 0xDD, 0xEE, or a subset of those.
* A (small?) subset of chips in this family contains a supercap rather than a proper EEPROM. Those chips retain the last temperature measurement between power cycles.
* Temperature errors up to 3 °C at 0 °C. Depending on batch, either noisy data or very noisy data.
* Sensors **do not work with Parasitic Power**
* Polling after function code 0x44 indicates approx. 500-550 ms for a 12-bit temperature conversion.

- Example ROM: 28-1C-BC **-46-92-** 10-02-88
- Example ROM: 28-24-1D **-77-91-** 04-02-CE
- Example ROM: 28-90-FE **-79-97-** 00-03-20
- Example ROM: 28-FD-58 **-94-97-** 14-03-05
- Initial Scratchpad: 90/01/55/05/7F/xx/xx/66/xx
- Example topmark: DALLAS DS18B20 1827C4 +051AG

### Family E: Incomplete Work
* ROM patterns: 28-tt-tt-ss-ss-00-80-crc
* Scratchpad register ``<byte 7> == 0xff``, ``<byte 6> == 0xff``.
* Contains no EEPROM.

- Example ROM: 28-9E-9C-1F **-00-00-80-** 04
- Initial Scratchpad: xx/xx/FF/FF/7F/FF/FF/FF/xx


(*Information on chips of Families A, B, C, and D comes from my own investigations of sensors in conjunction with the references below. Tests were performed at 5 V with 1.2 kOhm pull-up. Information on chips of Family E comes from web searches.*)

## References
* [DS18B20](https://datasheets.maximintegrated.com/en/ds/DS18B20.pdf) "DS18B20 Programmable Resolution 1-Wire Digital Thermometer", Datasheet, Maxim Integrated.
* [DS18S20](https://datasheets.maximintegrated.com/en/ds/DS18S20.pdf) "DS18S20 High-Precision 1-Wire Digital Thermometer", Datasheet, Maxim Integrated.
* [AN4377](https://www.maximintegrated.com/en/design/technical-documents/app-notes/4/4377.html) "Comparison of the DS18B20 and DS18S20 1-Wire Digital Thermometers", Maxim Integrated
* AN247 "DS18x20 EEPROM Corruption Issue", Maxim Integrated
