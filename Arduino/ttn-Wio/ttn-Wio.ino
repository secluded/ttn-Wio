//FOR THIS TO COMPILE YOU MUST DISABLE DEBUGGING IN CONFIG.H OF Arduino-lmic

//Defining DEBUG removes the attachInterupt for the rain gauge to allow it to compile small enough.
//#define DEBUG
#ifdef DEBUG
 #define DEBUG_PRINTLN(x)  Serial.println(x)
 #define DEBUG_PRINT(x)  Serial.print(x)
#else
 #define DEBUG_PRINTLN(x)
 #define DEBUG_PRINT(x)
#endif

/***I use a packet decoder in TTN
function Decoder(bytes, port) {
 var decoded = {};

 // Decode bytes to int
 var celciusInt = (bytes[0] << 8) | bytes[1];
 var pressureInt = (bytes[2] << 16) | bytes[3] << 8 | bytes[4];
 var humidityInt = (bytes[5] << 8) | bytes[6];
 // Decode int to float
 decoded.celcius = celciusInt / 100;
 decoded.pressure = pressureInt / 100;
 decoded.humidity = humidityInt / 100;
 decoded.rain = bytes[7] * 0.2794;
 
 return decoded;
}
 */

/*******************************************************************************
 * Copyright (c) 2015 Thomas Telkamp and Matthijs Kooijman
 *
 * Permission is hereby granted, free of charge, to anyone
 * obtaining a copy of this document and accompanying files,
 * to do whatever they want with them without any restriction,
 * including, but not limited to, copying, modification and redistribution.
 * NO WARRANTY OF ANY KIND IS PROVIDED.
 *
 * This example sends a valid LoRaWAN packet with payload "Hello,
 * world!", using frequency and encryption settings matching those of
 * the The Things Network.
 *
 * This uses OTAA (Over-the-air activation), where where a DevEUI and
 * application key is configured, which are used in an over-the-air
 * activation procedure where a DevAddr and session keys are
 * assigned/generated for use with all further communication.
 *
 * Note: LoRaWAN per sub-band duty-cycle limitation is enforced (1% in
 * g1, 0.1% in g2), but not the TTN fair usage policy (which is probably
 * violated by this sketch when left running for longer)!

 * To use this sketch, first register your application and device with
 * the things network, to set or generate an AppEUI, DevEUI and AppKey.
 * Multiple devices can use the same AppEUI, but each device has its own
 * DevEUI and AppKey.
 *
 * Do not forget to define the radio type correctly in config.h.
 *
 *******************************************************************************/
// https://github.com/matthijskooijman/arduino-lmic

#include <lmic.h>
#include <hal/hal.h>
#include <Wire.h>
#include "secrets.h" // moved secret keys referenced below to here
#include <avr/wdt.h>


// Schedule TX every this many seconds (might become longer due to duty
// cycle limitations).
const unsigned TX_INTERVAL = 300; //5mins
#define PACKET_SIZE 8
byte mydata[PACKET_SIZE] = {0};

#define BME280_ADDRESS 0x77
#define RAIN_GUAGE_PIN 7
#define LED_PIN 13
static osjob_t sendjob;


void setup() 
{
  wdt_disable();
  
#ifdef DEBUG
  Serial.begin(115200);
  while (!Serial);
#else
  pinMode(RAIN_GUAGE_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(RAIN_GUAGE_PIN), rainGaugeISR, FALLING);
#endif

  DEBUG_PRINTLN(F("Starting WIO"));
  pinMode(LED_PIN, OUTPUT);

  Wire.begin();
  bmeInit();

  // LMIC init
  os_init();
  // Reset the MAC state. Session and pending data transfers will be discarded.
  LMIC_reset();

  LMIC_setClockError(MAX_CLOCK_ERROR * 1 / 100);

  wdt_enable(WDTO_8S);
  
  // Start job (sending automatically starts OTAA too)
  do_send(&sendjob);
}

void loop() 
{
  wdt_reset();
  os_runloop_once();
}

void buildDataPacket()
{
  int16_t temperature; 
  uint32_t pressure;
  uint16_t humidity;
  getBMEData(&temperature, &pressure, &humidity);
  mydata[0] = temperature >> 8;
  mydata[1] = temperature & 0xff;
  mydata[2] = pressure >> 16;
  mydata[3] = (pressure >> 8) & 0xff;
  mydata[4] = pressure & 0xff;
  mydata[5] = humidity >> 8;
  mydata[6] = humidity & 0xff;
  mydata[7] = getRainCount();
}



