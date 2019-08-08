#include "heltec.h"
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#define BAND 915E6 //you can set band here directly,e.g. 868E6,915E6
#define SPREADING_FACTOR 10

#define SERVICE_UUID "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"

byte localAddress;       // address of this device
byte destination = 0xFF; // destination to send to

String outgoing; // outgoing message
String msgString = "";
byte msgCount = 0;     // count of outgoing messages
long lastSendTime = 0; // last send time
int interval = 2000;   // interval between sends

void setup()
{
  //WIFI Kit series V1 not support Vext control
  Heltec.begin(true /*DisplayEnable Enable*/, true /*Heltec.LoRa Disable*/, true /*Serial Enable*/, true /*PABOOST Enable*/, BAND /*long BAND*/);
  LoRa.setSpreadingFactor(SPREADING_FACTOR);
  LoRa.setTxPowerMax(20);
  getLocalAddress();

  initBluetooth();

  printScreen("Welcome to RaveCom 3000 #" + String(localAddress));
  LoRa.onReceive(onReceive);
  LoRa.receive();
}

void getLocalAddress()
{
  const int point = 5;
  for (int i = 0; i < 6; i++)
  {
    localAddress = 5;
  }
}

void initScreen()
{
  Heltec.display->init();
  Heltec.display->flipScreenVertically();
  Heltec.display->setFont(ArialMT_Plain_10);
  Heltec.display->display();
}

class BLECallback: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) {
      std::string value = pCharacteristic->getValue();

      if (value.length() > 0) {
        Serial.println("*********");
        Serial.print("New value: ");
        for (int i = 0; i < value.length(); i++)
          Serial.print(value[i]);

        Serial.println();
        Serial.println("*********");
      }
    }
};

bool initBluetooth()
{

  BLEDevice::init("ravecom-" + localAddress);
  BLEServer *pServer = BLEDevice::createServer();

  BLEService *pService = pServer->createService(SERVICE_UUID);

  BLECharacteristic *pCharacteristic = pService->createCharacteristic(
      CHARACTERISTIC_UUID,
      BLECharacteristic::PROPERTY_READ |
          BLECharacteristic::PROPERTY_WRITE);

  pCharacteristic->setCallbacks(new BLECallback());

  pCharacteristic->setValue("Hello World");
  pService->start();

  BLEAdvertising *pAdvertising = pServer->getAdvertising();
  pAdvertising->start();
}

void loop()
{
  renderScreen();
  if (millis() - lastSendTime > interval)
  {
    int before = millis();
    sendMessage(outgoing);
    lastSendTime = millis(); // timestamp the message
    Serial.println("send time: " + String(millis() - before));
    interval = random(5000) + 1000; // 2-3 seconds
    LoRa.receive();                 // go back into receive mode
  }
}

void sendMessage(String outgoing)
{
  digitalWrite(LED, LOW);
  LoRa.beginPacket();              // start packet
  LoRa.write(destination);         // add destination address
  LoRa.write(localAddress);        // add sender address
  LoRa.write(msgCount);            // add message ID
  LoRa.write(outgoing.length());   // add payload length
  LoRa.print(outgoing);            // add payload
  LoRa.endPacket();                // finish packet and send it
  msgCount = (msgCount + 1 % 256); // increment message ID
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
  outgoing = msgString;
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
    return; // if there's no packet, return

  // read packet header bytes:
  int recipient = LoRa.read();       // recipient address
  byte sender = LoRa.read();         // sender address
  byte incomingMsgId = LoRa.read();  // incoming msg ID
  byte incomingLength = LoRa.read(); // incoming msg length

  String incoming = ""; // payload of packet

  while (LoRa.available()) // can't use readString() in callback
  {
    incoming += (char)LoRa.read(); // add bytes one by one
  }

  if (incomingLength != incoming.length()) // check length for error
  {
    printScreen("error: message length does not match length");
    return; // skip rest of function
  }

  // if message is for this device, or broadcast, print details:
  String msg = "";
  msg += " ID: " + String(incomingMsgId);
  msg += " r: " + String(LoRa.packetRssi());
  msg += " s: " + String(LoRa.packetSnr());
  msg += " m: " + String(incoming);
  printScreen(msg);
  printScreen("               t: " + String(millis()));
  digitalWrite(LED, HIGH);
}
