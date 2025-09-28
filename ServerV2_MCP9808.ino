#include <Wire.h>
#include <Adafruit_MCP9808.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

// UUIDs used in this example:
#define SERVICE_UUID          "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID_1 "beb5483e-36e1-4688-b7f5-ea07361b26a8"
#define CHARACTERISTIC_UUID_2 "1c95d5e3-d8f7-413a-bf3d-7a2e5d7be87e"

// Initialize all pointers
BLEServer* pServer = NULL;
BLECharacteristic* pCharacteristic_1 = NULL;
BLECharacteristic* pCharacteristic_2 = NULL;
BLEDescriptor *pDescr_1;
BLE2902 *pBLE2902_1;
BLE2902 *pBLE2902_2;

// Some variables to keep track of device connection
bool deviceConnected = false;
bool oldDeviceConnected = false;

// Variable for MCP9808 temperature sensor
Adafruit_MCP9808 mcp;

// Callback function that is called whenever a client is connected or disconnected
class MyServerCallbacks : public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
        deviceConnected = true;
    }

    void onDisconnect(BLEServer* pServer) {
        deviceConnected = false;
    }
};

void setup() {
    Serial.begin(115200);

    // Create the BLE Device
    BLEDevice::init("ESP32");

    // Create the BLE Server
    pServer = BLEDevice::createServer();
    pServer->setCallbacks(new MyServerCallbacks());

    // Create the BLE Service
    BLEService *pService = pServer->createService(BLEUUID(SERVICE_UUID));

    // Create a BLE Characteristic
    pCharacteristic_1 = pService->createCharacteristic(
                          BLEUUID(CHARACTERISTIC_UUID_1),
                          BLECharacteristic::PROPERTY_NOTIFY
                        );

    pCharacteristic_2 = pService->createCharacteristic(
                          BLEUUID(CHARACTERISTIC_UUID_2),
                          BLECharacteristic::PROPERTY_READ   |
                          BLECharacteristic::PROPERTY_WRITE  |
                          BLECharacteristic::PROPERTY_NOTIFY
                        );

    // Create a BLE Descriptor
    pDescr_1 = new BLEDescriptor(BLEUUID((uint16_t)0x2901));
    pDescr_1->setValue("Temperature from MCP9808 in Celsius");
    pCharacteristic_1->addDescriptor(pDescr_1);

    // Add the BLE2902 Descriptor because we are using "PROPERTY_NOTIFY"
    pBLE2902_1 = new BLE2902();
    pBLE2902_1->setNotifications(true);
    pCharacteristic_1->addDescriptor(pBLE2902_1);

    pBLE2902_2 = new BLE2902();
    pBLE2902_2->setNotifications(true);
    pCharacteristic_2->addDescriptor(pBLE2902_2);

    // Start the service
    pService->start();

    // Start advertising
    BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
    pAdvertising->addServiceUUID(BLEUUID(SERVICE_UUID));
    pAdvertising->setScanResponse(false);
    pAdvertising->setMinPreferred(0x0);
    BLEDevice::startAdvertising();
    Serial.println("Waiting for a client connection to notify...");

    // Initialize MCP9808 sensor
    if (!mcp.begin()) {
        Serial.println("Couldn't find MCP9808!");
        while (1);
    }
}

void loop() {
    // Notify changed value
    if (deviceConnected) {
        // Read temperature from MCP9808 sensor in Celsius
        float temperature = mcp.readTempC();

        // Send the temperature value to the client
        pCharacteristic_1->setValue(temperature);
        pCharacteristic_1->notify();

        // Log the temperature to Serial
        Serial.print("Temperature: ");
        Serial.println(temperature);

        // In this example, "delay" is used to delay with one second.
        // For production code, consider using millis().
        delay(1000);
    }

    // The code below keeps the connection status up to date:
    // Disconnecting
    if (!deviceConnected && oldDeviceConnected) {
        delay(500); // give the Bluetooth stack the chance to get things ready
        pServer->startAdvertising(); // restart advertising
        Serial.println("Start advertising");
        oldDeviceConnected = deviceConnected;
    }

    // Connecting
    if (deviceConnected && !oldDeviceConnected) {
        // do stuff here on connecting
        oldDeviceConnected = deviceConnected;
    }
}
