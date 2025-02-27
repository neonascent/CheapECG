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


// Timer variables
unsigned long lastTime = 0;
unsigned long timerDelay = 3;

// analogue pin
#define analogPin A0 /* ESP8266 Analog Pin ADC0 = A0 */
int adcValue = 0;  /* Variable to store Output of ADC */
int maxThisWindow = 0;
float average = 512.0;
int sampleCount = 0;
int subsample = 4;

BLEServer *pServer = NULL;
BLECharacteristic *pTxCharacteristic;
bool deviceConnected = false;
bool oldDeviceConnected = false;
uint16_t txValue = 0;
uint8_t beatValue = 0;

bool isPeak = false;




// See the following for generating UUIDs:
// https://www.uuidgenerator.net/

#define SERVICE_UUID           "6E400001-B5A3-F393-E0A9-E50E24DCCA9E"  // UART service UUID "0000ffe0-0000-1000-8000-00805f9b34fb" // HM-10 service  // 
//#define CHARACTERISTIC_UUID_RX "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"//"0000ffe1-0000-1000-8000-00805f9b34fb"// HM-10 characteristic //
#define CHARACTERISTIC_UUID_TX "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"

class MyServerCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer *pServer) {
    deviceConnected = true;
    Serial.println("connected");
  };

  void onDisconnect(BLEServer *pServer) {
    deviceConnected = false;
    Serial.println("disconnected");
  }
};

class MyCallbacks : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *pCharacteristic) {
    String rxValue = pCharacteristic->getValue();

    if (rxValue.length() > 0) {
      Serial.println("*********");
      Serial.print("Received Value: ");
      for (int i = 0; i < rxValue.length(); i++) {
        Serial.print(rxValue[i]);
      }

      Serial.println();
      Serial.println("*********");
    }
  }
};

void setup() {
  Serial.begin(115200);

  // Create the BLE Device
  BLEDevice::init("UART Service");

  // Create the BLE Server
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  // Create the BLE Service
  BLEService *pService = pServer->createService(SERVICE_UUID);

  // Create a BLE Characteristic
  pTxCharacteristic = pService->createCharacteristic(CHARACTERISTIC_UUID_TX, BLECharacteristic::PROPERTY_NOTIFY);

  pTxCharacteristic->addDescriptor(new BLE2902());

   //BLECharacteristic *pRxCharacteristic = pService->createCharacteristic(CHARACTERISTIC_UUID_RX, BLECharacteristic::PROPERTY_WRITE);

   //pRxCharacteristic->setCallbacks(new MyCallbacks());

  // Start the service
  pService->start();

  // Start advertising
  pServer->getAdvertising()->start();
  Serial.println("Waiting a client connection to notify...");
}

void loop() {

  if (txValue > 1500) txValue = 0;
doSample();
  if (deviceConnected) {
    // uint8_t data[2];
    // data[0] = txValue & 0xFF;        // Lower byte
    // data[1] = (txValue >> 8) & 0xFF; // Higher byte
    // pTxCharacteristic->setValue(data, 2); // Send 16-bit value (2 bytes)
    // pTxCharacteristic->notify();  
    // Serial.printf("Sent: %u\n", txValue);
    //doSample();
  }

  // disconnecting
  if (!deviceConnected && oldDeviceConnected) {
    delay(500);                   // give the bluetooth stack the chance to get things ready
    pServer->startAdvertising();  // restart advertising
    Serial.println("start advertising");
    oldDeviceConnected = deviceConnected;
  }
  // connecting
  if (deviceConnected && !oldDeviceConnected) {
    // do stuff here on connecting
    oldDeviceConnected = deviceConnected;
  }
}

void doSample() {
  if ((millis() - lastTime) > timerDelay) {
    
    adcValue = analogRead(analogPin); /* Read the Analog Input value */  
    average = average + adcValue;
    maxThisWindow = max(adcValue, maxThisWindow); 

    // only send out after 
    if (sampleCount > subsample) {
      float valueAverage = average/subsample/4;
      processData(valueAverage);
      //sendData((uint16_t)valueAverage);
      //Serial.println(String(valueAverage));
      sampleCount = 0;
      average = 2048;
      maxThisWindow = 0;
    }
    sampleCount = sampleCount + 1;
    lastTime = millis();
    
  }
}

void processData(float value)  {
  if (!isPeak) {
    if (value >1000) {
      //sendData((uint16_t)1500);
      sendBeat();
      isPeak = true;
    }
  } else {
    if (value <1000) {
      //sendData((uint16_t)0);
      isPeak = false;
    }
  }
}

void sendBeat() {
    pTxCharacteristic->setValue(&beatValue, 1);
    pTxCharacteristic->notify();  
    beatValue++;
    Serial.printf("Beat");
}

void sendData(uint16_t value) {
    uint8_t data[2];
    data[0] = value & 0xFF;        // Lower byte
    data[1] = (value >> 8) & 0xFF; // Higher byte
    pTxCharacteristic->setValue(data, 2); // Send 16-bit value (2 bytes)
    pTxCharacteristic->notify();  
    //Serial.printf("Sent: %u\n", value);
   //Serial.printf("%u\n", value);
}