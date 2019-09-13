#include "heltec.h"
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include "esp_system.h"

#define BAND 915E6 //you can se\t band here directly,e.g. 868E6,915E6
#define SPREADING_FACTOR 11

#define SERVICE_UUID "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"

#define MAX_INTERVAL 5000
#define MIN_INTERVAL 1000

byte localAddress;       // address of this device

std::string outgoing; // outgoing message
String msgString = "";
byte msgCount = 0;     // count of outgoing messages
long lastSendTime = 0; // last send time
int interval = 2000;   // interval between sends

void setup()
{
  Heltec.begin(true /*DisplayEnable Enable*/, true /*Heltec.LoRa Disable*/, true /*Serial Enable*/, true /*PABOOST Enable*/, BAND /*long BAND*/);
  LoRa.setSpreadingFactor(SPREADING_FACTOR);
  LoRa.setTxPowerMax(20);
  getLocalAddress();

  initBluetooth();

  printScreen("Welcome to RaveCom 3000 #" + String(localAddress));
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
      //Serial.print("received bluetooth value: ");              
      //Serial.println(value.c_str());      
    }
    outgoing = value;
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
  BLEDevice::init("ravecom-" + localAddress);
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

void loop()
{
  renderScreen();
  if (millis() - lastSendTime > interval)
  {
    sendMessage();   
    interval = random(MAX_INTERVAL) + MIN_INTERVAL;
    LoRa.receive();                 // go back into receive mode
  }
}

void sendMessage()
{   if(outgoing.length() == 0) return;
    auto before = millis();
    sendLora();

    lastSendTime = millis();
    auto timeItTook = lastSendTime - before;
    sendSerial(timeItTook);
    
    if(timeItTook > MIN_INTERVAL) {
      //Serial.println("Message took too long to send. Resetting the message.");
      outgoing = "err: msg-length";
    }
    
}

void sendLora() {
  digitalWrite(LED, LOW);
  LoRa.beginPacket();              // start packet
  LoRa.write(localAddress);        // add sender address
  LoRa.write(msgCount);            // add message ID
  LoRa.write(outgoing.length());   // add payload length
  LoRa.print(String(outgoing.c_str()));            // add payload
  LoRa.endPacket();                // finish packet and send it
  msgCount = (msgCount + 1 % 256); // increment message ID
}

void sendSerial(int time) {
  //Serial.print("sent: ");
  //Serial.print(String(outgoing.c_str()));

  //Serial.print(" addr: ");
  //Serial.print(localAddress);

  //Serial.print(" msg#: ");
  //Serial.print(msgCount);

  //Serial.print(" len: ");
  //Serial.print(outgoing.length());  
  //Serial.print(" time: ");
  //Serial.print(time);
  //Serial.println();  
}

void printScreen(String message)
{
  //Serial.println(message);
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

  String msg = "";
  msg += " ID: " + String(incomingMsgId);
  msg += " r: " + String(LoRa.packetRssi());
  msg += " s: " + String(LoRa.packetSnr());
  msg += " m: " + String(incoming);
  printScreen(msg);
  printScreen("               t: " + String(millis()));
  digitalWrite(LED, HIGH);
}
