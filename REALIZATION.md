## Realization of the receiver

We use here a small easily available SMD module for the NRF24L01.
It fit very well on an Arduino Pro Mini with a specific adapter available at
[OSHPARK](https://oshpark.com/shared_projects/CtW4Xaow) and gives a compact realization.  

More information on this adapter [here](http://www.thinkering.de/cms/?p=687) and [here](https://forum.mysensors.org/topic/6220/nrf24l01-1-27mm-arduino-pro-mini-adapter-board). 

As designed, the adapter uses pins 9 and 10 of the Arduino for CE/CS but we must reserved these pins for the toneAC library and the buzzer.
The adapter can’t be soldered directly on the Pro Mini and we need to use some spacers and wiring to relocate CS and CE to pin 5 and A0. Picture is worth a thousand words.

|<img src="/images/NRF24L SMD.jpg" width="200">|<img src="/images/Pro Mini NRF24L_1.jpg" width="300">|<img src="/images/Pro Mini NRF24L_2.jpg" width="300">|
|-------|-----|----|
| NRF24L01 SMD and Pro Mini adaptor|Voltage regulator and capacitor added|Spacers and CE/CS wiring| 
|<img src="/images/recept_1.jpg" width="300">|<img src="/images/recept_2.jpg" width="300">|<img src="/images/recept_3.jpg" width="300">|  

*Remark*:  
If you hear a rumble or kind of tac-tac at ‘high’ volume, you may have problem with power supply.  
The power distribution schema used here is:  
A 1S lipo powering the Arduino Pro Mini by the RAW input and directly the DFPlayer.  
A 3.3v MCP1700 ldo voltage regulator powered from the lipo and used for the NRLF24L01.  

(The DFPlayer working voltage is 3.2v to 5v but the module's external interfaces are 3.3V)  

Don’t use the 3.3v VCC output pin of the Arduino Pro Mini for powering the NRF24L01 and/or the DFPlayer: it doesn’t deliver enough power.
The same apply for the 3.3v for a FTDI USB adapter.
