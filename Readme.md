# Project
nfcAudio is a wifi enabled MP3 player with NFC capability. A tag is placed on top and corresponding file is played.


# Software
Each time an RFID tag is detected:  
1. It is compared to volume tags
2. If a music is playing with the same tag, stop playing
3. Else GET an URL with the tag ID
4. If there the file contains an URL to an MP3 file, play it.

## Libraries
This project depends on
* [ESP8266 Audio](https://github.com/earlephilhower/ESP8266Audio) - Wonderfull MP3 decoder
* [Adafruit PN532](https://github.com/Oliv4945/Adafruit-PN532/tree/interrupt_spi) - NFC library from Adafruit, which I modified to use interrupts instead of defalut blocking functions. You have to use the "interrupt_spi" branch from my repo until my pull request is merged.

# Hardware
## Tags
Small stickers as well as standard "credit card" size tags can be used. I use a printed picture with thermal laminated film, but you can buy tags that can be printed if you have corresponding printer.  
Stickers can also be put underneath objects, toys, 3D printed things... Lot of fun for my 3 years old kid !

## Components
* I2S DAC MAX98357A - From [Adafruit](https://www.adafruit.com/product/3006)
* ESP8266 - [Wemos mini D1](https://wiki.wemos.cc/products:d1:d1_mini)
* NFC Reader PN532

## Wiring
### I2S DAC - MAX98357A
| I2S DAC board  | Wemos mini D1 mini pin  | ESP8266 pin  |
| ------- |:---:| ----:|
| LRC     | D4  | IO2  |
| BLCK    | D8  | IO15 |
| DIN     | RX  | IO3  |
| GND     | GND | GND  |
| VCC     | 5V  |      |

### NFC Reader - PN532
| NFC Reader  | Wemos mini D1 mini pin  | ESP8266 pin  |
| ------- |:---:| ----:|
| SCK     | D5  | IO14 |
| MISO    | D6  | IO12 |
| MOSI    | D7  | IO13 |
| SS      | D1  | IO5  |
| VCC     | 3.3V|      |
| GND     | GND | GND  |
| IRQ     | D2  | IO4  |
| RST     | NC  | NC   |


## Case
Design files are available in [onShape](https://cad.onshape.com/documents/462111829eacaa66e3425406/w/186305e0bc17a139465da4be/e/d87f807332cb58a96165f9d4), or in the [case](https://github.com/Oliv4945/nfcAudio/case) folder.  
The onShape model is fully parametrized, so all variables can be changed to fit you needs.
