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

std::string outgoing; // outgoing message
String msgString = "";
byte msgCount = 0;     // count of outgoing messages
long lastSendTime = 0; // last send time
int interval = INTERVAL;
void setup()
{
  Heltec.begin(true /*DisplayEnable Enable*/, true /*Heltec.LoRa Disable*/, true /*Serial Enable*/, true /*PABOOST Enable*/, BAND /*long BAND*/);
  LoRa.setSpreadingFactor(SPREADING_FACTOR);
  LoRa.setTxPowerMax(20);

  initBluetooth();

  printScreen("Dispair, humans, for The Mothership has arrived.");
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
    outgoing = value;
  }
};

bool initBluetooth()
{

  BLEDevice::init("The Mothership");
  BLEServer *pServer = BLEDevice::createServer();
  BLEService *pService = pServer->createService(SERVICE_UUID);

  BLECharacteristic *pCharacteristic = pService->createCharacteristic(
      CHARACTERISTIC_UUID,
      BLECharacteristic::PROPERTY_READ |
          BLECharacteristic::PROPERTY_WRITE);

  pCharacteristic->setCallbacks(new BLECallback());
  pService->start();

  BLEAdvertising *pAdvertising = pServer->getAdvertising();
  pAdvertising->setMinPreferred(0x06);  // functions that help with iPhone connections issue
  pAdvertising->setMinPreferred(0x12);
  pAdvertising->start();
}

void loop()
{
  renderScreen();
  if (millis() - lastSendTime > interval)
  {
    sendMessage();                   // go back into receive mode
  }
}

void sendMessage()
{   if(outgoing.length() == 0) return;
    auto before = millis();
    sendLora();

    lastSendTime = millis();
    auto timeItTook = lastSendTime - before;
    sendSerial(timeItTook);
    
    if(timeItTook > interval) {
      Serial.println("Message took too long to send. Increasing interval.");
      interval = interval + ((timeItTook - interval) * 2);
    } 
}

void sendLora() {
  digitalWrite(LED, HIGH);
  LoRa.beginPacket();
  LoRa.write(outgoing.length());
  LoRa.print(String(outgoing.c_str()));
  LoRa.endPacket();
  digitalWrite(LED, LOW);
  msgCount++;
}

void sendSerial(int time) {
  Serial.print("sent: ");
  Serial.print(String(outgoing.c_str()));

  Serial.print(" msg#: ");
  Serial.print(msgCount);

  Serial.print(" len: ");
  Serial.print(outgoing.length());  
  Serial.print(" time: ");
  Serial.print(time);
  Serial.println();  
}

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
