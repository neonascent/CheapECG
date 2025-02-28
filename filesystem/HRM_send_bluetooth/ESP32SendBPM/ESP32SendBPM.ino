/*
    Video: https://www.youtube.com/watch?v=oCMOYS71NIU
    Based on Neil Kolban example for IDF: https://github.com/nkolban/esp32-snippets/blob/master/cpp_utils/tests/BLE%20Tests/SampleNotify.cpp
    Ported to Arduino ESP32 by Evandro Copercini

   Create a BLE server that, once we receive a connection, will send periodic notifications.
   The service advertises itself as: 6E400001-B5A3-F393-E0A9-E50E24DCCA9E
   Has a characteristic of: 6E400002-B5A3-F393-E0A9-E50E24DCCA9E - used for receiving data with "WRITE"
   Has a characteristic of: 6E400003-B5A3-F393-E0A9-E50E24DCCA9E - used to send data with  "NOTIFY"

   The design of creating the BLE server is:
   1. Create a BLE Server
   2. Create a BLE Service
   3. Create a BLE Characteristic on the Service
   4. Create a BLE Descriptor on the characteristic
   5. Start the service.
   6. Start advertising.

   In this example rxValue is the data received (only accessible inside that function).
   And txValue is the data to be sent, in this example just a byte incremented every second.
*/
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

#define SERVICE_UUID           "6E400001-B5A3-F393-E0A9-E50E24DCCA9E"
#define CHARACTERISTIC_UUID_TX "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"

// Timer variables
unsigned long lastTime = 0;
unsigned long timerDelay = 3;

// Analog pin
#define analogPin A0
int adcValue = 0;
int maxThisWindow = 0;
float average = 2024.0;
int sampleCount = 0;
int subsample = 4;

BLEServer *pServer = NULL;
BLECharacteristic *pTxCharacteristic;
bool deviceConnected = false;
bool oldDeviceConnected = false;

bool isPeak = false;
unsigned long lastBeatTime = 0;
#define MAX_BEATS 20
unsigned long beatTimes[MAX_BEATS] = {0};  // Store last N beat timestamps
int beatIndex = 0;

class MyServerCallbacks : public BLEServerCallbacks {
    void onConnect(BLEServer *pServer) {
        deviceConnected = true;
        Serial.println("Connected");
    }

    void onDisconnect(BLEServer *pServer) {
        deviceConnected = false;
        Serial.println("Disconnected");
    }
};

void setup() {
    Serial.begin(115200);

    // Create BLE Device
    BLEDevice::init("UART Service");

    // Create BLE Server
    pServer = BLEDevice::createServer();
    pServer->setCallbacks(new MyServerCallbacks());

    // Create BLE Service
    BLEService *pService = pServer->createService(SERVICE_UUID);

    // Create BLE Characteristic for notifying BPM
    pTxCharacteristic = pService->createCharacteristic(
        CHARACTERISTIC_UUID_TX,
        BLECharacteristic::PROPERTY_NOTIFY
    );

    pTxCharacteristic->addDescriptor(new BLE2902());

    // Start Service & Advertising
    pService->start();
    pServer->getAdvertising()->start();
    Serial.println("Waiting for client connection...");
}

void loop() {
    doSample();

    // Handle disconnection & reconnection
    if (!deviceConnected && oldDeviceConnected) {
        delay(500);
        pServer->startAdvertising();
        Serial.println("Restarting advertising...");
        oldDeviceConnected = deviceConnected;
    }
    if (deviceConnected && !oldDeviceConnected) {
        oldDeviceConnected = deviceConnected;
    }
}

void doSample() {
    if ((millis() - lastTime) > timerDelay) {
        adcValue = analogRead(analogPin);
        maxThisWindow = max(adcValue, maxThisWindow); 
        average += adcValue;
        if (++sampleCount >= subsample) {
            float valueAverage = average / subsample / 4;
            processData(valueAverage);
            //Serial.println(maxThisWindow / 2);//valueAverage);
            Serial.println(valueAverage);
            sampleCount = 0;
            average = 2024;
            maxThisWindow = 0;

        }
        lastTime = millis();
    }
}

void processData(float value) {
    if (!isPeak && value > 800) {
        detectBeat();
        isPeak = true;
    } else if (isPeak && value < 800) {
        isPeak = false;
    }
}

void detectBeat() {
    Serial.println("Beat detected!");
    unsigned long now = millis();

    // Store timestamp in circular buffer
    beatTimes[beatIndex % MAX_BEATS] = now;
    beatIndex++;

    // Ensure at least 2 beats before calculating BPM
    if (beatIndex >= 2) {
        sendBPM();
    }
}

void sendBPM() {
    if (!deviceConnected) {
        Serial.println("BLE not connected. Skipping BPM send.");
        return;
    }

    int validBeats = 0;
    unsigned long totalInterval = 0;

    // Ensure we have at least 2 stored beats
    if (beatIndex < 2) {
        Serial.println("Not enough beats detected yet.");
        return;
    }

    // Use a rolling window of the last MAX_BEATS stored timestamps
    int beatsUsed = min(beatIndex, MAX_BEATS); // Use only valid stored beats
    for (int i = 1; i < beatsUsed; i++) {
        unsigned long prevTime = beatTimes[(beatIndex - i) % MAX_BEATS];
        unsigned long currTime = beatTimes[(beatIndex - (i + 1)) % MAX_BEATS];

        if (prevTime > 0 && currTime > 0 && prevTime > currTime) { // Ensure valid interval
            totalInterval += (prevTime - currTime);
            validBeats++;
        }
    }

    if (validBeats > 0) {
        float avgInterval = totalInterval / (float)validBeats;
        uint8_t bpm = (uint8_t)(60000.0 / avgInterval);

        Serial.printf("Calculated BPM: %d\n", bpm);

        // Send BPM via BLE
        pTxCharacteristic->setValue(&bpm, 1);
        pTxCharacteristic->notify();
        Serial.println("BPM sent via BLE.");
    } else {
        Serial.println("No valid beat intervals found.");
    }
}



