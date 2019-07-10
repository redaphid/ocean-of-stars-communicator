#include "heltec.h"
#include "esp_bt_main.h"
#include "esp_bt_device.h"
#define BAND    915E6  //you can set band here directly,e.g. 868E6,915E6

  
byte localAddress;     // address of this device
byte destination = 0xFF;      // destination to send to

String outgoing;              // outgoing message
String msgString = "";
byte msgCount = 0;            // count of outgoing messages
long lastSendTime = 0;        // last send time
int interval = 2000;          // interval between sends

void setup()
{
   //WIFI Kit series V1 not support Vext control
  Heltec.begin(true /*DisplayEnable Enable*/, true /*Heltec.LoRa Disable*/, true /*Serial Enable*/, true /*PABOOST Enable*/, BAND /*long BAND*/);
  LoRa.setSpreadingFactor(11);
  LoRa.setTxPowerMax(20);
  if(!initBluetooth()) {    
    sleep(999999);   
  }  
  getLocalAddress();
  printScreen("Welcome to RaveCom 3000 #" + String(localAddress));
  LoRa.onReceive(onReceive);
  LoRa.receive();
}

void getLocalAddress() {
  const uint8_t* point = esp_bt_dev_get_address();
  for(int i=0; i<6; i++) {
    localAddress = ((int)localAddress + (int)point[i]) % 256;
  }
}

void initScreen() {
  Heltec.display->init();
  Heltec.display->flipScreenVertically();
  Heltec.display->setFont(ArialMT_Plain_10);
  Heltec.display->display();
}

bool initBluetooth()
{
  if (!btStart()) {
    printScreen("Failed to initialize controller");
    return false;
  }
 
  if (esp_bluedroid_init() != ESP_OK) {
    printScreen("Failed to initialize bluedroid");
    return false;
  }
 
  if (esp_bluedroid_enable() != ESP_OK) {
    printScreen("Failed to enable bluedroid");
    return false;
  }
  return true;
 
}

void loop()
{  
  renderScreen();  
  if (millis() - lastSendTime > interval)
  {
    String message = "Hello World!";   // send a message
    sendMessage(message);
    lastSendTime = millis();            // timestamp the message
    interval = random(5000) + 1000;     // 2-3 seconds    
    LoRa.receive();                     // go back into receive mode
  }
}

void sendMessage(String outgoing)
{
  LoRa.beginPacket();                   // start packet
  LoRa.write(destination);              // add destination address
  LoRa.write(localAddress);             // add sender address
  LoRa.write(msgCount);                 // add message ID
  LoRa.write(outgoing.length());        // add payload length
  LoRa.print(outgoing);                 // add payload
  LoRa.endPacket();                     // finish packet and send it
  msgCount++;                           // increment message ID
}

void printScreen(String message) {
    Serial.println(message);
    msgString += message;
}

void renderScreen() {
  if(msgString == "") return;
  Heltec.display->clear();
  Heltec.display->setTextAlignment(TEXT_ALIGN_LEFT);
  Heltec.display->setFont(ArialMT_Plain_10);    
  Heltec.display->drawStringMaxWidth(0, 0, 128, msgString);    
  Heltec.display->display();
  msgString = "";
}

void printScreen() {
  printScreen("");
}

void onReceive(int packetSize)
{
  if (packetSize == 0) return;          // if there's no packet, return

  // read packet header bytes:
  int recipient = LoRa.read();          // recipient address
  byte sender = LoRa.read();            // sender address
  byte incomingMsgId = LoRa.read();     // incoming msg ID
  byte incomingLength = LoRa.read();    // incoming msg length

  String incoming = "";                 // payload of packet


  while (LoRa.available())             // can't use readString() in callback
  {
    incoming += (char)LoRa.read();      // add bytes one by one
  }

  if (incomingLength != incoming.length())   // check length for error
  {
    printScreen("error: message length does not match length");
    return;                             // skip rest of function
  }

  // if message is for this device, or broadcast, print details:
  String msg = "";
  msg += " ID: " + String(incomingMsgId);
  msg += " t: ";
  msg += (float)millis() / 1000 / 60;  
  msg += " r: " + String(LoRa.packetRssi());
  msg += " s: " + String(LoRa.packetSnr());
  printScreen(msg); 
}
