//  GNU General Public License v3.0

// Transmitter for vario / altimeter  2.4Ghz, with NRF24L01 module
//
//  Vario based on the work of "kapteinkuk"  Rolf R Bakke http://www.rcgroups.com/forums/showthread.php?t=1749208
//
// Transmitter for vario / altimeter  2.4Ghz, with NRF24L01 module (better with PA+LNA such as E01-ML01DP5 module)
// Processor Arduino Pro Mini 5v , powered 5v from the RC receiver
// Voltage regulator  3.3V LDO  MCP1700 for the NRF24L01
// Barometric presure sensor module Gy-63 (with MS5611 sensor).
//    It is a 5v module, that includes its own 3.3v LDO and level shifters.

// The message contains:
//    -the vario sound to be played by the receiver (as computed in http://www.rcgroups.com/forums/showthread.php?t=1749208 )
//    -the altitude (absolute value nt accurate ... No calibration. But we will uses differences )
//    -message number (incremented for each message)
//    -the main loop execution time (for debug ...  .  It is arround 15ms)
//  No acknowledge required
//
// Version 1.0
#include "RF24.h"
#include <Wire.h>

// You may change here the channel used by the NRF24L01.
// choose a "not too noisy" channel. 9X / TH9X uses 0 to 4D (77) ???
//  Between 0 and 125  (  0 and 0x7D )  Default 0x4C (76)
#define channelVario 0x50   //  Between 0 and 125  (0 and 0x7D)

#define cepin 7  // CE / CS pin of NRF24L01 module
#define cspin 8  // 
byte addresses[][6] = {"Vario2"};
RF24 radio(cepin, cspin);  // RF24 (uint8_t _cepin chip enable RX/TX, uint8_t _cspin SPI chip select)

unsigned int calibrationData[7];
float toneFreq, toneFreqLowpass, pressure, lowpassFast, lowpassSlow;
int ddsAcc;
unsigned long  startSampleTime;
int failed = 0;

//  message sent by the radio link:
struct dataStruct {
  float toneFreq;
  float altitude;
  unsigned long count;  // increment at each message. For debug
  unsigned long dureeBoucle;   //  For debug . Length off a loop: read pressure+ send packet
} myData;

float altitude;
// for smoothing, in altitude computation (based on 10 samples)
//  (see https://docs.arduino.cc/built-in-examples/analog/Smoothing )
const int numReadings = 10;
float readings[numReadings];      //  readings from the  input
int readIndex = 0;                //  index of the current reading
float total = 0;                  //  the running total


void setup() {
  Wire.begin();
  Serial.begin(9600);
  delay(1000);
  Serial.println(F("Starting..."));
  radio.begin();
  radio.setChannel(channelVario);
  // Set the PA Level low to prevent power supply related issues since this is a
  // getting_started sketch, and the likelihood of close proximity of the devices. RF24_PA_MAX is default.
  // RF24_PA_MIN = 0,RF24_PA_LOW, RF24_PA_HIGH, RF24_PA_MAX,

  radio.setPALevel(RF24_PA_MAX);
  // speed RF24_250KBPS for 250kbs, RF24_1MBPS for 1Mbps, or RF24_2MBPS for 2Mbps
  radio.setDataRate( RF24_250KBPS ) ;  // by default we have  RF24_1MBPS, (canal 76(4C))
  radio.setPayloadSize(sizeof(myData));  // Optimisation. Avoid sending 32bytes. To be donne before open Pipe
  radio.openWritingPipe(addresses[0]);
  radio.enableDynamicAck();  // pour faire des write sans ACK
  myData.count = 0;
  Serial.println(F("End init radio..."));
  setupSensor();
  Serial.println(F("End init sensor..."));
  smoothing_Init();
  pressure = getPressure();
  lowpassFast = lowpassSlow = pressure;
  Serial.println(F("Here we go..."));
  startSampleTime = millis() ;
}


void loop()
{
  startSampleTime = millis() ;
  pressure = getPressure();  //  en 1/100 de mbar

  lowpassFast = lowpassFast + (pressure - lowpassFast) * 0.1;
  lowpassSlow = lowpassSlow + (pressure - lowpassSlow) * 0.05;

  toneFreq = (lowpassSlow - lowpassFast) * 50;

  toneFreqLowpass = toneFreqLowpass + (toneFreq - toneFreqLowpass) * 0.1;

  toneFreq = constrain(toneFreqLowpass, -500, 500);

  ddsAcc += toneFreq * 100 + 2000;

  if (toneFreq < 0 || ddsAcc > 0)
  {
    // tone(2, toneFreq + 510);
    myData.toneFreq = toneFreq + 510;
  }
  else
  {
    // noTone(2);
    myData.toneFreq = 0xFFFF;
  }
  // height above mean sea level. No calibration....
  // In fact we will use the difference between 2 altitudes to announce the altitude above the field.
  altitude = 44330 * (1.0 - pow(pressure / 101325, 0.1903));
  myData.altitude =  smoothing(altitude);
  myData.count++;
  if (!radio.write( &myData, sizeof(myData), 1 )) { // noack
    failed++;
    Serial.print(F("failed ")); Serial.print(failed); Serial.print(F(" cnt:")); Serial.println(myData.count);
  }
  myData.dureeBoucle = millis() - startSampleTime;  // in fact we send the loop time of the previous loop ...
  // Result of test: In fact the loop is done in 14ms ( 10 + 1 come from delay in getpressure )
  // if we remove the following synchro at the end of the loop
  while (millis() < startSampleTime+20);        //loop frequency timer (the transmiter runs full speed)
  startSampleTime = millis();
} // Loop

// A bit of smoothing for altitude computation.
// Based on https://docs.arduino.cc/built-in-examples/analog/Smoothing
void smoothing_Init()
{
  // initialize all the readings to 0:
  for (int thisReading = 0; thisReading < numReadings; thisReading++) {
    readings[thisReading] = 0;
  }
  // fill up the smoothing array with "real" value.
  // The first altitude we will send will be more accurate and it is better because it may be used as the field altitude
  //
  for (int i = 0; i < numReadings; i++)
  {
    delay(100);
    pressure = getPressure();  //  en 1/100 de mbar
    altitude = 44330.0 * (1.0 - pow(pressure / 101325.0, 0.1903));
    altitude = smoothing(altitude);
  }
}
float smoothing(float sample)
{
  // subtract the last reading:
  total = total - readings[readIndex];
  // read from the sensor:
  readings[readIndex] = sample;
  // add the reading to the total:
  total = total + readings[readIndex];
  // advance to the next position in the array:
  readIndex = readIndex + 1;
  // if we're at the end of the array...
  if (readIndex >= numReadings) {
    // ...wrap around to the beginning:
    readIndex = 0;
  }
  // calculate the average:
  return   total / numReadings;

}
long getPressure()
{
  long D1, D2, dT, P;
  float TEMP;
  int64_t OFF, SENS;

  D1 = getData(0x48, 10);   // convert D1, OSR 4096  pression, delais 10ms  (max 9.4 datasheet)
  D2 = getData(0x50, 1);    // convert D2, OSR 256   temperature, delis 1ms (max 0.6ms)

  dT = D2 - ((long)calibrationData[5] << 8);
  TEMP = (2000 + (((int64_t)dT * (int64_t)calibrationData[6]) >> 23)) / (float)100;
  OFF = ((unsigned long)calibrationData[2] << 16) + (((int64_t)calibrationData[4] * dT) >> 7);
  SENS = ((unsigned long)calibrationData[1] << 15) + (((int64_t)calibrationData[3] * dT) >> 8);
  P = (((D1 * SENS) >> 21) - OFF) >> 15;
  return P;
}


long getData(byte command, byte del)
{
  long result = 0;
  twiSendCommand(0x77, command);
  delay(del);
  twiSendCommand(0x77, 0x00); // ADC read command
  Wire.requestFrom(0x77, 3);
  if (Wire.available() != 3) Serial.println(F("Error in getData: raw data not available"));
  for (int i = 0; i <= 2; i++)
  {
    result = (result << 8) | Wire.read();  // lecture 24bits
  }
  return result;
}


void setupSensor()
{
  twiSendCommand(0x77, 0x1e);
  delay(100);
  for (byte i = 1; i <= 6; i++)
  {
    unsigned int low, high;
    twiSendCommand(0x77, 0xa0 + i * 2);   // PROM read
    Wire.requestFrom(0x77, 2);
    if (Wire.available() != 2) Serial.println(F("Error: calibration data not available"));
    high = Wire.read();
    low = Wire.read();
    calibrationData[i] = high << 8 | low;
  }
}


void twiSendCommand(byte address, byte command)
{
  Wire.beginTransmission(address);
  if (!Wire.write(command)) Serial.println(F("Error: Wire write()"));
  if (Wire.endTransmission())
  {
    Serial.print(F("Error twiSendCommand when sending command: "));
    Serial.println(command, HEX);
  }
}
