```
#include <RHReliableDatagram.h>
#include <RH_RF95.h>
#include <SPI.h>
#include <Console.h>
#include <FileIO.h>
#include "encrypt.h"

#define BAUDRATE 115200
#define GATEWAY_ID 250

#define MAX_CLIENT 100
#define MAX_RESEND 5
#define BC_INTERVAL 10000

#define RH_RF95_MAX_MESSAGE_LEN 50

RH_RF95 driver;
RHReliableDatagram manager(driver, GATEWAY_ID);
ENCRYPT encrypt_decrypt;

unsigned char encryptkey[16]={0x00,0x11,0x22,0x33,0x44,0x55,0x66,0x77,0x88,0x99,0xAA,0xBB,0xCC,0xDD,0xEE,0xFF};
unsigned long start = 0;

int clients[MAX_CLIENT];
int clientNum = 0;
int resendCnt = 0;
uint8_t tmpClientId = -100;
uint8_t tmpDataCnt = -100;

void setup() {
  Bridge.begin(BAUDRATE);
  Console.begin();
  FileSystem.begin();
  while (!Console) ; // Wait for serial port to be available
  
  if (!manager.init())
    Console.println(F("[fail] rf init"));

  driver.setFrequency(915.0);

  start = millis();

  for(int i=0; i<MAX_CLIENT; i++)
    clients[i] = -100;

  Console.println(F("[succ] set up"));
}

void sendBroadcast()
{
  uint8_t broadcast[4] = {0}; //Broadcast Message
  broadcast[0] = 'B';
  broadcast[1] = 'C';
  broadcast[2] = ':';  
  broadcast[3] = GATEWAY_ID;
            
  encrypt_decrypt.btea(broadcast, sizeof(broadcast), encryptkey); //Encrypt the broadcast message
  Console.println(F("[send] Broadcast msg")); 
  manager.sendtoWait(broadcast, sizeof(broadcast), RH_BROADCAST_ADDRESS);
}

void sendJoinAck(uint8_t clientId)
{
  uint8_t JoinAck[4] = {0}; // Join ACK message
  JoinAck[0] = 'J';
  JoinAck[1] = 'A';
  JoinAck[2] = ':';
  JoinAck[3] = clientId;
  
  Console.print(F("[send] "));
  Console.print((char)JoinAck[0]);
  Console.print((char)JoinAck[1]);
  Console.print((char)JoinAck[2]);
  Console.println(JoinAck[3], DEC);
  
  encrypt_decrypt.btea(JoinAck, sizeof(JoinAck), encryptkey); // encrypt the outgoing message
  manager.sendtoWait(JoinAck, sizeof(JoinAck), clientId); // send a Join Message
}

void acceptClient()
{
  uint8_t buf[RH_RF95_MAX_MESSAGE_LEN];
  uint8_t len = sizeof(buf);
  uint8_t from;
  uint8_t clientId;
  
  if (manager.recvfromAckTimeout(buf, &len, 100, &from))
  { 
    encrypt_decrypt.btea(buf, -len, encryptkey); // decode receive message 
    if (buf[0] == 'J' && buf[1] == 'R' && buf[2] == ':') // Get a Join request, Store the Client ID
    {
      Console.print(F("[recv] "));
      Console.print((char)buf[0]);
      Console.print((char)buf[1]);
      Console.print((char)buf[2]);
      Console.println(buf[3], DEC);
    
      clientId = buf[3];
      sendJoinAck(clientId);

      Console.println(clients[clientId]);
      if (clients[clientId] == -100)
      {
        clients[clientId] = 0;
        clientNum ++;
        Console.print(F("[succ] Join : "));
        Console.println(clientId, DEC);
      }
      else if (clients[clientId] == -1)
      {
        clients[clientId] = 0;
        clientNum ++;
        Console.print(F("[succ] Join : "));
        Console.println(clientId, DEC);
      }
    }
  }
  delay(100);
}

void requestData(uint8_t clientId, uint8_t dataCnt)
{
  uint8_t query[6] = {0};
  query[0] = 'D';
  query[1] = 'R';
  query[2] = ':';
  query[3] = clientId;
  query[4] = ':';
  query[5] = dataCnt;
  
  Console.print(F("[send] "));
  Console.print((char)query[0]);
  Console.print((char)query[1]);
  Console.print((char)query[2]);
  Console.print(query[3], DEC);
  Console.print((char)query[4]);
  Console.println(query[5], DEC);

  if(tmpClientId == clientId && tmpDataCnt == dataCnt)
    resendCnt++;
  else
  {
    tmpClientId = clientId;
    tmpDataCnt = dataCnt;
  }

  if(resendCnt > MAX_RESEND)
  {
    clients[clientId] = -100;
    resendCnt = 0;
  }

  encrypt_decrypt.btea(query, sizeof(query), encryptkey);
  manager.sendtoWait(query, sizeof(query), clientId);
  delay(100);
}

int recvData(uint8_t clientId)
{
  uint8_t buf[RH_RF95_MAX_MESSAGE_LEN];
  uint8_t len = sizeof(buf);
  uint8_t from;
  
  if (manager.recvfromAckTimeout(buf, &len, 100, &from))
  { 
    encrypt_decrypt.btea(buf, -len, encryptkey);
    if (buf[0] == 'D' && buf[1] == 'S' && buf[2] == ':' && from == clientId)
    {
      Console.print(F("[recv] "));
      Console.print((char)buf[0]);
      Console.print((char)buf[1]);
      Console.print((char)buf[2]);
      Console.print(buf[3], DEC);
      Console.print((char)buf[4]);
      Console.print(buf[5], DEC);
      Console.print((char)buf[6]);
      for(int i=7; i<len-2; i++)
        Console.print((char)buf[i]);

      if(buf[5] != 0xff)
      {
        char data[len-7-1];
        memset(data, 0, len-7-1);
        strncpy(data, (char*)&buf[7], len-7-2);
  
        char date[len-7-1];
        strncpy(date, data, len-7-1);
        int year = atoi(strtok(date,"-"));
        int month = atoi(strtok(NULL, "-"));
        int day = atoi(strtok(NULL, "-"));
        
        // saveData
        char filename[40];
        memset(filename, 0, 40);
        snprintf(filename, sizeof(filename), "/mnt/sda1/data/client%02d_%d_%d_%d.csv", clientId, year, month, day);
  
        File dataFile = FileSystem.open(filename, FILE_APPEND);
        if (dataFile) 
        {
          dataFile.print(data);
          dataFile.close();
        }
        else 
        {
          Console.println(F("error save data"));
        } 
      }
      
      clients[clientId] = buf[5]+1;
    }
  }
  return buf[5];
}

void sendDataAck(uint8_t clientId)
{
  uint8_t ack[4] = {0};
  ack[0] = 'D';
  ack[1] = 'A';
  ack[2] = ':';
  ack[3] = clientId;
  
  Console.print(F("[send] "));
  Console.print((char)ack[0]);
  Console.print((char)ack[1]);
  Console.print((char)ack[2]);
  Console.println(ack[3], DEC);
  
  encrypt_decrypt.btea(ack, sizeof(ack), encryptkey);
  manager.sendtoWait(ack, sizeof(ack), clientId);

  clients[clientId] = -1;
  delay(1000);
}

void getData()
{
  for(int i=0; i<MAX_CLIENT; i++)
  {
    if(clients[i] >= 0)
    {
      while(1)
      {
        requestData(i, clients[i]);
        if(recvData(i) == uint8_t(-1))
        {
          sendDataAck(i);
          break;
        }
        if(clients[i] < 0)
          break;
      }
    }
  }
}

void loop() 
{
  if(millis() - start > BC_INTERVAL)
  {
    sendBroadcast();
    start = millis();
  }
  acceptClient();
  getData();
}
```
