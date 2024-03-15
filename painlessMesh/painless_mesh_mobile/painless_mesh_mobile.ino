#include "painlessMesh.h"
#include <M5StickCPlus.h>
#include <Arduino_JSON.h>
#include <cmath>
#include <Triangle.h>
#include <vector>

#define MESH_PREFIX "myMesh"
#define MESH_PASSWORD "password"
#define MESH_PORT 5555
#define MAINNODE "A"
#define NODE "B"

Scheduler userScheduler;
painlessMesh mesh;

int count = 0;
bool mainNodeSet = false;
uint32_t mainNode;
String readings;

void sendMessage();
void sendMainNode();
void estimateLocation();

struct NodeInfo {
  double x;
  double y;
  String macAddress;
  uint32_t nodeId;
};

std::vector<NodeInfo> nodeList;

Task taskSendMessage(TASK_SECOND * 1, TASK_FOREVER, &sendMessage);
// Task taskSendMainNode(TASK_SECOND * 1, TASK_FOREVER, &sendMainNode);
Task taskEstimateLocation(TASK_SECOND * 10, TASK_FOREVER, &estimateLocation);


int findNodeByMAC(const String& macAddress) {
  for (size_t i = 0; i < nodeList.size(); i++) {
    if (nodeList[i].macAddress == macAddress) {
      return i;
    }
  }
  return -1;
}

void estimateLocation() {
  // Get all RSSI
  std::vector<std::pair<String, int>> rssiVector;

  int n = WiFi.scanNetworks();
  Serial.println("Scan done");
  if (n == 0) {
    Serial.println("No networks found");
    return;
  } else {
    for (int i = 0; i < n; ++i) {
      // Store BSSID and RSSI for each network found
      String msg;
      if (WiFi.SSID(i) == "myMesh") {
        rssiVector.push_back(std::make_pair(WiFi.BSSIDstr(i), WiFi.RSSI(i)));
      }
    }
  }
  WiFi.scanDelete();

  if (rssiVector.size() < 3) {
    Serial.println("Insufficient beacons found");
    return;
  }
  
  // Get nearest 3 mac addresses
  std::sort(rssiVector.begin(), rssiVector.end(), [](const std::pair<String, int>& a, const std::pair<String, int>& b) {
    return a.second > b.second;
  });

  JSONVar nearestThree;
  int count = 0;
  for (auto& pair : rssiVector) {
    if (count++ >= 3) break;
    nearestThree[pair.first] = pair.second;
  }

  // Compare nodeList to get three coordinates
  std::vector<Point> topThreeCoordinates;

  // Iterate through the top three RSSI values
  for (int i = 0; i < 3; i++) {
    // Extract the MAC address from the nearestThree JSONVar
    String targetMac = (const char*)nearestThree.keys()[i];

    // Convert RSSI to distances
    // Constants
    double n = 4.0;
    double A = -70;

    // Calculate
    double distance = pow(10, ((A - (double)nearestThree[targetMac]) / (10 * n)));

    nearestThree[i] = distance;

    // Find MAC address in nodeList
    for (auto& node : nodeList) {
      if (node.macAddress == targetMac) {
        // Add the node's coordinates to topThreeCoordinates
        topThreeCoordinates.push_back(Point(node.x, node.y));
        break;  // Stop searching through nodeList once a match is found
      }
    }
  }

  // Get triangulation
  Triangle triangle = Triangle(topThreeCoordinates[0], topThreeCoordinates[1], topThreeCoordinates[2]);
  Point estimated_point = triangle.getTriangulation(nearestThree[0], nearestThree[1], nearestThree[2]);

  double x = estimated_point.getX();  // To get estimated x coordinate
  double y = estimated_point.getX();  // To get estimated y coordinate

  // Send to main node
  JSONVar data;
  data["x"] = x;
  data["y"] = y;
  data["macAddress"] = WiFi.macAddress().c_str();
  data["nodeID"] = (int)mesh.getNodeId();

  String output = JSON.stringify(data);
   mesh.sendSingle(mainNode, output);
}

void sendMessage() {
  String msg = getReadings();
  mesh.sendBroadcast(msg);
  taskSendMessage.setInterval(random(TASK_SECOND * 1, TASK_SECOND * 5));
}

void sendMainNode() {
  if (mainNodeSet) {
    int n = WiFi.scanNetworks();
    Serial.println("Scan done");
    if (n == 0) {
      Serial.println("No networks found");
    } else {
      for (int i = 0; i < n; ++i) {
        // Print SSID and RSSI for each network found
        String msg;
        if (WiFi.SSID(i) == "myMesh") {
          M5.Lcd.print("MAC Address: ");
          M5.Lcd.print(WiFi.BSSIDstr(i));
          M5.Lcd.print(" ; RSSI: ");
          M5.Lcd.printf(" ; RSSI: (%d dBm)\n", WiFi.RSSI(i));

          Serial.printf("BSSID: ");
          Serial.print(WiFi.BSSIDstr(i));
          Serial.printf(" RSSI: ");
          Serial.print(WiFi.RSSI(i));
          Serial.printf(" (dBm)\n");
        }
      }
    }
    WiFi.scanDelete();

    String msg = "Hello Main Node from ";
    msg += NODE;
    msg += count;
    count++;
    mesh.sendSingle(mainNode, msg);
  }
}

// Needed for painless library
void receivedCallback(uint32_t from, String& msg) {

  Serial.printf("startHere: Received from %u msg=%s\n", from, msg.c_str());

  if (!mainNodeSet) {
    if (msg.startsWith(MAINNODE)) {
      mainNode = from;
      Serial.printf("Main node identified: %u\n", mainNode);
      M5.Lcd.printf("Main node identified: %u\n", mainNode);
      mainNodeSet = true;
    }
  }

  JSONVar newInfo = JSON.parse(msg);

  // Check if parsing succeeded
  if (JSON.typeof(newInfo) == "undefined") {
    Serial.println("Not a JSON");
    return;
  }

  if ((const char*)newInfo["type"] == "BEACON") {
    String macAddress = (const char*)newInfo["macAddress"];
    int index = findNodeByMAC(macAddress);

    if (index != -1) {
      // Node is already in nodeList, update its information
      nodeList[index].x = (double)newInfo["x"];
      nodeList[index].y = (double)newInfo["y"];
      nodeList[index].nodeId = (int)newInfo["nodeID"];
    } else {
      // This is a new node, add it to nodeList
      NodeInfo newNode;
      newNode.x = (double)newInfo["x"];
      newNode.y = (double)newInfo["y"];
      newNode.macAddress = macAddress;
      newNode.nodeId = (int)newInfo["nodeID"];
      nodeList.push_back(newNode);
    }
  }
}

void newConnectionCallback(uint32_t nodeId) {
  // Doesnt matter
  Serial.printf("--> startHere: New Connection, nodeId = %u\n", nodeId);
}

void changedConnectionCallback() {
  Serial.printf("Changed connections\n");
}

void nodeTimeAdjustedCallback(int32_t offset) {
  Serial.printf("Adjusted time %u. Offset = %d\n", mesh.getNodeTime(), offset);
}

void initMesh() {
  //mesh.setDebugMsgTypes( ERROR | MESH_STATUS | CONNECTION | SYNC | COMMUNICATION | GENERAL | MSG_TYPES | REMOTE ); // all types on
  mesh.setDebugMsgTypes(ERROR | STARTUP);  // set before init() so that you can see startup messages

  mesh.init(MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT);
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

  // userScheduler.addTask(taskSendMainNode);
  userScheduler.addTask(taskEstimateLocation);
  taskEstimateLocation.enable();
}

void setup() {

  Serial.begin(115200);
  M5.begin();
  int x = M5.IMU.Init();
  if (x != 0)
    Serial.printf("IMU initialisation fail!");

  M5.Lcd.setTextSize(1);
  delay(2000);
  initMesh();
}

void loop() {
  // it will run the user scheduler as well
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