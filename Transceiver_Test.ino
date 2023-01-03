#include "RF24.h"
#include <nRF24L01.h> 
/*1 way transmitting test*/

#define isTransmitter  false

//stored address
const byte address = 00001;
RF24 radio(1,2); //CE CSN
struct DataPacket{
  byte a; //packet ID
  byte b; // data of some kind
  byte debug = 0; //for packet loss calculation
};
DataPacket mydata;//to keep self check
DataPacket dataReceived; //data read from radio

//bits of data
byte numPacketSent;
int packetID;
int numData;

//connection data
#if isTransmitter == false
byte expectedPID = 1; //expected packet id 1-255;
byte numPacketReceived = 0; //how many did we receive out of 255? (dont forget 0 is debug)
byte lastPacketReceived = 0;
float packetsLost;
int cycles = 0; //how many cycles have happened (still working on a name);
float connectionStatus = 100;
int totalPacketsLost;
#endif




//Function to configure the radio
void configureRadio() {
  radio.begin();//enable radio, trigger the chip
  radio.setPALevel(RF24_PA_LOW);//set power amplification to low, one level lower is PA_MIN
  //radio.setAutoAck(false);//disable auto ack
  //radio.disableCRC();//disable CRC, cyclic redundant checks?

  #if isTransmitter == true
  radio.openWritingPipe(address);//writing pipe with address 00001
  //Stop the radio listening for data, we're about to transmit
  radio.stopListening();
  #else
  radio.openReadingPipe(1,address);
  radio.startListening();
  #endif
}
void setup()
{
  configureRadio();
  #if isTransmitter == false
  Serial.begin(19200);
  delay(1000);
  Serial.println("Receiver Side");
  Serial.println("====================");
  Serial.println("====================");
  Serial.println("====================");
  #endif
  delay(2000);//to popup on screen
}

void loop()
{
  #if isTransmitter == true
  for(int i = 1; i <=256; i++)
  {
    packetID = i;
    if(i == 256)packetID == 0;//debug packet
    if(i == 0)
    {
     //this is were we send a debug packet, for now just send serial message 
    }else
    {
      //send a packet
      bool sentOk = false;
      while(!sentOk)
      {
        mydata.a = packetID;
        mydata.b = 255-i; //idk some random data
        mydata.debug = 0;
        radio.writeFast(&mydata,sizeof(DataPacket));
        sentOk = radio.txStandBy(250);//wait up to 250 millis to send, return 1 if succesful
        delay(10);
      }
    }
  }
  #else
  uint32_t failTimer = millis();
  while(!radio.available())
  {
    if(millis()-failTimer > 10)
    {
      Serial.println("Response Timed Out");
    }
  }
  while(radio.available())
  {
    radio.read(&dataReceived, sizeof(DataPacket));
    radio.flush_rx();
    if(dataReceived.a != 0){
    Serial.println("Received Data Packet");
    Serial.print("Expected PID -- ");
    Serial.println(expectedPID);
    Serial.print("Received PID -- ");
    Serial.println(dataReceived.a);
    if(dataReceived.a != expectedPID)
    {
      //means packet was lost and continuity error in stream
      Serial.println("PID Error");
      Serial.print("Packets Probably Lost -- ");
      packetsLost += abs(lastPacketReceived-dataReceived.a); //total counter
      Serial.println(lastPacketReceived-dataReceived.a); //count for just this instance of loss
    }
    lastPacketReceived = dataReceived.a;
    expectedPID = lastPacketReceived +=1; //whats the next packet supposed to be
    Serial.print(dataReceived.a);
    Serial.println("*");
    Serial.print(dataReceived.b);
    Serial.println("*");
    Serial.print("Cycles -- ");
    Serial.println(cycles);
    if(cycles>0)
    {
      Serial.print("Connection Integrity -- ");
      Serial.println(connectionStatus*100);
    }
    Serial.println("====================");
    numPacketReceived +=1;
    }else
    {
      //we received a 0 debug packet
      Serial.println("Received Debug Packet");
      Serial.println("Packets Sent -- 255");
      Serial.print("Packets Received -- ");
      Serial.println(numPacketReceived);
      Serial.print("Connection Integrity -- ");
      connectionStatus = ((255.0-float(packetsLost)) / 255.0);
      Serial.print((connectionStatus*100));
      Serial.println("%");
      Serial.println("====================");
      numPacketReceived = 0;
      cycles+=1;
      totalPacketsLost += packetsLost;//add to counter
      packetsLost = 0;//reset counter
      expectedPID+=1; //to skip over 0
    }
    Serial.println("============");
    Serial.print("Total Packets Lost -- ");
    Serial.println(totalPacketsLost);
  }
  #endif
}
