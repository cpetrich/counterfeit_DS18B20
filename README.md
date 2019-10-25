# Your DS18B20 temperature sensor is likely a fake, counterfeit, clone...
...unless you bought the chips directly from [Maxim Integrated](https://www.maximintegrated.com/en/products/sensors/DS18B20.html) (or Dallas Semiconductor in the old days) or an authorized distributor (DigiKey, RS, Farnell, Mouser, Conrad, etc.), or you took exceptionally good care purchasing waterproofed DS18B20 probes. We bought over 500 "waterproof" probes from two dozen sellers on ebay. All of them contained counterfeit DS18B20 sensors. Also, all but one sensor we bought on ebay were counterfeit.

> Author: Chris Petrich, 26 October 2019.
> License: CC BY.

## Why should I care?
Besides ethical concerns, some of the counterfeit sensors actually do not contain an EEPROM, do not work in parasitic power mode, have a high noise level or temperature offset outside the advertised ±0.5 °C band, have bugs and unspecified failure rates, or differ in another unknown manner from the specifications in the Maxim datasheet. Clearly, the problems are not big enough to discourage people from buying probes on ebay, but it may be good to know the actual specs when the data are important or measurement conditions are difficult.

## Are they clones?
IMHO, they are not clones, they are counterfeits (fakes). They are not clones because, as of writing (2019), all counterfeits behave differently electrically from the authentic Maxim products and can be distinguished easily from the originals. The manufacturers of the counterfeits have not attempted to disguise their counterfeit nature electrically, and even use topmarks with production date--batch code combinations different from the ones used by Maxim. However, I consider them counterfeits because their topmarks wrongly implies they were produced by ``Dallas`` (i.e., a shorthand for a company bought by Maxim).

## How do I know if I am affected?
If the DS18B20 have been bought from authorized dealers though a controlled supply chain then the chips are legit.

Otherwise, (I) one can test for compliance with the datasheet. If a sensor fails any of those tests, it is a fake (unless Maxim's implementation is buggy \[4\]). (II) one can compare sensor behavior with the behavior of Maxim-produced DS18B20. Those tests are based on the conjecture that all Maxim-produced DS18B20 behave alike. This should be the case at least for sensors that share a die code (which has been ``C4`` since at least some time in 2011 \[5\]) \[5\].

Regarding (I), discrepancy between what the datasheet says should happen and what the sensors do include \[1\]
* the content of the scratchpad register in Families D and E (see below): the temperature reading right after power-up is not 85 °C, and/or reserved bytes 5 and 7 of the scratchpad register are not ``0xff`` and ``0x10``, respectively
* the apparently very small number of EEPROM write cycles in Family C
* dysfunctional parasitic power mode in Family D
* temperature readings outside the specification, affecting mostly sensors of Family D \[5\]
* the missing EEPROM in Family E
However, heuristic methods (II) will have to be used to identify sensors of Family B as counterfeit.

Regarding (II), there are simple tests for differences with Maxim-produced DS18B20 sensors (``C4`` die) that apparently *all* counterfeit sensors fail \[5\]. The most straight-forward software tests are probably these:
1. It is a fake if its ROM address does not follow the pattern 28-xx-xx-xx-xx-00-00-xx \[5\]. (Maxim's ROM is essentially a mostly chronological serial number (mostly, but not strictly when compared with the date code on the case) \[5\].)
2. It is a fake if ``<byte 6> == 0`` or ``<byte 6> > 0x10`` in the scratchpad register, or if the following scratchpad register relationship applies after **any** *successful* temperature conversion: ``(<byte 0> + <byte 6>) & 0x0f != 0`` (*12-bit mode*) \[3,5\].
3. It is a fake if the chip returns data to simple queries of undocumented function codes other than 0x68 and 0x93 \[4,5\]. (*As of writing (2019), this can actually be simplified to: it is a fake if the return value to sending code 0x68 is ``0xff`` \[5\].*)

Alternatively,
* It is a fake if the date--batch combination printed on the case of the sensor is not in the Maxim database (need to ask Maxim tech support to find out).

Note that none of the points above give certainty that a particular DS18B20 is an authentic Maxim product, but if any of the tests above indicate "fake" then it is most defintely counterfeit \[5\]. Based on my experience, a sensor that will fail any of the three software tests will fail all of them.

## What families of DS18B20-like chips can I expect to encounter?
Besides the DS18B20 originally produced by Dallas Semiconductor and continued by Maxim Integrated after they purchased Dallas (Family A, below), similar circuits seem to be produced independently by at least 4 other companies (Families B, C, D, and E) \[5\]. The separation into families is based on patterns in undocumented function codes that the chips respond to as similarities at that level are unlikely to be coincidental \[5\].

In our ebay purchases in 2018/19 of waterproof DS18B20 probes from China, Germany, and the UK, most lots had sensors of Family B1 (i.e., seems ok at first glance, but this is not an endorsement), while one in three purchases had sensors of Family D (i.e., garbage for our purposes). None had sensors of Family A. Neither origin nor price were indicators of sensor Family.

In the ROM patterns below, *tt* and *ss* stand for fast-changing and slow-changing values within a production run \[5\], and *crc* is the CRC8 checksum defined in the datasheet \[1\].

### Family A: Authentic Maxim DS18B20
* ROM pattern \[5\]: 28-tt-tt-ss-ss-00-00-crc
* Scratchpad register:  ``(<byte 0> + <byte 6>) & 0x0f == 0`` after all successful temperature conversions, and ``0x00 < <byte 6> <= 0x10`` \[2,3,5\].
* Returns "Trim1" and "Trim2" values if queried with function codes 0x93 and 0x68, respectively \[4\]. The bit patterns are very similar to each other within a production run \[4\]. Trim2 is currently less likely to equal 0xff than Trim1 \[5\]. Trim2 was 0xDB or 0xDC around 2011 through 2013 or later, and has been 0x74 (possibly 0x73 occasionally) since at least 2017 (all with ``C4`` die) \[5\].
	+ Trim1 and Trim2 encode two parameters \[5\]. Let the bit pattern of Trim1 be ``[t17, t16, t15, t14, t13, t12, t11, t10]`` (MSB to LSB) and Trim2 be ``[t27, t26, t25, t24, t23, t22, t21, t20]``. Then,
		- offset parameter = ``[t22, t21, t20, t10, t11, t12, t13, t14, t15, t16, t17]`` (unsigned 11 bit-value) \[5\], and
		- curve parameter = ``[t27, t26, t25, t24, t23]`` (unsigned 5 bit-value) \[5\].
	+ Within a batch, the offset parameter seems to spread over 20 to 30 units while all sensors within the batch share the same curve parameter \[5\].
	+ The offset parameter shifts the temperature output over a range of approx. 100 °C (0.053 °C per unit), while the curve parameter adjusts the temperature sensitivity by approximately up to 1% (tentative estimate) \[5\]. Example values of 2019 are ``offset = 0x420`` and ``curve = 0xE``, i.e. they lie pretty central within their respective ranges.
* Temperature offset of current batches (2019) is as shown on the [Maxim FAQ](https://www.maximintegrated.com/en/support/faqs/ds18b20-faq.html) page, i.e. approx. +0.1 °C at 0 °C \[6\] (*i.e., not as shown on the datasheet \[1\]. The plot on the datasheet stems from production runs at the time of introduction of the sensor 10+ years ago \[5\].*). Very little if any temperature discretization noise \[5\].
* Polling after function code 0x44 indicates a spread of 590-610 ms between sensors for a 12-bit temperature conversion at room temperature \[5\]. Conversion time is easily repeatable for individual chips. Lower resolutions cut the time in proportion, i.e. 11 bit-conversions take half the time.
* It appears the chip returns a temperature of 127.94 °C (=0x07FF / 16.0) if a temperature conversion was unsuccessful \[5\] (e.g. due to power stability issues which arise reproducibly in "parasitic power" mode with *multiple* DS18B20 if Vcc is left floating rather than tied to ground. Note that the datasheet clearly states that Vcc is to be tied to GND in parasitic mode.).

- Example ROM: 28-13-9B-BB-0B **-00-00-** 1F
- Initial Scratchpad: **50**/**05**/4B/46/**7F**/**FF**/0C/**10**/1C
- Example topmark: DALLAS DS18B20 1932C4 +786AB

### Family B1: Matches Datasheet Temperature Offset Curve
* ROM patterns \[5\]:
	- 28-AA-tt-ss-ss-ss-ss-crc
	- 28-tt-tt-ss-ss-ss-ss-crc
* Scratchpad register ``<byte 6>`` is constant (default ``0x0c``) \[5\].
* Write scratchpad-bug (0x4E) \[5\]:
	- If 3 data bytes are sent (TH, TL, Config) then ``<byte 6>`` changes to the third byte sent,
	- 5 data bytes can be sent with function code 0x4E, where the last two bytes overwrite ``<byte 6>`` and ``<byte 7>``, respectively.
* Does not return data on undocumented function code 0x68 \[5\]. Does return data from codes 0x90, 0x91, 0x92, 0x93, 0x95, and 0x97 \[5\].
* ROM code can be changed in software with command sequence "96-Cx-Dx-94" \[5\].
* Temperature offset as shown on the datasheet (-0.15 °C at 0 °C) \[6\]. Very little if any temperature discretization noise \[5\].
* Polling after function code 0x44 indicates approx. 650-700 ms for a 12-bit temperature conversion \[5\].

- Example ROM: 28 **-AA-** 3C-61-55-14-01-F0
- Example ROM: 28-AB-9C-B1 **-33-14-01-** 81
- Initial Scratchpad: 50/05/4B/46/7F/FF/0C/10/1C
- Example topmark: DALLAS DS18B20 1626C4 +233AA

### Family B2: -0.5 °C Temperature Offset at 0 °C
* ROM patterns \[5\]: 28-FF-tt-ss-ss-ss-ss-crc
* Scratchpad register ``<byte 6>`` is constant (default ``0x0c``) \[5\].
* Write scratchpad-bug (0x4E) \[5\]:
	- If 3 data bytes are sent (TH, TL, Config) then ``<byte 6>`` changes to the third byte sent,
	- 5 data bytes can be sent with function code 0x4E, where the last two bytes overwrite ``<byte 6>`` and ``<byte 7>``, respectively.
``<byte 7>``, respectively.
* Does not return data on undocumented function code 0x68 \[5\]. Does return data from codes 0x90, 0x91, 0x92, 0x93, 0x95, and 0x97 \[5\].
* ROM code can **not** be changed in software with command sequence "96-Cx-Dx-94" \[5\].
* Typical temperature offset at at 0 °C is -0.5 °C \[6\]. Very little if any temperature discretization noise \[5\].
* Polling after function code 0x44 indicates approx. 650-700 ms for a 12-bit temperature conversion \[5\].

- Example ROM: 28 **-FF-** 7C-5A-61-16-04-EE
- Initial Scratchpad: 50/05/4B/46/7F/FF/0C/10/1C
- Example topmark: DALLAS DS18B20 1626C4 +233AA

### Family C: Small Offset at 0 °C
* ROM patterns \[5\]: 28-ss-64-ss-ss-tt-tt-crc
* Scratchpad register ``<byte 6> == 0x0c`` \[5\].
* Does not return data on undocumented function code 0x68 or any other undocumented function code \[5\].
* Typical temperature offset at 0 °C is +0.05 °C \[6\]. Very little if any temperature discretization noise \[5\].
* EEPROM endures only about eight (8) write cycles (function code 0x48) \[5\].
* Polling after function code 0x44 indicates 30 ms (thirty) for a 12-bit temperature conversion \[5\].
* Operates in 12-bit conversion mode, only (configuration byte is fixed at ``0x7f``) \[5\].
* Default alarm register settings differ from Family A \[5\].

- Example ROM: 28 **-FF-64-** 1D-CD-96-F2-01
- Initial Scratchpad: 50/05/55/00/7F/FF/0C/10/21
- Example topmark: DALLAS DS18B20 1810C4 +158AC

### Family D1: Noisy Rubbish
* ROM patterns \[5\]: 28-tt-tt-ss-ss-ss-ss-crc
* Scratchpad register ``<byte 7> == 0x66``, ``<byte 6> != 0x81`` and ``<byte 5> != 0xff`` \[5\].
* Does not return data on undocumented function code 0x68 \[5\]. Responds back with data or status information after codes 
	+ 0x4D, 0x8B, 0xBA, 0xBB, 0xDD, 0xEE \[5\], or
	+ 0x4D, 0x8B, 0xBA, 0xBB \[5\].
* Temperature errors up to 3 °C at 0 °C \[6\]. Depending on batch, either noisy data or very noisy data \[5\].
* Sensors **do not work with Parasitic Power** \[5\]
* Polling after function code 0x44 indicates approx. 500-550 ms for a 12-bit temperature conversion \[5\].
* Initial temperature reading is 25 °C \[5\]. Default alarm register settings differ from Family A \[5\].

- Example ROM: 28-1C-BC **-46-92-** 10-02-88
- Example ROM: 28-90-FE **-79-97-** 00-03-20
- Example ROM: 28-FD-58 **-94-97-** 14-03-05
- Initial Scratchpad: 90/01/55/05/7F/xx/xx/66/xx
- Example topmark: DALLAS DS18B20 1827C4 +051AG

### Family D2: Noisy Rubbish without EEPROM
* ROM patterns \[5\]: 28-tt-tt-77-91-ss-ss-crc
* Scratchpad register ``<byte 7> == 0x66``, ``<byte 6> == 0x81`` and ``<byte 5> == 0x7e`` \[5\].
* Does not return data on undocumented function code 0x68 \[5\]. Responds back with data or status information after codes 0x4D, 0x8B, 0xBA, 0xBB \[5\].
* It is possible to send arbitrary content for ROM code and for bytes 5, 6, and 7 of the status register after undocumented function codes 0xA3 and 0x66, respectively \[5\].
* Temperature errors up to 3 °C at 0 °C \[6\]. Very noisy data \[5\].
* Sensors **do not work with Parasitic Power** \[5\]
* Polling after function code 0x44 indicates approx. 11 ms (eleven) for a 12-bit temperature conversion \[5\].
* Chips contain a supercap rather than a proper EEPROM to hold alarm and configuration settings \[5\]. I.e., the last temperature measurement and updates to the alarm registers are retained between power cycles that are not too long \[5\].
	+ The supercap retains memory for several minutes unless Vcc is pulled to GND, in which case memory retention is 5 to 30 seconds \[5\].
* Initial temperature reading is 25 °C or the last reading before power-down \[5\]. Default alarm register settings differ from Family A \[5\].
	
- Example ROM: 28-24-1D **-77-91-** 04-02-CE
- Initial Scratchpad: 90/01/55/05/7F/7E/81/66/27
- Example topmark: DALLAS DS18B20 1827C4 +051AG

### Family E: Incomplete Work
* ROM patterns \[5,7\]: 28-tt-tt-ss-ss-00-80-crc
* Scratchpad register ``<byte 7> == 0xff``, ``<byte 6> == 0xff`` \[5,7\].
* Contains no EEPROM \[7\].

- Example ROM: 28-9E-9C-1F **-00-00-80-** 04
- Initial Scratchpad: xx/xx/FF/FF/7F/FF/FF/FF/xx

## MAX31820
The MAX31820 appears to be a DS18B20 with limited supply voltage range (i.e. up to 3.7 V) and smaller temperature range of high accuracy \[1,8\]. Like the DS18B20, it uses one-wire family code 0x28 \[1,8\]. Preliminary investigations have not (yet) revealed a test to distinguish between DS18B20 of Family A and Maxim-produced MAX31820 in software \[5\].

## Warning
**Sending undocumented function codes to a DS18B20 sensor may render it permanently useless,** for example if temperature calibration coefficients are overwritten \[5\]. The recommended (and currently sufficient) way of identifying counterfeit sensors is to analyze state and behavior of the scratchpad register in response to commands that comply with the datasheet \[5\].


(*Information on chips of Families A, B, C, and D comes from my own investigations of sensors in conjunction with the references below as indicated by reference number \[1,2,3,4,5,6\]. Information on chips of Family E comes from web searches \[7\]. Tests were performed at 5 V with 1.2 kOhm pull-up.*)

## References

1. [DS18B20](https://datasheets.maximintegrated.com/en/ds/DS18B20.pdf) "DS18B20 Programmable Resolution 1-Wire Digital Thermometer", Datasheet, Maxim Integrated.
2. [DS18S20](https://datasheets.maximintegrated.com/en/ds/DS18S20.pdf) "DS18S20 High-Precision 1-Wire Digital Thermometer", Datasheet, Maxim Integrated.
3. [AN4377](https://www.maximintegrated.com/en/design/technical-documents/app-notes/4/4377.html) "Comparison of the DS18B20 and DS18S20 1-Wire Digital Thermometers", Maxim Integrated
4. AN247 "DS18x20 EEPROM Corruption Issue", Maxim Integrated
5. Own investigations 2019, unpublished.
6. Petrich, C., M. O'Sadnick, Ø. Kleven, I. Sæther (2019). A low-cost coastal buoy for ice and metocean measurements. In Proceedings of the 25th International Conference on Port and Ocean Engineering under Arctic Conditions (POAC), Delft, The Netherlands, 9-13 June 2019, 6 pp.
7. Contribution of user *m_elias* on https://forum.arduino.cc/index.php?topic=544145.15
8. [MAX31820](https://datasheets.maximintegrated.com/en/ds/MAX31820.pdf) "1-Wire Ambient Temperature Sensor", Datasheet, Maxim Integrated.
