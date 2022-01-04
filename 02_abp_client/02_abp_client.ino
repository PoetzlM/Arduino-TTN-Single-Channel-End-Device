/*******************************************************************************
   Copyright (c) 2015 Thomas Telkamp and Matthijs Kooijman
   Copyright (c) 2018 Terry Moore, MCCI

   Permission is hereby granted, free of charge, to anyone
   obtaining a copy of this document and accompanying files,
   to do whatever they want with them without any restriction,
   including, but not limited to, copying, modification and redistribution.
   NO WARRANTY OF ANY KIND IS PROVIDED.

   This example sends a valid LoRaWAN packet with payload "Hello,
   world!", using frequency and encryption settings matching those of
   the The Things Network.

   This uses ABP (Activation-by-personalisation), where a DevAddr and
   Session keys are preconfigured (unlike OTAA, where a DevEUI and
   application key is configured, while the DevAddr and session keys are
   assigned/generated in the over-the-air-activation procedure).

   Note: LoRaWAN per sub-band duty-cycle limitation is enforced (1% in
   g1, 0.1% in g2), but not the TTN fair usage policy (which is probably
   violated by this sketch when left running for longer)!

   To use this sketch, first register your application and device with
   the things network, to set or generate a DevAddr, NwkSKey and
   AppSKey. Each device should have their own unique values for these
   fields.

   Do not forget to define the radio type correctly in
   arduino-lmic/project_config/lmic_project_config.h or from your BOARDS.txt.

 *******************************************************************************/

// References:
// [feather] adafruit-feather-m0-radio-with-lora-module.pdf

#include <lmic.h>
#include <hal/hal.h>
#include <SPI.h>

//
// For normal use, we require that you edit the sketch to replace FILLMEIN
// with values assigned by the TTN console. However, for regression tests,
// we want to be able to compile these scripts. The regression tests define
// COMPILE_REGRESSION_TEST, and in that case we define FILLMEIN to a non-
// working but innocuous value.
//
#ifdef COMPILE_REGRESSION_TEST
# define FILLMEIN 0
#else
# warning "You must replace the values marked FILLMEIN with real values from the TTN control panel!"
# define FILLMEIN (#dont edit this, edit the lines that use FILLMEIN)
#endif

/*
  // LoRaWAN NwkSKey, network session key
  // This should be in big-endian (aka msb).
  static const PROGMEM u1_t NWKSKEY[16] = { FILLMEIN };

  // LoRaWAN AppSKey, application session key
  // This should also be in big-endian (aka msb).
  static const u1_t PROGMEM APPSKEY[16] = { FILLMEIN };

  // LoRaWAN end-device address (DevAddr)
  // See http://thethingsnetwork.org/wiki/AddressSpace
  // The library converts the address to network byte order as needed, so this should be in big-endian (aka msb) too.
  static const u4_t DEVADDR = FILLMEIN ; // <-- Change this address for every node!
*/

// login data to an application of the TTN
static const PROGMEM u1_t NWKSKEY[16] = {XXXXXX};
static const u1_t PROGMEM APPSKEY[16] = {XXXXXX};
static const u4_t DEVADDR = XXXXXX; // <-- Change this address for every node!


// These callbacks are only used in over-the-air activation, so they are
// left empty here (we cannot leave them out completely unless
// DISABLE_JOIN is set in arduino-lmic/project_config/lmic_project_config.h,
// otherwise the linker will complain).
void os_getArtEui (u1_t* buf) { }
void os_getDevEui (u1_t* buf) { }
void os_getDevKey (u1_t* buf) { }

static uint8_t mydata[] = "ABP-Example, Hello, world!";
static osjob_t sendjob;

// Schedule TX every this many seconds (might become longer due to duty
// cycle limitations).
const unsigned TX_INTERVAL = 60;


#define TTN_PIN_SPI_SCK 18
#define TTN_PIN_SPI_MOSI 23
#define TTN_PIN_SPI_MISO 19
#define TTN_PIN_NSS 5
#define TTN_PIN_RXTX TTN_NOT_CONNECTED
#define TTN_PIN_RST 14
#define TTN_PIN_DIO0 12
#define TTN_PIN_DIO1 13

const lmic_pinmap lmic_pins = {
  .nss = TTN_PIN_NSS,
  .rxtx = LMIC_UNUSED_PIN,
  .rst = TTN_PIN_RST,
  .dio = {TTN_PIN_DIO0, TTN_PIN_DIO1, LMIC_UNUSED_PIN},
};

void onEvent (ev_t ev) {
  Serial.print(os_getTime());
  Serial.print(": ");
  switch (ev) {
    case EV_SCAN_TIMEOUT:
      Serial.println(F("EV_SCAN_TIMEOUT"));
      break;
    case EV_BEACON_FOUND:
      Serial.println(F("EV_BEACON_FOUND"));
      break;
    case EV_BEACON_MISSED:
      Serial.println(F("EV_BEACON_MISSED"));
      break;
    case EV_BEACON_TRACKED:
      Serial.println(F("EV_BEACON_TRACKED"));
      break;
    case EV_JOINING:
      Serial.println(F("EV_JOINING"));
      break;
    case EV_JOINED:
      Serial.println(F("EV_JOINED"));

      LMIC_disableChannel(1);
      LMIC_disableChannel(2);
      LMIC_disableChannel(3);
      LMIC_disableChannel(4);
      LMIC_disableChannel(5);
      LMIC_disableChannel(6);
      LMIC_disableChannel(7);
      LMIC_disableChannel(8);

      LMIC_setLinkCheckMode(0);

      break;
    /*
      || This event is defined but not used in the code. No
      || point in wasting codespace on it.
      ||
      || case EV_RFU1:
      ||     Serial.println(F("EV_RFU1"));
      ||     break;
    */
    case EV_JOIN_FAILED:
      Serial.println(F("EV_JOIN_FAILED"));
      break;
    case EV_REJOIN_FAILED:
      Serial.println(F("EV_REJOIN_FAILED"));
      break;
    case EV_TXCOMPLETE:
      Serial.println(F("EV_TXCOMPLETE (includes waiting for RX windows)"));
      if (LMIC.txrxFlags & TXRX_ACK)
        Serial.println(F("Received ack"));
      if (LMIC.dataLen) {
        Serial.println(F("Received "));
        Serial.println(LMIC.dataLen);
        Serial.println(F(" bytes of payload"));
      }
      // Schedule next transmission
      os_setTimedCallback(&sendjob, os_getTime() + sec2osticks(TX_INTERVAL), do_send);

      //nach jeder Receive ack, können neue Frequenztabellen kommen,
      //da wir immer nur auf einer Frequenz bleibenm wollen müssen wir diese wieder erneut ausschalten

      LMIC_disableChannel(1);
      LMIC_disableChannel(2);
      LMIC_disableChannel(3);
      LMIC_disableChannel(4);
      LMIC_disableChannel(5);
      LMIC_disableChannel(6);
      LMIC_disableChannel(7);
      LMIC_disableChannel(8);

      LMIC_setLinkCheckMode(0);


      break;
    case EV_LOST_TSYNC:
      Serial.println(F("EV_LOST_TSYNC"));
      break;
    case EV_RESET:
      Serial.println(F("EV_RESET"));
      break;
    case EV_RXCOMPLETE:
      // data received in ping slot
      Serial.println(F("EV_RXCOMPLETE"));
      break;
    case EV_LINK_DEAD:
      Serial.println(F("EV_LINK_DEAD"));
      break;
    case EV_LINK_ALIVE:
      Serial.println(F("EV_LINK_ALIVE"));
      break;
    /*
      || This event is defined but not used in the code. No
      || point in wasting codespace on it.
      ||
      || case EV_SCAN_FOUND:
      ||    Serial.println(F("EV_SCAN_FOUND"));
      ||    break;
    */
    case EV_TXSTART:
      Serial.println(F("EV_TXSTART"));
      break;
    case EV_TXCANCELED:
      Serial.println(F("EV_TXCANCELED"));
      break;
    case EV_RXSTART:
      /* do not print anything -- it wrecks timing */
      break;
    case EV_JOIN_TXCOMPLETE:
      Serial.println(F("EV_JOIN_TXCOMPLETE: no JoinAccept"));
      break;
    default:
      Serial.print(F("Unknown event: "));
      Serial.println((unsigned) ev);
      break;
  }
}

void do_send(osjob_t* j) {
  // Check if there is not a current TX/RX job running
  if (LMIC.opmode & OP_TXRXPEND) {
    Serial.println(F("OP_TXRXPEND, not sending"));
  } else {
    // Prepare upstream data transmission at the next possible time.
    LMIC_setTxData2(1, mydata, sizeof(mydata) - 1, 0);
    Serial.println(F("Packet queued"));
  }
  // Next TX is scheduled after TX_COMPLETE event.
}

void setup() {

  //spi
  SPI.begin(TTN_PIN_SPI_SCK, TTN_PIN_SPI_MISO, TTN_PIN_SPI_MOSI, TTN_PIN_NSS);

  //    pinMode(13, OUTPUT);
  while (!Serial); // wait for Serial to be initialized
  Serial.begin(115200);
  delay(100);     // per sample code on RF_95 test
  Serial.println(F("Starting"));

  // LMIC init
  os_init();
  // Reset the MAC state. Session and pending data transfers will be discarded.
  LMIC_reset();
  //LMIC_setClockError(MAX_CLOCK_ERROR * 2 / 100);
  LMIC_setClockError(65535);

  // If not running an AVR with PROGMEM, just use the arrays directly
  // missing typecast fixed...
  LMIC_setSession (0x13, DEVADDR, (xref2u1_t)NWKSKEY, (xref2u1_t)APPSKEY);

  //frequenzbänder müssen manuell aufgesetzt werden, da die join funktion nicht aufgerufen wird
  //sollten 8 bänder sein, da EU868 8 bänder erwartet

  LMIC_setupChannel(0, 868300000, DR_RANGE_MAP(DR_SF7, DR_SF7),  BAND_CENTI);      // g-band
  LMIC_setupChannel(1, 868300000, DR_RANGE_MAP(DR_SF7, DR_SF7), BAND_CENTI);      // g-band
  LMIC_setupChannel(2, 868300000, DR_RANGE_MAP(DR_SF7, DR_SF7),  BAND_CENTI);      // g-band
  LMIC_setupChannel(3, 868300000, DR_RANGE_MAP(DR_SF7, DR_SF7),  BAND_CENTI);      // g-band
  LMIC_setupChannel(4, 868300000, DR_RANGE_MAP(DR_SF7, DR_SF7),  BAND_CENTI);      // g-band
  LMIC_setupChannel(5, 868300000, DR_RANGE_MAP(DR_SF7, DR_SF7),  BAND_CENTI);      // g-band
  LMIC_setupChannel(6, 868300000, DR_RANGE_MAP(DR_SF7, DR_SF7),  BAND_CENTI);      // g-band
  LMIC_setupChannel(7, 868300000, DR_RANGE_MAP(DR_SF7, DR_SF7),  BAND_CENTI);      // g-band
  LMIC_setupChannel(8, 868300000, DR_RANGE_MAP(DR_SF7,  DR_SF7),  BAND_MILLI);      // g2-band

  LMIC_disableChannel(1);
  LMIC_disableChannel(2);
  LMIC_disableChannel(3);
  LMIC_disableChannel(4);
  LMIC_disableChannel(5);
  LMIC_disableChannel(6);
  LMIC_disableChannel(7);
  LMIC_disableChannel(8);

  // Disable link check validation
  LMIC_setLinkCheckMode(0);

  // TTN uses SF9 for its RX2 window.
  LMIC.dn2Dr = DR_SF9;

  // Set data rate and transmit power for uplink
  LMIC_setDrTxpow(DR_SF7, 14);

  // Start job
  do_send(&sendjob);
}

void loop() {
  unsigned long now;
  now = millis();
  if ((now & 512) != 0) {
    digitalWrite(13, HIGH);
  }
  else {
    digitalWrite(13, LOW);
  }

  os_runloop_once();

}
