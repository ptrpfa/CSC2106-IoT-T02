#include "painlessMesh.h"
#include "SSD1306Wire.h"
#include <Arduino_JSON.h>
#include <RadioLib.h>
#include <WiFi.h>
#include "boards.h"

/* painlessMesh */
#define   MESH_PREFIX     "myMesh"
#define   MESH_PASSWORD   "password"
#define   MESH_PORT       5555
#define   MAINNODE        "A"
#define   NODE            "A"

/*  */
Scheduler userScheduler;
painlessMesh  mesh;

SSD1306Wire display(0x3c, 18, 17);

int count = 0;

void announceNodeId();

Task taskAnnounceNodeId( TASK_SECOND * 1, TASK_FOREVER, &announceNodeId );

/* LORA */
SX1280 radio = new Module(RADIO_CS_PIN, RADIO_DIO1_PIN, RADIO_RST_PIN, RADIO_BUSY_PIN);

// save transmission state between loops
int transmissionState = RADIOLIB_ERR_NONE;

// flag to indicate that a packet was sent
volatile bool transmittedFlag = true;

// disable interrupt when it's not needed
volatile bool enableTransmitInterrupt = true;

uint32_t counter = 0;

void announceNodeId() {
  String msg = NODE;
  mesh.sendBroadcast(msg);
  // taskAnnounceNodeId.setInterval( random( TASK_SECOND * 1, TASK_SECOND * 5 ));
}

void pmReceiveCallback( uint32_t from, String &msg ) {
  // Parse message received
  JSONVar message = JSON.parse(msg);

  // Check if parsing succeeded
  if (JSON.typeof(message) == "undefined") {
    Serial.println("Not a JSON");
    return;
  }

  // Only process messages with type of LOCATION (ignore BEACON messages)
  if (strcmp((const char*)message["type"], "LOCATION") == 0) {
    Serial.println("Received new location information!\n");
    Serial.printf("Received from %u msg=%s\n", from, msg.c_str());

    // Send message received to LoRa
    message["loraAddress"] = WiFi.macAddress().c_str();
    message["floor"] = (int)counter;
    String lora_message = JSON.stringify(message);

    // check if the previous transmission finished
    if(transmittedFlag) {
      // disable the interrupt service routine while processing the data
      enableTransmitInterrupt = false;

      // reset flag
      transmittedFlag = false;

      // start transmitting the packet
      transmissionState = radio.startTransmit(lora_message.c_str());

      // we're ready to send more packets, enable interrupt service routine
      enableTransmitInterrupt = true;

      // Display on LCD
      if(count < 4) {
        display.clear();
        display.drawString(0, 0, lora_message.c_str());
        // if (transmissionState == RADIOLIB_ERR_NONE) {
        //   // packet was successfully sent
        //   u8g2->clearBuffer();
        //   u8g2->drawStr(0, 12, "Transmitting..");
        //   u8g2->drawStr(0, 30, ("TX: " + String(counter)).c_str());
        //   u8g2->sendBuffer();
        // } 
        // else {
        //   printToDisplay("transmission failed!", u8g2);
        // }
        display.display();
        count+=1;
      }
      else {
        display.clear(); // This clears the entire display
        display.display(); // Update the display to reflect the clear operation
        count = 0;
      }

    }
  } 
  else {
    Serial.println("Not a LOCATION message!\n");
  }
}

void newConnectionCallback(uint32_t nodeId) {
    Serial.printf("--> startHere: New Connection, nodeId = %u\n", nodeId);
}

void changedConnectionCallback() {
  Serial.printf("Changed connections\n");
}

void nodeTimeAdjustedCallback(int32_t offset) {
    Serial.printf("Adjusted time %u. Offset = %d\n", mesh.getNodeTime(),offset);
}

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

// Callback function once a complete packet is transmitted (this function MUST be 'void' type and MUST NOT have any arguments!)
void loraTransmitCallback(void) {
  // check if the interrupt is enabled
  if (!enableTransmitInterrupt) {
    return;
  }
  else {
    counter++;
    // we sent a packet, set the flag
    transmittedFlag = true;
  }
}

void setup() {
  /* LORA */
  // Initialise board
  initBoard();
  
  // When the power is turned on, a delay is required.
  delay(1500);

  // Initialise LoRa
  int state = initLora();

  // set callback function when packet transmission is finished
  radio.setDio1Action(loraTransmitCallback);

  /* painlessMesh */
  Serial.begin(115200);
  delay(100);
  display.init();
  display.clear();
  display.setFont(ArialMT_Plain_10);
  display.flipScreenVertically();
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  
  delay(3000);

  //mesh.setDebugMsgTypes( ERROR | MESH_STATUS | CONNECTION | SYNC | COMMUNICATION | GENERAL | MSG_TYPES | REMOTE ); // all types on
  mesh.setDebugMsgTypes( ERROR | STARTUP );  // set before init() so that you can see startup messages

  mesh.init( MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT );
  mesh.onReceive(&pmReceiveCallback);
  mesh.onNewConnection(&newConnectionCallback);
  mesh.onChangedConnections(&changedConnectionCallback);
  mesh.onNodeTimeAdjusted(&nodeTimeAdjustedCallback);

  Serial.println("Starting node");
  Serial.println(mesh.getNodeId());

  display.clear();
  display.drawString(0, 0,"STARTING MAIN NODE ");
  display.drawString(0, 10, NODE);
  display.display();

  userScheduler.addTask( taskAnnounceNodeId );
  taskAnnounceNodeId.enable();
}

void loop()
{
  // it will run the user scheduler as well
  mesh.update();
}