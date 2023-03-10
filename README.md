## talking_variometer_altimeter
### A Poor Man’s simple and inexpensive Arduino-based RC sailplane talking variometer and altimeter

This project is built around Rolf R Bakke’s project of a [DIY simple and inexpensive Arduino-based sailplane variomete](<https://www.rcgroups.com/forums/showthread.php?1749208-DIY-simple-and-inexpensive-Arduino-based-sailplane-variometer>).  
For the fun, a “voice output” was added and the height of the model is spoken on demand.  

Main features:  
- Standalone system: it doesn’t rely on some telemetry channel offered by RC equipments but provides its own downlink transmitter and receiver in the 2.4 MHz band.
- Announce altitude increases with a specified step
- Announce on demand max / current altitude 
- Setup menu
- Choice of unit meters / feet
- Language for voice output easily customizable
- Built with inexpensive modules easily available

The system register the highest altitude reached during the current flight and can announce numbers up to 10999, positive or negative. (Hope that you are simply at the top of the slope if announced altitude is negative …)   

The transmitter and the receiver use only on one channel in the 2.4 GHz band.
If need be, you can change this channel in the transmitter and receiver source code. See at the beginning of source code the line

```
#define channelVario 0x50   //  Between 0 and 125  (0 and 0x7D)
```

## The transmitter

**Bill of material:**

- Arduino Pro Mini 5v 16Mhz
- Module GY-63 MS5611 barometric pressure sensor
- Module NRF24L01 with PA+LNA (E01-ML01DP5)
- MCP1700 3.3v voltage regulator
- 2 capacitors 100µF
- RC cable with servo type plug. 
 <img src="/images/emetteur%20NRF24%20vario_bb.jpg"  width="600">
 
|<img src="/images/nRF24L01.jpg" width="200">|<img src="/images/GY-63.JPG" width="150">|<img src="/images/vario_3.jpg" width="200">|
|-------|-----|----|
| NRF24L01 with PA+LNA (E01-ML01DP5)|GY-63 (MS5611)|
|<img src="/images/vario_1.jpg" width="200">|<img src="/images/vario_2.jpg" width="200">|<img src="/images/vario_4.jpg" width="200">|  

This NRF24L01(E01-ML01DP5) module has the same size as the Arduino Pro Mini. If the SMA connector is too bulky it can be removed and the antenna replaced by a simple wire.  <br>

The transmitter is powered directly from the RC equipment, in 5v, through for example a spare servo plug on the receiver.  

**Wiring:** During the realization / wiring it may be wise to let at least the I2C SDA/SCL pins, as well as VCC / GND easily available for future development (airspeed sensor for example).   
Depending on the physical implementation pin 7 (CE of the NRF24L01) and 8 (CS ) may be relocated elsewhere if it helps. See the “#define” in the source code. Others pins assignment must be respected.

## The receiver / ground station

**Bill of material:**

- Arduino Pro Mini 3.3v 8Mhz
- Module NRF24L01 (optional: with PA+LNA E01-ML01DP5, or SMD NRF24L odule with adapter for Pro Mini)
- DFPlayer Mini MP3 reader
- MCP1700 3.3v voltage regulator
- 1 capacitor 100µF
- Small speaker 8Ω
- Passive buzzer 12x8mm
- Lipo battery 1s 120ma as a minimum
- Lipo charge / discharge management module TP4056
- Switch 
 <img src="/images/recepteur%20vario%20NRF24%20public_bb.jpg"  width="600">
 
|<img src="/images/DFPlayer.jpg" width="150">|<img src="/images/chargeur.JPG" width="150">|<img src="/images/buzzer.JPG" width="70">|<img src="/images/recept_1.jpg" width="300">|
|-------|-----|---|---|

Don’t forget to adapt the charge current in the TP4056 module to the lipo you use ([see for example this video](https://www.youtube.com/watch?v=oMez6wHsvC4)). As delivered the module is usually set up for 1000ma battery.  
The sound of the buzzer may be greatly changed when you plug the small hole at the top of the buzzer with a piece of tape.   

**Wiring:** some pins assignment must be respected: pins 9/10 are reserved for the buzzer and pins 11/12/13 are reserved for the NRF24L01. Other pins may be relocated if it helps the wiring. See “#define” in the source code.  

More information / pictures [here](REALIZATION.md)  

## Using the variometer / altimeter

At power on, the system announces its current configuration and waits for a link with the transceiver. The current altitude is registered and the bip-bip of the variometer starts (if activated). Each time the current altitude of the model increments by a configurable step, the gain in altitude is announced (if activated).

Video on [YouTube](https://www.youtube.com/watch?v=yhJU-6fNBOY)   

### Setup menu. Buttons handling


The receiver in ground station features 2 push buttons used for managing the volume and the setup menu: buttonUp and buttonDown. 

The setup menu allows to control: 
- Activation / deactivation of the variometer
- Activation / deactivation of the altimeter
- Choice of the unit for altitude: meters or feet
- Choice of the step for altitude announcement
- Volume management for the messages

The configuration is saved and will be retrieved at next power up. 

When in normal mode:   
- A click on any button will increment/decrement the volume of the buzzer used for the variometer.
- Pressing both buttons 1 second will announce the current altitude and the maximum altitude since power on.
- Long push on any button will enter the setting mode. 

When in setting mode:   
The system will announce the menu entry and the associated parameter value. 
- short push on a button:
  - if the current entry is “Exit the setting menu”:  Exit setting mode and return to normal mode
  - For other entries:  increment or decrement a parameter value, or toggle the value. The system will announce the new value 
- Long push on a button: register le current value and pass to the next/ previous entry in the menu.  

Remark: action on a button is not detected when the system is talking.

## Development requirements:  

- Arduino IDE
- RF24 library by TMRH20 installed with Arduino IDE library manager.
- toneAC by Tim Eckel installed with Arduino IDE library manager.
- Button library by t3db0t installed from <https://github.com/t3db0t/Button>
- Other libraries used by the project come with the IDE install. 

## Building the SD card

Voice output is generated by the DFPlayer module that includes a micro SD memory card slot with mp3 files.  
As delivered, the package already includes the files for English, French, Italian and Spanish but it is very easy to generate the files for a new language as we will see latter.   
The set of file for a specific language uses around 12Mb.  
Each directory for a language contains an mp3 folder, with subfolders 01 to 06 containing themselves the mp3 files. These subfolders must be copied at the root of the SD card.   
In order to optimize the response time of the DFPlayer modules it is better to use a newly formatted card and make the copy one folder after the other, in the order 00 to 06, just to be sure that folder 00 is physically copied first on the SD card, 01 the second etc..  (In Windows, it is impossible to control the order of the copy in a multiple select/drag/drop scenario) 

## Customizing the language.

With the help of **gTTS** (*Google Text-to-Speech*), a Python library and CLI tool to interface with Google Translate's text-to-speech API we generate spoken MP3 files that are then copied on the micro SD card.  (More info <http://gtts.readthedocs.org/>)

2 kinds of files are generated:

- files for numbers
  - one MP3 file for each number from 0 to 999 stored in folders names 01, 02, 03, 04
  - ten MP3 files for the thousand 1000 to 10000 stored in folder 05
- A set of files for menu management. and announces  stored in folder 06

**Software environment:**

You must install **Python** on your system as well as **gTTS** and **eyeD3** libraries.
```
>pip install gTTS
>pip install eyeD3
```
**Customization:**

In a new working directory, make a copy of file one of the delivered [messages.txt](english/messages.txt) file and edit it.  
Each line of the file is composed of 3 field separated by “;”.   
Don’t change the first line of the file nor the first field (a file number) of each line.  
Translate every second field (Google Translate is my friend …).  
The third field is just a comment and ignored by the process.
```
Example: 		8;Altitude announced in; choice of meters or feet
```
Make a copy of Python script [generate_mp3.py](english/generate_mp3.py) and edit it (a simple text editor is OK …)   
Only one line must be change to customize the language:
```
langage = 'en'     # change this line for support of a new language.
```
The list of supported languages is available [here](list_of_languages.txt) (more than 50)

In Windows, start a PowerShell and from the newly created working directory run your customized script generate\_mp3.py
```
>cd “path to my_new_dir”
>python generate_mp3.py
```
The Python script generates an mp3 folder, with subfolders 01 to 06 containing the mp3 files that must be copied at the root of the SD card. ([See above](#building-the-sd-card))

Check mp3 files, mainly in directory “06” for the messages files.

**Warning:** gTTS needs  access to Google servers, and if the network is slow or access to google.com unavailable, the Python script may fails. You can try to use an other Top-level domain for the Google Translate.   
Choosing an other Top Level domain may also help to get **a better translation with your local ‘accent’.**   
See description of [tld parameter](https://gtts.readthedocs.io/en/latest/module.html#gtts.tts.gTTS) and of [Localized ‘accents’](https://gtts.readthedocs.io/en/latest/module.html#localized-accents)  in gTTS documntation.   
Only one line must be change in the script generate_mp3.py to customize the Top Level domain:
```
topLevelDomain = 'com'  # change this line if you want a specific Top Level Domain.
```
The list of supported Top Level Domain  is available [here.](https://gtts.readthedocs.io/en/latest/module.html#localized-accents)   

---





