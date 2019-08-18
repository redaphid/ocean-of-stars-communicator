#include "heltec.h"
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include "esp_system.h"

#define BAND 915E6 //you can se\t band here directly,e.g. 868E6,915E6
#define SPREADING_FACTOR 11

#define SERVICE_UUID "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"

#define INTERVAL 5000

String msgString = "";

void setup()
{
  Heltec.begin(true /*DisplayEnable Enable*/, true /*Heltec.LoRa Disable*/, true /*Serial Enable*/, true /*PABOOST Enable*/, BAND /*long BAND*/);
  LoRa.setSpreadingFactor(SPREADING_FACTOR);
  LoRa.setTxPowerMax(20);

  initBluetooth();
  getLocalAddress();
  printScreen("Drone #" + String(localAddress) + " has awakened.");
  LoRa.onReceive(onReceive);
  LoRa.receive();
}

void initScreen()
{
  Heltec.display->init();
  Heltec.display->flipScreenVertically();
  Heltec.display->setFont(ArialMT_Plain_10);
  Heltec.display->display();
}

class BLECallback : public BLECharacteristicCallbacks
{
  void onWrite(BLECharacteristic *pCharacteristic)
  {
    std::string value = pCharacteristic->getValue();

    if (value.length() > 0)
    {
      Serial.print("received bluetooth value: ");              
      Serial.println(value.c_str());      
    }    
  }
};

void getLocalAddress()
{
  uint8_t mac[6];
  esp_efuse_mac_get_default(mac);
  localAddress = 0;
  for (int i = 0; i < 6; i++)
  {
    localAddress = ((int)localAddress + (int)mac[i]) % 256;
  }
}

bool initBluetooth()
{

  getLocalAddress();
  BLEDevice::init("drone-" + localAddress);
  BLEServer *pServer = BLEDevice::createServer();
  BLEService *pService = pServer->createService(SERVICE_UUID);

  BLECharacteristic *pCharacteristic = pService->createCharacteristic(
      CHARACTERISTIC_UUID,
      BLECharacteristic::PROPERTY_READ |
          BLECharacteristic::PROPERTY_WRITE);

  pCharacteristic->setCallbacks(new BLECallback());
  pService->start();

  BLEAdvertising *pAdvertising = pServer->getAdvertising();
  pAdvertising->start();
}

void loop(){}

void printScreen(String message)
{
  Serial.println(message);
  msgString += message;
}

void renderScreen()
{
  if (msgString == "")
    return;
  Heltec.display->clear();
  Heltec.display->setTextAlignment(TEXT_ALIGN_LEFT);
  Heltec.display->setFont(ArialMT_Plain_10);
  Heltec.display->drawStringMaxWidth(0, 0, 128, msgString);
  Heltec.display->display();
  msgString = "";
}

void printScreen()
{
  printScreen("");
}

void onReceive(int packetSize)
{
  if (packetSize == 0)
    return;

  byte incomingLength = LoRa.read();
  String incoming = "";

  while (LoRa.available()){
    incoming += (char)LoRa.read();
  }

  if (incomingLength != incoming.length())
  {
    printScreen("error: message length does not match length");
    return;
  }

  String msg = "";
  msg += " r: " + String(LoRa.packetRssi());
  msg += " s: " + String(LoRa.packetSnr());
  msg += " m: " + String(incoming);
  printScreen(msg);
  printScreen("               t: " + String(millis()));
  digitalWrite(LED, HIGH);
  sleep(100);
  digitalWrite(LED, LOW);
}
