/*
  Based on Neil Kolban example for IDF: https://github.com/nkolban/esp32-snippets/blob/master/cpp_utils/tests/BLE%20Tests/SampleNotify.cpp
  Ported to Arduino ESP32 by Evandro Copercini
  updated by chegewara and MoThunderz
*/
#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

BLEServer* pServer = NULL;
BLECharacteristic* pChar_01 = NULL;
BLEDescriptor *pDescr_01;
BLE2902 *pBLE2902_01;
BLECharacteristic* pChar_02 = NULL;
BLEDescriptor *pDescr_02;
BLE2902 *pBLE2902_02;
BLECharacteristic* pChar_03 = NULL;
BLEDescriptor *pDescr_03;
BLE2902 *pBLE2902_03;
BLECharacteristic* pChar_04 = NULL;
BLEDescriptor *pDescr_04;
BLE2902 *pBLE2902_04;

bool deviceConnected = false;
bool oldDeviceConnected = false;
uint32_t value = 0;
uint32_t value_01 = 0;
uint32_t value_02 = 0;
uint32_t value_03 = 0;
uint32_t value_04 = 0;

// See the following for generating UUIDs:
// https://www.uuidgenerator.net/

#define SERVICE_UUID "b7c713f7-3479-4604-8f98-8e7d5bcb7e97"
#define CHAR_UUID_01 "d83f51c9-564f-4028-9ac5-2aa00948e23f"
#define CHAR_UUID_02 "662e5d0f-82fa-47b9-a223-7160dcd1a313"
#define CHAR_UUID_03 "e28bd0e6-d57e-4619-87fe-504a8d9f967b"
#define CHAR_UUID_04 "4421fecd-b521-4b99-8bae-bd917ce840b0"

class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
      deviceConnected = true;
    };

    void onDisconnect(BLEServer* pServer) {
      deviceConnected = false;
    }
};

uint32_t ThermistorPin = 34;
uint32_t Vo;
float R1 = 10000.0;
float logR2, R2, T_01;
float c1 = 1.009249522e-03, c2 = 2.378405444e-04, c3 = 2.019202697e-07;


void setup() {
  Serial.begin(115200);

  // Create the BLE Device
  BLEDevice::init("eGokart");

  // Create the BLE Server
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  // Create the BLE Service
  BLEService *pService = pServer->createService(SERVICE_UUID);

  // Create a BLE Characteristic
  pChar_01 = pService->createCharacteristic(
             CHAR_UUID_01,
             BLECharacteristic::PROPERTY_NOTIFY
             );                   
  pChar_02 = pService->createCharacteristic(
             CHAR_UUID_02,
             BLECharacteristic::PROPERTY_NOTIFY
             );
  pChar_03 = pService->createCharacteristic(
             CHAR_UUID_03,
             BLECharacteristic::PROPERTY_NOTIFY
             );
  pChar_04 = pService->createCharacteristic(
             CHAR_UUID_04,
             BLECharacteristic::PROPERTY_NOTIFY
             );
  // Create a BLE Descriptor
  
  pDescr_01 = new BLEDescriptor((uint16_t)0x2901);
  pDescr_01->setValue("Motor temperature value - deg C");
  pChar_01->addDescriptor(pDescr_01);
  pDescr_02 = new BLEDescriptor((uint16_t)0x2901);
  pDescr_02->setValue("Motor speed value - rpm");
  pChar_02->addDescriptor(pDescr_02);
  pDescr_03 = new BLEDescriptor((uint16_t)0x2901);
  pDescr_03->setValue("Battery voltage value - V");
  pChar_03->addDescriptor(pDescr_03);
  pDescr_04 = new BLEDescriptor((uint16_t)0x2901);
  pDescr_04->setValue("Battery power value - W");
  pChar_04->addDescriptor(pDescr_04);
  
  pBLE2902_01 = new BLE2902();
  pBLE2902_01->setNotifications(true);
  pChar_01->addDescriptor(pBLE2902_01);
  pBLE2902_02 = new BLE2902();
  pBLE2902_02->setNotifications(true);
  pChar_02->addDescriptor(pBLE2902_02);
  pBLE2902_03 = new BLE2902();
  pBLE2902_03->setNotifications(true);
  pChar_03->addDescriptor(pBLE2902_03);
  pBLE2902_04 = new BLE2902();
  pBLE2902_04->setNotifications(true);
  pChar_04->addDescriptor(pBLE2902_04);

  // Start the service
  pService->start();

  // Start advertising
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(false);
  pAdvertising->setMinPreferred(0x0);  // set value to 0x00 to not advertise this parameter
  BLEDevice::startAdvertising();
  Serial.println("Waiting a client connection to notify...");
}

void loop() {
    // notify changed value
    if (deviceConnected) {
        pChar_01->setValue(value_01);
        pChar_01->notify();
        pChar_02->setValue(value_02);
        pChar_02->notify();
        pChar_03->setValue(value_03);
        pChar_03->notify();
        pChar_04->setValue(value_04);
        pChar_04->notify();

        Vo = analogRead(ThermistorPin);
        R2 = R1 * (4095.0 / (float)Vo - 1.0);
        logR2 = log(R2);
        T_01 = (1.0 / (c1 + c2*logR2 + c3*pow(logR2,3))) - 273.15;
        //T_01 = (T_01 * 9.0)/ 5.0 + 32.0; 
        value_01=(int)1000.0*T_01;
        value++;
        value_02=1001*value;
        value_03=2002*value;
        value_04=3003*value;
        delay(1000);

    }
    // disconnecting
    if (!deviceConnected && oldDeviceConnected) {
        delay(500); // give the bluetooth stack the chance to get things ready
        pServer->startAdvertising(); // restart advertising
        Serial.println("start advertising");
        oldDeviceConnected = deviceConnected;
    }
    // connecting
    if (deviceConnected && !oldDeviceConnected) {
        // do stuff here on connecting
        oldDeviceConnected = deviceConnected;
    }
}
