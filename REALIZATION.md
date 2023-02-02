## Realization of the receiver

We use here a small easily available SMD module for the NRF24L01.
It fit very well on an Arduino Pro Mini with a specific adapter available at
[OSHPARK](https://oshpark.com/shared_projects/CtW4Xaow) and gives a compact realization.

As designed, the adapter uses pins 9 and 10 of the Arduino for CE/CS but we must reserved these pins for the toneAC library and the buzzer.
The adapter can’t be soldered directly on the Pro Mini and we need to use some spacers and wiring to relocate CS and CE to pin 5 and A0. Picture is worth a thousand words.

|<img src="/images/NRF24L SMD.jpg" width="200">|<img src="/images/Pro Mini NRF24L_1.jpg" width="300">|<img src="/images/Pro Mini NRF24L_2.jpg" width="300">|
|-------|-----|----|
| NRF24L01 SMD and Pro Mini adaptor|Voltage regulator and capacitor added|Spacers and CE/CS wiring| 
|<img src="/images/recept_1.jpg" width="300">|<img src="/images/recept_2.jpg" width="300">|<img src="/images/recept_3.jpg" width="300">|  

