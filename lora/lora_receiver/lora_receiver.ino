#include <RadioLib.h>
#include "boards.h"

SX1280 radio = new Module(RADIO_CS_PIN, RADIO_DIO1_PIN, RADIO_RST_PIN, RADIO_BUSY_PIN);

// flag to indicate that a packet was received
volatile bool receivedFlag = true;

// disable interrupt when it's not needed
volatile bool enableReceiveInterrupt = true;

// Function to initialise LoRa
int initLora() {
  // initialize SX1280 with default settings
  printToDisplay("[SX1280] Initialising ...", u8g2);
  int state = radio.begin();
  if (state != RADIOLIB_ERR_NONE) {
    printToDisplay("Failed Initialisation!", u8g2);
  }
  else {
    printToDisplay("Radio initialised!", u8g2);
  }

  #if defined(RADIO_RX_PIN) && defined(RADIO_TX_PIN)
      //Set ANT Control pins
      radio.setRfSwitchPins(RADIO_RX_PIN, RADIO_TX_PIN);
  #endif

  // T3 S3 V1.2 (No PA) Version Set output power to 3 dBm (Cannot be greater than 3dbm)
  int8_t TX_Power = 13;
  if (radio.setOutputPower(TX_Power) == RADIOLIB_ERR_INVALID_OUTPUT_POWER) {
      Serial.println(F("Selected output power is invalid for this module!"));
      while (true);
  }

  // set carrier frequency to 2410.5 MHz
  if (radio.setFrequency(2400.0) == RADIOLIB_ERR_INVALID_FREQUENCY) {
      Serial.println(F("Selected frequency is invalid for this module!"));
      while (true);
  }

  // set bandwidth to 203.125 kHz
  if (radio.setBandwidth(203.125) == RADIOLIB_ERR_INVALID_BANDWIDTH) {
      Serial.println(F("Selected bandwidth is invalid for this module!"));
      while (true);
  }

  // set spreading factor to 10
  if (radio.setSpreadingFactor(10) == RADIOLIB_ERR_INVALID_SPREADING_FACTOR) {
      Serial.println(F("Selected spreading factor is invalid for this module!"));
      while (true);
  }

  // set coding rate to 6
  if (radio.setCodingRate(6) == RADIOLIB_ERR_INVALID_CODING_RATE) {
      Serial.println(F("Selected coding rate is invalid for this module!"));
      while (true);
  }
  return state;
}

// Callback function once a complete packet is received (this function MUST be 'void' type and MUST NOT have any arguments!)
void loraReceiveCallback(void) {
  // check if the interrupt is enabled
  if (!enableReceiveInterrupt) {
      return;
  }
  else {
    // we got a packet, set the flag
    receivedFlag = true;
  }
}

void setup() {
  // Initialise board
  initBoard();

  // When the power is turned on, a delay is required.
  delay(1500);
  
  // Initialise LoRa
  int state = initLora();

  // set the function that will be called when packet transmission is finished
  radio.setDio1Action(loraReceiveCallback);
}

void loop()
{   
  /* 
    This example listens for LoRa transmissions and tries to
    receive them. Once a packet is received, an interrupt is
    triggered. To successfully receive data, the following
    settings have to be the same on both transmitter
    and receiver:
      - carrier frequency
      - bandwidth
      - spreading factor
      - coding rate
      - sync word
    */

  // check if the flag is set
  if (receivedFlag) {
    // disable the interrupt service routine while processing the data
    enableReceiveInterrupt = false;

    // reset flag
    receivedFlag = false;

    // you can read received data as an Arduino String
    String str;
    int state = radio.readData(str);

    // uint32_t counter;
    // int state = radio.readData((uint8_t *)&counter, 4);

    // you can also read received data as byte array
    /*
      byte byteArr[8];
      int state = radio.readData(byteArr, 8);
    */

    if (state == RADIOLIB_ERR_NONE) {
      // packet was successfully received
      u8g2->clearBuffer();
      char buf[256];
      u8g2->drawStr(0, 12, "Received OK (NORMAL!");
      // snprintf(buf, sizeof(buf), "Data:%s", counter);
      snprintf(buf, sizeof(buf), "Data:%s", str);
      u8g2->drawStr(0, 26, buf);
      // print RSSI (Received Signal Strength Indicator)
      snprintf(buf, sizeof(buf), "RSSI:%.2f", radio.getRSSI());
      u8g2->drawStr(0, 40, buf);
      // print SNR (Signal-to-Noise Ratio)
      snprintf(buf, sizeof(buf), "SNR:%.2f dB", radio.getSNR());
      u8g2->drawStr(0, 54, buf);
      u8g2->sendBuffer();
    } 
    
    else if (state == RADIOLIB_ERR_CRC_MISMATCH) {
      // packet was received, but is malformed
      Serial.println(F("[SX1280] CRC error!"));
    } 
    else {
      // some other error occurred
      Serial.print(F("[SX1280] Failed, code "));
      Serial.println(state);
    }

    // put module back to listen mode
    radio.startReceive();
  
    // we're ready to receive more packets, enable interrupt service routine
    enableReceiveInterrupt = true;
  }
}