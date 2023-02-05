//  GNU General Public License v3.0
// This project is built around Rolf R Bakke’s project of a simple variometer,
//  described in https://www.rcgroups.com/forums/showthread.php?1749208-DIY-simple-and-inexpensive-Arduino-based-sailplane-variometer.
//  For the fun, a “voice output” was added and the height of the model is spoken on demand.

//  2/2023
// Version 1.0

#include "SoftwareSerial.h"
#include "RF24.h"
#include <EEPROM.h>
#include <Button.h>    //   Use this library: https://github.com/t3db0t/Button ( Tom Igoe's fork)
#include <toneAC.h>  // https://github.com/teckel12/arduino-toneac

//#define FSDEBUG
// You may change here the channel used by the NRF24L01.
// choose a "not too noisy" channel. 9X / TH9X uses 0 to 4D (77) ???

#define channelVario 0x50   //  Between 0 and 125  (0 and 0x7D)

#define  softRX  7 // softwareSerial RX wired to TX of DTPlayer - not used - not wired
#define  softTX  4  // softwareSerial TX wired to RX of DTPlayer

#define pinBusy  3  // pin BUSY du DFPlayer

#define buttonUpPin A2      // 
#define buttonDownPin A3     // 

#define pinLed 8

#define cepin 5 // For SMD module NRF24l01 on special adaptor for ProMini
#define cspin A0
// Pin 9 and 10 reserved for toneAC
//#define cepin 9 // For SMD module NRF24l01 on special adaptor for ProMini
//#define cspin 10
Button buttonUp = Button(buttonUpPin, BUTTON_PULLUP_INTERNAL, true, 50 );
Button buttonDown = Button(buttonDownPin, BUTTON_PULLUP_INTERNAL, true, 50 );
SoftwareSerial mySerial(softRX, softTX);  // RX  et TX for communication with DFPlayer

#define signatureEEPROM 3 // to be changed with any new value if we change the EEPROM conf structure, add fields, etc...
struct objectEEPROM   // configuration saved in EEPROM.
// Default value used for the 1st run.
{
  int signature ;// will be changed the first time with signatureEEPROM
  int incrementAltitude = 10;     // altitude announced ever 10m
  int unitAltitude = 0;  //  0: meter    2: feet
  int volToneAC = 5;  //  0 - 10
  int volDFP = 20;   // 0 - 30  10 is very weak
  boolean altimeterActivated = true;
  boolean varioActivated  = true;
} conf;  // configuration stored in EEPROM

byte addresses[][6] = {"Vario2"};
RF24 radio(cepin, cspin);  // RF24 (uint8_t _cepin chip enable RX/TX, uint8_t _cspin SPI chip select)
struct dataStruct {
  float toneFreq;
  float altitude;
  unsigned long count;  // increment at each message. For debug
  unsigned long work;   // for debug ....
} myData;
//int currentTone;
//unsigned long previousCount;  // count of previous message.
unsigned long timeMute, timePerdu, timeRelanceMenu;
unsigned long loopCount;
unsigned long debutTime, finTime;

int nbrMessages = 0;
unsigned long sumToneFreq = 0;
unsigned int minToneFreq = 30000;
unsigned int maxToneFreq = 0;
unsigned long sumTimeTransmitter = 0;
#define timeOutMute 2000    // mute the vario if contact lost more than xxx ms
#define timeOutPerdu  10000 // announce a message if contact lost more than  xxx ms
#define timeoutRelanceMenu 10000 // for restart of config menu if we don't give an answer in the setup menu


// For DFPlayer optimisation we have:
// One mp3 file for each number from 0 to 999, stored in several folders 01, 02, 03, 04
// Each folder contains 250 files, named 001.mp3, 002.mp3  ...
// Filename 000 is illegal in DFPlayer and so sound for number 0 is in file 001.mp3 in the first folder 01
// In folder 00 fil 001.mp3 if for nmber 0
// In folder 02  file 001.mp3 is for number 250 ....
// Next folder is for the thousand (1000 to 10000),
// and next folder for the messages
// Folder 05 contains the thousands from 1000 (file 001.mp3) to 10000 (file 010.mp3)
// Folder 06 contains the messages (see file messages.csv )
#define filesPerFolder 250  // seems the better for DFPlayer performance ...
#define folder1000 1000/filesPerFolder+1 // folder containing mp3 files for the "thousands" (1000 to 10000)
#define folderMessages folder1000+1  // folder containing messages
#define timeout 4000 // timeout for the DFPlayer (wait for busy high / low)

boolean longPushUp = false;
boolean longPushDown = false;
boolean settings = false;
int currentItem ; // current line in the setup menu

float altitudeTerrain;
int altitudeMax = 0;
int altitudeCourante = 0;
int derniereAnnonce = 0;    // last altitude announced
bool altitudeInitialised = false;

// voir http://www.gammon.com.au/callbacks for array address of function & Co ...
void altimetreActivation(bool action, Button &bouton);
void varioAtivation(bool action, Button &bouton);
void pasAltitude(bool action, Button &bouton);
void finMenu(bool action, Button &bouton);
void volumeDFP(bool action, Button &bouton);
void choixMetrePieds(bool action, Button &bouton);

typedef void (*GeneralFunction) (bool action, Button &bouton);
typedef struct menuEntry {
  GeneralFunction action;
};
const menuEntry menu[] {
  altimetreActivation,
  varioAtivation,
  choixMetrePieds,
  pasAltitude,
  volumeDFP,
  finMenu
};

void setup() {
  pinMode(pinBusy, INPUT);
  Serial.begin (74880);
  Serial.println(F("Starting ..."));
  mySerial.begin (9600);

  Serial.println(F("Reading settings from EEPROM ..."));
  int signature;
  EEPROM.get(0, signature);   // read the signature stored in EEPROM
  if (signature != signatureEEPROM)
  {
    // 1ere execution: init of the default configuration in EEPROM
    conf.signature = signatureEEPROM;
    EEPROM.put(0, conf);
    Serial.println(F("Init EEPROM with default values"));
  }
  else
    EEPROM.get(0, conf);
  Serial.print(F("volDFP from conf: ")); Serial.println(conf.volDFP);
  Serial.print(F("volToneAC from conf: ")); Serial.println(conf.volToneAC);
  Serial.println(F("Init NFR24 transmitter"));
  radio.begin();
  radio.setChannel(channelVario);
  // RF24_PA_MIN = 0,RF24_PA_LOW, RF24_PA_HIGH, RF24_PA_MAX,
  radio.setPALevel(RF24_PA_MAX);
  // speed RF24_250KBPS for 250kbs, RF24_1MBPS for 1Mbps, or RF24_2MBPS for 2Mbps
  radio.setDataRate( RF24_250KBPS ) ;  // par defaut on a  RF24_1MBPS, (canal 76(4C))
  radio.setPayloadSize(sizeof(myData));  // pour eviter d'envoyer 32bytes. Optimisation ??. A faire avant open Pipe
  radio.openReadingPipe(1, addresses[0]);
  radio.startListening();
  Serial.println(F("End init NRF24.\nInit DFPlayer"));
  initDFPlayer();
  Serial.println(F("End  initDFPlayer"));
  playFile(folderMessages, 27);  // Thanks for using the talking vario altimeter
  //
  Serial.println(F("Announcing current settings ..."));
  for (int i = 0; i <  sizeof(menu) / sizeof(menu[0]) - 1; i++)
  {
    menu[i].action (false, buttonUp);
  }
  buttonUp.releaseHandler(handlerButtonReleaseEvents);
  buttonDown.releaseHandler(handlerButtonReleaseEvents);
  buttonUp.holdHandler(handlerButtonHoldEvents, 1000);
  buttonDown.holdHandler(handlerButtonHoldEvents, 1000);
  playFile(folderMessages, 22); // waiting vario
  timeMute = millis() + timeOutMute;
  timePerdu = timeMute + timeOutPerdu;
  debutTime = millis();
  Serial.println(F("Here we go..."));
}
void loop() {
  buttonUp.process();
  buttonDown.process();

  if (!settings) {
    if ( radio.available()) // Check for incoming data from transmitter
    {
      while (radio.available())  // While there is data ready
      {
        radio.read( &myData, sizeof(myData) );
      }

      if (!altitudeInitialised) readAirfielElevation();
      if (conf.unitAltitude == 2) myData.altitude = myData.altitude * 3.28;
      altitudeCourante = myData.altitude - altitudeTerrain;
      altitudeMax = max(altitudeCourante, altitudeMax);
      if (conf.altimeterActivated && altitudeCourante != derniereAnnonce && altitudeCourante % conf.incrementAltitude == 0)
      {
        noToneAC();
        sayNumber ( altitudeCourante) ;
        derniereAnnonce = altitudeCourante;
        while (digitalRead(pinBusy) == LOW);  // waiting for DFPlayer idle
      }
      //     Serial.println(myData.toneFreq);
      if (conf.varioActivated )
      { // faire du bruit si pas en mode settings et si vario activé
        if (myData.toneFreq == 0xFFFF)
        {
          noToneAC();
        }
        else if (myData.toneFreq >= 0)
        {
          toneAC(myData.toneFreq, conf.volToneAC);
        }
      }
      timeMute = millis() + timeOutMute;
      timePerdu = timeMute + timeOutPerdu;
#ifdef FSDEBUG
      if (myData.toneFreq != 0xFFFF)
      {
        sumToneFreq += myData.toneFreq;
        minToneFreq = min(myData.toneFreq, minToneFreq );
        maxToneFreq = max(myData.toneFreq, minToneFreq );
        nbrMessages++;
        sumTimeTransmitter += myData.work;
        if (nbrMessages >= 200)
        {
          // This period should be given by the period of the transmitter: 20ms
          finTime = millis();
          Serial.print(F("Period for 1 receive/update loop micro sec:")); Serial.println((finTime - debutTime));
          Serial.print(F("Average tone frequency: ")); Serial.print(sumToneFreq / 200);
          Serial.print(F("\tmin: ")); Serial.print(minToneFreq);
          Serial.print(F("\tmax: ")); Serial.println(maxToneFreq);
          Serial.print(F("Average loop time in transmitter: ")); Serial.println(sumTimeTransmitter / 200);
          nbrMessages = 0;
          sumToneFreq = 0;
          minToneFreq = 30000;
          maxToneFreq = 0;
          sumTimeTransmitter = 0;
        }
      }
      debutTime = millis();
#endif
    } //END Radio available

    //
    if (millis() > timeMute)
    {
      noToneAC();
    }
    if (millis() > timePerdu)
    {
      timePerdu = millis() + timeOutPerdu;
      playFile (folderMessages, 25) ; // contact lost
    }
  }  // if !settings

  else if ( millis() >= timeRelanceMenu)
  {
    timeRelanceMenu = millis() + timeoutRelanceMenu;
    Serial.print(F("+++++++timeout settings currentItem:")); Serial.println(currentItem);
    playFile(folderMessages, 1);  // Wake up the user !! welcome setup
    menu[currentItem].action (false, buttonUp);  // and re-announce the current item/line in the menu
    timeRelanceMenu = millis() + timeoutRelanceMenu;
  }

} //--(end main loop )-- -

void readAirfielElevation()
{
  Serial.println(F("Reading airfield elevation"));
  timePerdu = millis() + timeOutPerdu;
  for (int i = 1; i <= 100; i++)
  {
    while ( !radio.available()) {
      if (millis() > timePerdu) {
        playFile (folderMessages, 25) ; // contact lost
        timePerdu = millis() + timeOutPerdu; ; // attendre le vario
      }
    }
    while (radio.available())  // While there is data ready
    {
      radio.read( &myData, sizeof(myData) ); // Get the data payload
    }
    altitudeTerrain = altitudeTerrain + myData.altitude;
  }
  altitudeTerrain = altitudeTerrain / float(100);
  altitudeInitialised = true;
  // this altitude is not accurate: there is no compensation.
  // But we will use differences to estimate the altitude of the plane above the airfield, and so it does not matter ..
  Serial.print(F("Airfield elevation (meters): ")); Serial.println(altitudeTerrain);
  if (conf.unitAltitude == 2) altitudeTerrain = altitudeTerrain * 3.28;
  // playFile(folderMessages, 23); // Field elevation
  // sayNumber ( altitudeTerrain) ;
  // playFile(folderMessages, 19 + conf.unitAltitude);  // meters or feet
}

void playFile(uint8_t folder, uint16_t fileNbr) {
  Serial.print(F("\tPlaying folder: ")); Serial.print(folder);
  Serial.print(F("\tfile: ")); Serial.println(fileNbr);
  long debutWaitBusy = millis();
  while (digitalRead(pinBusy) == LOW)  // waiting for DFPlayer idle
    if (millis() - debutWaitBusy > timeout) {
      Serial.print(F("\t++++ error. Time out waiting for busy high (DTPlayer idle)."));
      initDFPlayer();
      debutWaitBusy = millis();
    }
  execute_CMD(0x0F, folder, fileNbr);
  debutWaitBusy = millis();
  while (digitalRead(pinBusy) != LOW)  // attendre le début du busy
    if (millis() - debutWaitBusy > timeout) {
      Serial.print(F("\t++++ error. Time out waiting for busy low (DTPlayer running)."));
      initDFPlayer();
      execute_CMD(0x0F, folder, fileNbr);  //  retry ...
      debutWaitBusy = millis();
    }
}

void sayNumber(int nbr) {
  if (nbr < 0)
  {
    playFile(folderMessages, 24);  // minus ...
    nbr = -nbr;
  }
  if (nbr > 10999) return;
  if (nbr / 1000 >= 1) playFile(folder1000, nbr / 1000);
  // filename "000" is invalid.  The first file in a folder is 001.mp3 (speeling of 0  is in fomder 01, file 001.mp3)
  if (nbr % 1000 != 0  || nbr == 0) playFile(((nbr % 1000) / filesPerFolder) + 1, nbr % filesPerFolder + 1);
}

void initDFPlayer() {
  delay(1000);
  execute_CMD(0x0C, 0, 0);  // reset module
  delay(4000);
  execute_CMD(0x06, 0, conf.volDFP);         // Set the volume (0x00~0x30)
  delay(100);
  while (digitalRead (pinBusy) == LOW) {
    delay(100);
  }
}
void execute_CMD(byte CMD, byte Par1, byte Par2)
//  For DFPlayer
#define Start_Byte     0x7E
#define Version_Byte   0xFF
#define Command_Length 0x06
#define End_Byte       0xEF
#define Acknowledge    0x00  //Returns info with command 0x41 [0x01: info, 0x00: no info]
// Excecute the command and parameters
{
  // Calculate the checksum (2 bytes)
  word checksum =  -(Version_Byte + Command_Length + CMD + Acknowledge + Par1 + Par2);
  // Build the command line
  byte Command_line[10] = { Start_Byte, Version_Byte, Command_Length, CMD, Acknowledge,
                            Par1, Par2, highByte(checksum), lowByte(checksum), End_Byte
                          };
  //Send the command line to the module
  for (byte i = 0; i < 10; i++)
  {
    mySerial.write( Command_line[i]);
  }
  //  digitalWrite( softTX, LOW);
}

void announceAltitudeMax()
{
  playFile(folderMessages, 11);  // altitude
  Serial.print(F("Announcing current altitude: ")); Serial.println (altitudeCourante);
  sayNumber ( altitudeCourante) ;
  playFile(folderMessages, 19 + conf.unitAltitude);  // meters or feet
  while (digitalRead(pinBusy) == LOW);  // waiting for DFPlayer idle
  playFile(folderMessages, 10);  // highest altitude
  Serial.print(F("Announcing maximum altitude: ")); Serial.println (altitudeMax);
  sayNumber ( altitudeMax) ;
  playFile(folderMessages, 19 + conf.unitAltitude);  // meters or feet
  while (digitalRead(pinBusy) == LOW);  // waiting for DFPlayer idle
}

void handlerButtonReleaseEvents(Button & btn) {
  if ( btn == buttonUp && longPushUp )   // ignore the release event comming from a long push.
    longPushUp = false;
  else if (btn == buttonDown && longPushDown )
    longPushDown = false;
  else
  {
    noToneAC();
    timeRelanceMenu = millis() + timeoutRelanceMenu;
    if (settings) {
      // if release button in setting mode and not comming just after a long press/hold: execute the action
      // get function address  voir // http://www.gammon.com.au/callback
      menu[currentItem].action (true, btn);
    }
    else   // release button outside settting mode: vol+-
    {
      if (btn == buttonUp )
      {
        Serial.println("volToneAC +");
        conf.volToneAC++;
      }
      else
      {
        Serial.println("volToneAC -");
        conf.volToneAC--;
      }
      conf.volToneAC = constrain(conf.volToneAC, 1, 10);
      Serial.print(F(" volToneAC:")); Serial.println(conf.volToneAC);
    }  // release not in settings mode
  }  // release not after a long push/hold
}


// Handling of a button hold: we enter the setting mode or change line of the menu.
// If the other button is also pressed: we announce the current & max altitude.
void handlerButtonHoldEvents(Button & btn) {
  if (btn == buttonUp && buttonDown.isPressed() || btn == buttonDown && buttonUp.isPressed() )
  {
    Serial.println("Announcing altitudes");
    announceAltitudeMax();
    timePerdu = millis() + timeOutPerdu; //  reset the watchdog because it may take a long time to annouce altitude
    longPushUp = true;
    longPushDown = true;
  }
  else
  {
    Serial.println(F("Hold"));
    timeRelanceMenu = millis() + timeoutRelanceMenu;
    noToneAC();
    if (!settings)  { // 1st hold: enter settings mode
      settings = true;
      Serial.println("Entree en mode setting ");
      playFile(folderMessages, 1); // Welcome setup
      // select the first or last line of the menu, depending on the button
      currentItem = btn == buttonUp ? 0 : sizeof(menu) / sizeof(menu[0]) - 1;
    } else {
      if  (btn == buttonUp ) { // appui long bouton central: passer à la ligne menu suivante, ou à la 1ere si on etait à la fin
        currentItem++ ;
        if (currentItem == sizeof(menu) / sizeof(menu[0])) currentItem = 0; // wrap
      }
      else {
        currentItem--; // appui long autre bouton: passer à la ligne menu précédente, ou à la derniière si on etait au début
        if (currentItem < 0) currentItem = sizeof(menu) / sizeof(menu[0]) - 1; // wrap
      }
    }
    Serial.print (" Ligne menu:"); Serial.println(currentItem);
    menu[currentItem].action (false, btn);
  }
  if (btn == buttonUp)
    longPushUp = true;  // just to remember we don't need to handle the release of this button.
  else
    longPushDown = true;
}
void volumeDFP(boolean action, Button & bouton) {
  // between 0 and 30

  if (action)
  {
    if ( bouton == buttonUp )
      conf.volDFP++;
    else
      conf.volDFP--;
    conf.volDFP = constrain(conf.volDFP, 1, 30);
    Serial.print(F(" volDFP:")); Serial.println(conf.volDFP);
  } else
  {
    Serial.println("Announcing choice for volDFP");
    playFile(folderMessages, 13); // Volume of spoken messages
  }
  while (digitalRead(pinBusy) == LOW);  // waiting for DFPlayer idle
  // play a tone and then a message to have an idea of the volume DFPlayer and tone ....
  //  toneAC (510, conf.volDFP) ; //
  //  delay(500);
  //  noToneAC();
  execute_CMD(0x06, 0, conf.volDFP);         // Set the volume (0x00~0x30)
  delay(50);
  Serial.println("Announcing volDFP");
  sayNumber ( conf.volDFP) ;
}

void choixMetrePieds(bool action, Button & bouton)
{
  if (action) {
    if (conf.unitAltitude == 0 ) // from meter to feet
    {
      conf.unitAltitude = 2;
      altitudeTerrain = altitudeTerrain * 3.28;
      altitudeMax = altitudeMax * 3.28;
    }
    else  // from feet to meter
    {
      conf.unitAltitude = 0;
      altitudeTerrain = altitudeTerrain / 3.28;
      altitudeMax = altitudeMax / 3.28;
    }
    Serial.println("Announcing value for unit altitude");
    playFile(folderMessages , 19 + conf.unitAltitude);
  }
  else
  {
    Serial.println("Announcing choice and value for unit altitude");
    playFile(folderMessages, 8);  // Altitude annonce in ...
    playFile(folderMessages , 19 + conf.unitAltitude);
  }
}

void pasAltitude(bool action, Button & bouton)
{
  if (action) {
    if ( bouton == buttonUp )
    {
      if (conf.incrementAltitude == 50 ) conf.incrementAltitude = 1; // wrap entre 1 et 50
      else conf.incrementAltitude++;
    }
    else
    {
      if (conf.incrementAltitude == 1) conf.incrementAltitude = 50;
      else conf.incrementAltitude--;
    }
    Serial.println("Announcing value step for altitude");
    sayNumber ( conf.incrementAltitude) ;
  }
  else
  {
    Serial.println("Announcing choice and value step for altitude");
    playFile(folderMessages, 9);  // Altitude announced every ...
    sayNumber ( conf.incrementAltitude) ;
    playFile(folderMessages, 19 + conf.unitAltitude);  // meters or feet
  }
}

void altimetreActivation(boolean action, Button & bouton)
{
  if (action)
  {
    Serial.println("Toggling altimeter");
    conf.altimeterActivated = !conf.altimeterActivated;
  }
  Serial.println("Announcing altimeter on/off");
  if (conf.altimeterActivated)
    playFile(folderMessages, 2);  // Altimeter on
  else
    playFile(folderMessages, 3);  // Altimeter off
}

void varioAtivation(boolean action, Button & bouton)
{
  if (action)
  {
    Serial.println("Toggling vario");
    conf.varioActivated = !conf.varioActivated;
  }
  Serial.println("Announcing vario on/off");
  if (conf.varioActivated)
    playFile(folderMessages, 5);  // vario on
  else
    playFile(folderMessages, 6); // vario off
}

void finMenu(boolean action, Button & bouton)
{
  if (action)
  {
    Serial.println("Exit setup");
    playFile(folderMessages, 15);  //  You exit setup menu
    settings = false;
    EEPROM.put(0, conf);  // sauvegarder la config que l'on a surement modifiée
    while (digitalRead(pinBusy) == LOW);  // waiting for DFPlayer idle
  } else
  {
    Serial.println("Announcing exit setup");
    playFile(folderMessages, 14);  // Exit setup menu
  }
}
