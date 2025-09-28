#include "BLEDevice.h"

// UUIDs for the MCP9808 and LM75 servers
static BLEUUID serviceUUID1("4fafc201-1fb5-459e-8fcc-c5c9c331914b"); // MCP9808
static BLEUUID charUUID1_1("beb5483e-36e1-4688-b7f5-ea07361b26a8");
static BLEUUID serviceUUID2("4edafa4d-02e5-4a64-ba3b-361bf2057e3a"); // LM75
static BLEUUID charUUID2_1("df41524a-8bfa-4b8f-bf1c-8262397c020b");

// Connection flags and device pointers for both servers
static boolean doConnectServer1 = false, connectedServer1 = false;
static boolean doConnectServer2 = false, connectedServer2 = false;
static BLEAdvertisedDevice* myDevice1 = nullptr;
static BLEAdvertisedDevice* myDevice2 = nullptr;

// Pointers to remote characteristics for both servers
BLERemoteCharacteristic* pRemoteChar1_1 = nullptr;
BLERemoteCharacteristic* pRemoteChar2_1 = nullptr;

// Generic callback for notifications, adapted for string and binary data
static void notifyCallback(
  BLERemoteCharacteristic* pBLERemoteCharacteristic, uint8_t* pData, size_t length, bool isNotify) {
  
  Serial.print("Notification from characteristic: ");
  Serial.println(pBLERemoteCharacteristic->getUUID().toString().c_str());

  if(pBLERemoteCharacteristic->getUUID().equals(charUUID1_1)) {
    // Handle data from MCP9808
    if(length == 4) { // Expecting 4 bytes for a float
      float temperature;
      memcpy(&temperature, pData, sizeof(temperature));
      Serial.print("Temperature from MCP9808: ");
      Serial.println(temperature);
    } else {
      Serial.println("Unexpected data length for MCP9808, cannot interpret as float.");
    }
  } else if(pBLERemoteCharacteristic->getUUID().equals(charUUID2_1)) {
    // Handle data from LM75
    String receivedData = String((char*)pData).substring(0, length);
    float temperature = receivedData.toFloat();
    Serial.print("Temperature from LM75: ");
    Serial.println(temperature);
  }
}

class MyClientCallback : public BLEClientCallbacks {
  void onConnect(BLEClient* pclient) {}

  void onDisconnect(BLEClient* pclient) {
    Serial.println("Disconnected");
  }
};

bool connectToServer(BLEAdvertisedDevice* device, BLEUUID serviceUUID, BLERemoteCharacteristic** pRemoteChar, BLEUUID charUUID) {
  Serial.print("Forming a connection to ");
  Serial.println(device->getAddress().toString().c_str());
  
  BLEClient* pClient = BLEDevice::createClient();
  Serial.println(" - Created client");
  pClient->setClientCallbacks(new MyClientCallback());

  // Attempt to connect to the remote BLE server.
  if (!pClient->connect(device)) {
    Serial.println(" - Failed to connect");
    return false;
  }
  Serial.println(" - Connected to server");

  // Obtain a reference to the specified service on the server.
  BLERemoteService* pRemoteService = pClient->getService(serviceUUID);
  if (pRemoteService == nullptr) {
    Serial.print("Failed to find service UUID: ");
    Serial.println(serviceUUID.toString().c_str());
    pClient->disconnect();
    return false;
  }
  Serial.println(" - Found our service");

  // Attempt to retrieve a reference to the specified characteristic on the server.
  *pRemoteChar = pRemoteService->getCharacteristic(charUUID);
  if (*pRemoteChar == nullptr) {
    Serial.print("Failed to find characteristic UUID: ");
    Serial.println(charUUID.toString().c_str());
    pClient->disconnect();
    return false;
  }
  Serial.println(" - Found our characteristic");

  // If the characteristic supports notifications, register for them.
  if ((*pRemoteChar)->canNotify())
    (*pRemoteChar)->registerForNotify(notifyCallback);

  return true;
}

class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
  void onResult(BLEAdvertisedDevice advertisedDevice) override {
    Serial.print("BLE Device found: ");
    Serial.println(advertisedDevice.toString().c_str());

    // Check if the advertised device is one of the sensors we're interested in.
    if (advertisedDevice.haveServiceUUID() && advertisedDevice.isAdvertisingService(serviceUUID1)) {
      BLEDevice::getScan()->stop();
      myDevice1 = new BLEAdvertisedDevice(advertisedDevice);
      doConnectServer1 = true;
    } else if (advertisedDevice.haveServiceUUID() && advertisedDevice.isAdvertisingService(serviceUUID2)) {
      BLEDevice::getScan()->stop();
      myDevice2 = new BLEAdvertisedDevice(advertisedDevice);
      doConnectServer2 = true;
    }
  }
};

void setup() {
  Serial.begin(115200);
  Serial.println("Starting BLE client application");
  BLEDevice::init("");

  // Start scanning for devices
  BLEScan* pBLEScan = BLEDevice::getScan();
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setInterval(1349);
  pBLEScan->setWindow(449);
  pBLEScan->setActiveScan(true);
  pBLEScan->start(5, false);
}

void loop() {
  // Connecting or reconnecting
  if (doConnectServer1) {
    if (connectToServer(myDevice1, serviceUUID1, &pRemoteChar1_1, charUUID1_1)) {
      Serial.println("Connected to server 1");
      connectedServer1 = true;
    } else {
      Serial.println("Failed to connect to server 1. Reconnecting...");
      doConnectServer1 = false;
      myDevice1 = nullptr;
    }
  }
  
  if (doConnectServer2) {
    if (connectToServer(myDevice2, serviceUUID2, &pRemoteChar2_1, charUUID2_1)) {
      Serial.println("Connected to server 2");
      connectedServer2 = true;
    } else {
      Serial.println("Failed to connect to server 2. Reconnecting...");
      doConnectServer2 = false;
      myDevice2 = nullptr;
    }
  }

  // Put your main code here, to run repeatedly:
  delay(10000); // Delay for demonstration purpose
}