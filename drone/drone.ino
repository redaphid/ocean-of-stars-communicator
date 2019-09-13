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
String incoming = "";

int localAddress = 0;
int msgCount = 1;
BLECharacteristic *pCharacteristic;
void setup()
{
  setCpuFrequencyMhz(80);
  Heltec.begin(true /*DisplayEnable Enable*/, true /*Heltec.LoRa Disable*/, true /*Serial Enable*/, true /*PABOOST Enable*/, BAND /*long BAND*/);
  LoRa.setSpreadingFactor(SPREADING_FACTOR);  
  getLocalAddress();

  initBluetooth();  
  printScreen("Drone #" + String(localAddress) + " has awakened.");
  renderScreen();
  LoRa.onReceive(onReceive);
  LoRa.receive();
}

void initScreen()
{
  return;
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
      //Serial.print("received bluetooth value: ");              
      //Serial.println(value.c_str());      
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

  BLEDevice::init(String("Drone " + String(localAddress)).c_str());
  BLEServer *pServer = BLEDevice::createServer();
  BLEService *pService = pServer->createService(SERVICE_UUID);

    pCharacteristic = pService->createCharacteristic(
      CHARACTERISTIC_UUID,
      BLECharacteristic::PROPERTY_READ   |
                      BLECharacteristic::PROPERTY_WRITE  |
                      BLECharacteristic::PROPERTY_NOTIFY |
                      BLECharacteristic::PROPERTY_INDICATE
    );

  pCharacteristic->setCallbacks(new BLECallback());
  pService->start();

  BLEAdvertising *pAdvertising = pServer->getAdvertising();
  pAdvertising->setMinPreferred(0x06);  // functions that help with iPhone connections issue
  pAdvertising->setMinPreferred(0x12);
  pAdvertising->start();
}


void loop(){
  renderScreen();
}

void printScreen(String message)
{
  //Serial.println(message);
  msgString += message;
}

void renderScreen()
{
  digitalWrite(LED, LOW);
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
  digitalWrite(LED, HIGH);
  if (packetSize == 0)
    return;

  byte incomingLength = LoRa.read();
  incoming = "";

  while (LoRa.available()){
    incoming += (char)LoRa.read();
  }

  if (incomingLength != incoming.length())
  {
    printScreen("error: message length does not match length\n" + incoming);
    return;
  }

  String msg = "";
  msg += "Drone #" + String(localAddress) + \
  "                    Heard from mothership " + msgCount++ + "x";
  printScreen(msg);
  pCharacteristic->setValue((uint8_t*) incoming.c_str(), incoming.length());
  pCharacteristic->notify();    
}
