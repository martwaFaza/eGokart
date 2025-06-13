#include <Adafruit_MAX31865.h>
#include <Wire.h>
#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>



#define PI 3.141
#define MAX31865_CS   5   // CS pin -> D5
#define MAX31865_MOSI 23  // MOSI (SDI) -> D23
#define MAX31865_MISO 19  // MISO (SDO) -> D19
#define MAX31865_CLK  18  // CLK -> D18

Adafruit_MAX31865 thermo = Adafruit_MAX31865(MAX31865_CS, MAX31865_MOSI, MAX31865_MISO, MAX31865_CLK);

const int pinEnkoder = 27;
const float iloscImpulsow = 11.0;
volatile int licznik = 0;
int timer;
int timer2;
float predkoscObrotowa = 0.0;
float predkoscObrotowa_m = 0.0;
const float obwodKola = 0.82;
float promienKola;
float predkoscLiniowa = 0;
float predkoscLiniowaDrukowanie;
const float konwersjaDoKm_h = 6.0/100.0;
const float wspolczynnikNadziei = 6.1869;
const float rezystor = 1000.0;
const float rezystor_odniesienia = 4289.0;

volatile int timer_1 = millis();
volatile int timer_2 = millis();
volatile int delta_czas;
float d_czas;


void IRAM_ATTR onInterrupt() {
  delta_czas = timer_2 - timer_1;
  timer_1 = timer_2; 
  timer_2 = millis();
}

//komunkacja
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

const int maxCnt = 100;

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


//komunikacja

void setup()
{

  // komunikacja
Serial.begin(115200);


  // unsigned long start = micros();
  // int old = 1;
  // int cnt = 0;
  // while (cnt < maxCnt) 
  // {
  //   int val = digitalRead(hallPin);
  //   if (!val && val != old) cnt++;
  //   old = val;
  // }
  
  // float sekundy = (micros() - start) / 1000000.0;
  // float rpm = cnt / sekundy * 60.0;

  // Serial.print("rpm: ");
  // Serial.println(rpm);

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

  //komunikacja
  Serial.begin(115200);
  pinMode(pinEnkoder, INPUT);
  timer = millis();
  attachInterrupt(digitalPinToInterrupt(pinEnkoder), onInterrupt, RISING);
  promienKola = (obwodKola/(2*PI)); 
  thermo.begin(MAX31865_3WIRE);
}

void loop() {


  // if(millis() - timer > 1000)
  // {
  //   predkoscLiniowaDrukowanie = obliczaniePredkosci(licznik);
  //   timer = millis();
  //   licznik = 0;
  // }

  uint16_t rtd = thermo.readRTD();
  float rezystancja = rtd;
  rezystancja /= 32768;
  rezystancja *= rezystor_odniesienia;

  float temperatura = thermo.temperature(rezystor, rezystor_odniesienia);

  d_czas = (float)delta_czas;
  predkoscObrotowa = 1.00 / (d_czas/60000.00);
  predkoscLiniowa = predkoscObrotowa * obwodKola / 1000.00 * 60.00;
  predkoscObrotowa_m = predkoscObrotowa * 33.00 / 8.00;
  if(millis()- timer_2  > 5000)
  {
predkoscObrotowa = 0.00;
predkoscLiniowa = 0.00;
predkoscObrotowa_m = 0.0;
  }

  //komunikacja
 if (deviceConnected) {
        pChar_01->setValue(value_01);
        pChar_01->notify();
        pChar_02->setValue(value_02);
        pChar_02->notify();
        pChar_03->setValue(value_03);
        pChar_03->notify();
        pChar_04->setValue(value_04);
        pChar_04->notify();

        
        value_01=(int)(temperatura*100)*10;
        value_02=(int)(predkoscObrotowa_m*100)*10;
        value_03=(int)(predkoscLiniowa*100)*10;;
        value_04=3003;
        delay(100);

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
  //komunikacja

  Serial.println("Delta Czas:");
  Serial.println(delta_czas);
  Serial.println("Delta Czas_f:");
  Serial.println(d_czas);
  Serial.println("Timer_1:");
  Serial.println(timer_1);
  Serial.println("Timer_2:");
  Serial.println(timer_2);
  
  //Serial.println("Timer:");
  //Serial.println(timer);
  Serial.println("Predkosc liniowa:");
  Serial.println(predkoscLiniowa);
  Serial.println("Predkosc obrotowa:");
  Serial.println(predkoscObrotowa);
  Serial.println("Temperatura: ");
  Serial.println(temperatura);
}


