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

// XOR encryption key
const uint8_t xorKey = 0b101010;

// Function to encrypt the message using XOR
String xor_encrypt(String message, uint8_t key) {
  String encrypted_message = "";
  for (int i = 0; i < message.length(); i++) {
    encrypted_message += char(message[i] ^ key);
  }
  return encrypted_message;
}


int findNodeByMAC(const String& macAddress) {
  for (size_t i = 0; i < nodeList.size(); i++) {
    if (nodeList[i].macAddress == macAddress) {
      return i;
    }
  }
  return -1;
}

void estimateLocation() {
  // Find MAC address in nodeList
  Serial.printf("ALL NODES IN NODELIST\n");
  int nodeListCount = 0;
  for (auto& node : nodeList) {
    nodeListCount++;
    Serial.println(node.macAddress);
    Serial.printf("x: %lf, y: %lf\n", node.x, node.y);
  }
  if (nodeListCount < 3) {
    Serial.println("Not enough information!\n");
    return;
  }

  Serial.println("Estimating location");
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
        Serial.println("WiFi BSSID: " + WiFi.BSSIDstr(i) + "\n");
        
        rssiVector.push_back(std::make_pair(WiFi.BSSIDstr(i), WiFi.RSSI(i)));
      } 
    }
  }
  WiFi.scanDelete();

  if (rssiVector.size() < 3) {
    Serial.println("Insufficient beacons found");
    return;
  }
  
  Serial.println("At least three beacons found");

  for (const auto& pair : rssiVector) {
    Serial.print("BSSID: ");
    Serial.print(pair.first);
    Serial.print(", RSSI: ");
    Serial.println(pair.second);
  }

  // Get nearest 3 mac addresses
  std::sort(rssiVector.begin(), rssiVector.end(), [](const std::pair<String, int>& a, const std::pair<String, int>& b) {
    return a.second > b.second;
  });
  Serial.println("RSSI sorted");

  double nearestThree[3];
  std::pair<String, int> nearestThreeArray[3];

  int count = 0;
  for (auto& pair : rssiVector) {
    if (count >= 3) break;
    nearestThreeArray[count] = std::make_pair(pair.first, pair.second);

    Serial.printf("Pair %d First: ", count);
    Serial.println(pair.first);
    Serial.printf("Pair %d Second: %d\n", count, pair.second);

    count++;
  }

  // Compare nodeList to get three coordinates
  std::vector<Point> topThreeCoordinates;

  // Iterate through the top three RSSI values
  Serial.println("Calculating distance\n");
  for (int i = 0; i < 3; i++) {
    // Extract the MAC address from the nearestThree JSONVar
    String targetMac = nearestThreeArray[i].first;
    Serial.printf("Getting distance for");
    Serial.println(targetMac);
    // Convert RSSI to distances
    // Constants
    double n = 5.0;
    double A = -70;

    // Calculate
    double distance = pow(10, ((A - (double)nearestThreeArray[i].second) / (10 * n)));
    Serial.printf("Distance #%d: %lf\n", i, distance);

    nearestThree[i] = distance;

    // Find MAC address in nodeList
    String targetMacSubstrFirst = targetMac.substring(0,15);
    String targetMacSubstrSec = targetMac.substring(15,17);
    Serial.println(targetMacSubstrFirst);
    Serial.println(targetMacSubstrSec);
    int hexVal = strtol(targetMacSubstrSec.c_str(), nullptr, 16);
    if (hexVal > 0) {
      hexVal -= 1;
    }
    else {

    }
    char hexStr[3];
    sprintf(hexStr, "%02X",hexVal & 0xFF);
    targetMacSubstrSec = String(hexStr);

    targetMac = targetMacSubstrFirst + targetMacSubstrSec;

    for (auto& node : nodeList) {
      Serial.println(targetMac);

      // String nodeMacAddressSubstr = node.macAddress.substring(0,14);
      // Serial.println(nodeMacAddressSubstr);
      // Serial.println(targetMacSubstr);
      if (strcmp(node.macAddress.c_str(), targetMac.c_str()) == 0) {
        // Add the node's coordinates to topThreeCoordinates
        Serial.println("Found");
        topThreeCoordinates.push_back(Point(node.x, node.y));
        break;  // Stop searching through nodeList once a match is found
      }
    }
  }
  Serial.printf("Points: \n");
  Serial.printf("%lf, %lf \n", topThreeCoordinates[0].getX(), topThreeCoordinates[0].getY());
  Serial.printf("%lf, %lf \n", topThreeCoordinates[1].getX(), topThreeCoordinates[1].getY());
  Serial.printf("%lf, %lf \n", topThreeCoordinates[2].getX(), topThreeCoordinates[2].getY());

  Serial.printf("Getting triangulation data\n");
  // Get triangulation
  Triangle triangle = Triangle(topThreeCoordinates[0], topThreeCoordinates[1], topThreeCoordinates[2]);
  Serial.printf("Made Triangle\n");
  Point estimated_point = triangle.getTriangulation(nearestThree[0], nearestThree[1], nearestThree[2]);
  Serial.printf("Got estimated point\n");

  double x = estimated_point.getX();  // To get estimated x coordinate
  double y = estimated_point.getY();  // To get estimated y coordinate
  Serial.printf("Got estimated x and y: %lf, %lf\n", x, y);

  M5.Lcd.setCursor(0, 40, 2);
  M5.Lcd.print("X Coordinate: ");
  M5.Lcd.print(x);
  
  M5.Lcd.setCursor(0, 60, 2);
  M5.Lcd.print("y Coordinate: ");
  M5.Lcd.print(y);
  // Send to main node
  JSONVar data;
  Serial.printf("Sending to main node\n");
  data["x"] = x;
  data["y"] = y;
  data["type"] = "LOCATION";
  data["macAddress"] = WiFi.macAddress().c_str();
  data["nodeID"] = (int)mesh.getNodeId();
  
  // sending elderly and geofence area 
  data["elderly"] = "Karen";
  data["geofenced_area"] = "Flat B";

  String output = JSON.stringify(data);
  Serial.println(output);

  String encrypted_message = xor_encrypt(output, xorKey);
  Serial.println(encrypted_message);

  mesh.sendSingle(mainNode, encrypted_message);
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

  // Serial.printf("startHere: Received from %u msg=%s\n", from, msg.c_str());

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
    // Serial.println("Not a JSON");
    return;
  }

  // Serial.println(newInfo["type"]);
  
  if (strcmp((const char*)newInfo["type"], "BEACON") == 0) {
    // Serial.println("Received new beacon\n");
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
  } else {
    Serial.println("not a beacon\n");
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
  pinMode(BUTTON_A_PIN, INPUT);
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
  if (digitalRead(BUTTON_A_PIN) == LOW) { 
    JSONVar data;
    Serial.printf("Sending panic to main node\n");
    data["macAddress"] = WiFi.macAddress().c_str();
    data["nodeID"] = (int)mesh.getNodeId();
    data["type"] = "PANIC";

    String output = JSON.stringify(data);
    mesh.sendSingle(mainNode, output);
    while(digitalRead(BUTTON_A_PIN) == LOW); // Wait for button release to avoid multiple writes
  }

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