/*
    Current implementation here does not involve sending JSON message
*/

#include "painlessMesh.h"
#include <M5StickCPlus.h>
#include <Arduino_JSON.h>
#include <Preferences.h>

#define MESH_PREFIX "myMesh"
#define MESH_PASSWORD "password"
#define MESH_PORT 5555
#define MAINNODE "A"
#define NODE "B"
#define TYPE "BEACON"

Scheduler userScheduler;
Preferences preferences;
painlessMesh mesh;

bool mainNodeSet = false;
bool isFirstConnection = true;
uint32_t mainNode;
String readings;

void sendMessage();
void sendMainNode();

Task taskSendMessage(TASK_SECOND * 5, TASK_FOREVER, &sendMessage);
Task taskSendMainNode(TASK_SECOND * 5, TASK_FOREVER, &sendMainNode);

struct NodeInfo {
  double x;
  double y;
  String macAddress;
  String type;
  uint32_t nodeId;

  String toJSONString() {
    JSONVar node;
    node["x"] = x;
    node["y"] = y;
    node["macAddress"] = macAddress.c_str();
    node["type"] = type.c_str();
    node["nodeID"] = (int)nodeId;

    String output = JSON.stringify(node);
    return output;
  }
};

NodeInfo myInfo;

void sendMessage() {
  String msg = getReadings();
  mesh.sendBroadcast(msg);
  // taskSendMessage.setInterval( random( TASK_SECOND * 1, TASK_SECOND * 5 ));
}

void sendMainNode() {
  if (mainNodeSet) {
    // Retrieve self coords
    preferences.begin("coords", true);
    double x = preferences.getDouble("x", 0.0);
    double y = preferences.getDouble("y", 0.0);
    preferences.end();

    String msg = "Hello Main Node from Beacon (";
    msg += x;
    msg += ", ";
    msg += y;
    msg += ")";
    mesh.sendSingle(mainNode, msg);
  }
}

// Receveied message callback
void receivedCallback(uint32_t from, String &msg) {

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
  // Send own data to new node
  mesh.sendSingle(nodeId, myInfo.toJSONString());

  Serial.printf("--> startHere: New Connection, nodeId = %u\n", nodeId);
}

void changedConnectionCallback() {
  if (isFirstConnection) {
    // Initial connection
    isFirstConnection = false;

    // Broadcast my info
    mesh.sendBroadcast(myInfo.toJSONString());
  }
  Serial.printf("Changed connections\n");
}

void nodeTimeAdjustedCallback(int32_t offset) {
  Serial.printf("Adjusted time %u. Offset = %d\n", mesh.getNodeTime(), offset);
}

void storeCoordinates(double x, double y) {
  preferences.begin("coords", false);
  preferences.putDouble("x", x);
  preferences.putDouble("y", y);
  preferences.end();
}

void initMesh() {
  mesh.setDebugMsgTypes(ERROR | STARTUP);  // set before init() so that you can see startup messages

  mesh.init(MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT);
  mesh.onReceive(&receivedCallback);
  mesh.onNewConnection(&newConnectionCallback);
  mesh.onChangedConnections(&changedConnectionCallback);
  mesh.onNodeTimeAdjusted(&nodeTimeAdjustedCallback);

  Serial.println("Starting node");
  Serial.println(mesh.getNodeId());

  M5.Lcd.printf("STARTING NODE %s\n", NODE);
}

void initNodeObject() {
  preferences.begin("coords", true);

  // Load data from preferences into myInfo
  myInfo.x = preferences.getDouble("x", 0.0);
  myInfo.y = preferences.getDouble("y", 0.0);
  preferences.end();  // Close the preferences

  myInfo.macAddress = WiFi.macAddress();
  myInfo.type = TYPE;
  myInfo.nodeId = mesh.getNodeId();

  // Use the data for whatever you need, for example, print the JSON string
  Serial.println(myInfo.toJSONString());
}

void setup() {
  Serial.begin(115200);
  M5.begin();
  int x = M5.IMU.Init();
  // Anchor 1
  double x = 2.0;
  double y = 5.0;

  // Anchor 2
  // double x = 5.0;
  // double y = 3.0;

  // Anchor 3
  // double x = 3.0;
  // double y = 1.0;

  storeCoordinates(x, y);

  if (x != 0)
    Serial.printf("IMU initialisation fail!");

  M5.Lcd.setTextSize(1);
  M5.Lcd.setRotation(3);
  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.setCursor(0, 0, 2);
  
  initMesh();
  delay(1500);
  initNodeObject();
  M5.Lcd.setCursor(0, 2, 2);
  delay(1500);
  userScheduler.addTask(taskSendMainNode);
  taskSendMainNode.enable();
}

void loop() {
  // It will run the user scheduler as well
  mesh.update();
}

void clearDisplay() {
  M5.Lcd.fillRect(0, 20, M5.Lcd.width(), M5.Lcd.height() - 20, BLACK);
  M5.Lcd.setCursor(0, 20, 2);
}

String getReadings() {
  JSONVar jsonReadings;
  jsonReadings["node"] = NODE;
  jsonReadings["temp"] = M5.Axp.GetTempData() * 0.1 - 144.7;
  readings = JSON.stringify(jsonReadings);
  return readings;
}