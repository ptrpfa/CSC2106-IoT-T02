/*
    Current implementation here does not involve sending JSON message
*/

#include "painlessMesh.h"
#include <M5StickCPlus.h>
#include <Arduino_JSON.h>

#define   MESH_PREFIX     "myMesh"
#define   MESH_PASSWORD   "password"
#define   MESH_PORT       5555
#define   MAINNODE        "A"
#define   NODE            "B"

Scheduler userScheduler;
painlessMesh  mesh;

int count = 0;
bool mainNodeSet = false;
uint32_t mainNode;
String readings;

void sendMessage();
void sendMainNode();

Task taskSendMessage( TASK_SECOND * 1 , TASK_FOREVER, &sendMessage );
Task taskSendMainNode( TASK_SECOND * 1, TASK_FOREVER, &sendMainNode );

void sendMessage() {
  String msg = getReadings();
  mesh.sendBroadcast( msg );
  taskSendMessage.setInterval( random( TASK_SECOND * 1, TASK_SECOND * 5 ));
}

void sendMainNode() {

  if ( mainNodeSet ) {

    String msg = "Hello Main Node from ";
    msg += NODE;
    mesh.sendSingle(mainNode, msg);
  
  }

}

// Needed for painless library
void receivedCallback( uint32_t from, String &msg ) {

  Serial.printf("startHere: Received from %u msg=%s\n", from, msg.c_str());

  if (!mainNodeSet) {
    if (msg.startsWith(MAINNODE)) {
      mainNode = from;
      Serial.printf("Main node identified: %u\n", mainNode);
      M5.Lcd.printf("Main node identified: %u\n", mainNode);
      mainNodeSet = true;
    }
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

void setup() {

  Serial.begin(115200);
  M5.begin();
  int x = M5.IMU.Init();
  if(x!=0)
    Serial.printf("IMU initialisation fail!");  

  M5.Lcd.setTextSize(1);
  delay(3000);
//mesh.setDebugMsgTypes( ERROR | MESH_STATUS | CONNECTION | SYNC | COMMUNICATION | GENERAL | MSG_TYPES | REMOTE ); // all types on
  mesh.setDebugMsgTypes( ERROR | STARTUP );  // set before init() so that you can see startup messages

  mesh.init( MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT );
  mesh.onReceive(&receivedCallback);
  mesh.onNewConnection(&newConnectionCallback);
  mesh.onChangedConnections(&changedConnectionCallback);
  mesh.onNodeTimeAdjusted(&nodeTimeAdjustedCallback);

  Serial.println("Starting node");
  Serial.println(mesh.getNodeId());

  M5.Lcd.setRotation(3);
  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.setCursor(0, 0, 2);
  M5.Lcd.printf("STARTING NODE %s\n", NODE);

  userScheduler.addTask( taskSendMainNode );
  taskSendMainNode.enable();
}

void loop() {
  // it will run the user scheduler as well
  mesh.update();
}

void clearDisplay() {

  M5.Lcd.fillRect(0, 20, M5.Lcd.width(), M5.Lcd.height() - 20, BLACK);
  M5.Lcd.setCursor(0, 20, 2);

}

String getReadings () {
  JSONVar jsonReadings;
  jsonReadings["node"] = NODE;
  jsonReadings["temp"] = M5.Axp.GetTempData()*0.1-144.7;
  readings = JSON.stringify(jsonReadings);
  return readings;
}