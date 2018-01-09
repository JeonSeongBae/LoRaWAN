```
#include <RHReliableDatagram.h>
#include <RH_RF95.h>
#include <SPI.h>
#include <DS1302.h>
#include "SdFat.h"
#include "encrypt.h"

#define RTC_RST 3
#define RTC_IO 4
#define RTC_CLK 5
#define DHTPIN A0
#define MHPIN A2
#define SD_CS_PIN 8

#define CLIENT_ID 60
#define GATEWAY_ID 250

#define SAVE_INTERVAL (60000*1)
#define IDLE_INTERVAL (60000*10)

#define RH_RF95_MAX_MESSAGE_LEN 50

RH_RF95 driver;
RHReliableDatagram manager(driver, CLIENT_ID);
DS1302 rtc(RTC_RST,RTC_IO,RTC_CLK); // DS1302 rtc([CE/RST], [I/O], [CLOCK]);
SdFat SD;
ENCRYPT encrypt_decrypt;
File myFile;

unsigned char encryptkey[16] = {0x00,0x11,0x22,0x33,0x44,0x55,0x66,0x77,0x88,0x99,0xAA,0xBB,0xCC,0xDD,0xEE,0xFF};

bool detected = false;
bool flag = false;
short dataCnt = 0;
bool finish = false;
unsigned int save_cnt = 0;
unsigned int idle_cnt = 0;
char filename[7];

void setFilename()
{
  memset(filename, 0, sizeof(filename));
  snprintf(filename, sizeof(filename), "%02d.csv", idle_cnt%10);
  Serial.println(filename);
  
  if(SD.exists(filename))
  {
    if(!SD.remove(filename))
      Serial.println(F("remove file failed"));
  }
}

void setup() {
  rtc.halt(false);
  rtc.writeProtect(false);
  
  Serial.begin(9600);
  while (!Serial) ;

  SPI.begin();

  if (!SD.begin(SD_CS_PIN)) // 8-cs 11-si 12-so sck-13
    Serial.println(F("[fail] sd init"));

  pinMode(SD_CS_PIN, OUTPUT);
  digitalWrite(SD_CS_PIN, HIGH);

  SPI.setClockDivider(SPI_CLOCK_DIV8);

  if (!manager.init())
    Serial.println(F("[fail] rf init"));

  driver.setFrequency(915.0);

  pinMode(DHTPIN, OUTPUT);
  digitalWrite(DHTPIN, HIGH);

  Time t(2017, 12, 1, 22, 30, 00, 6);  // need to modify time
  rtc.time(t);

  setFilename();

  Serial.print(F("[start] client id : "));
  Serial.println(CLIENT_ID);
  Serial.println(F("[succ] set up"));
}

uint16_t calcByte(uint16_t crc, uint8_t b)
{
  crc = crc ^ (uint32_t)b << 8;
  for (uint32_t i = 0; i < 8; i++)
  {
    if ((crc & 0x8000) == 0x8000)
      crc = crc << 1 ^ 0x1021;
    else
      crc = crc << 1;
  }  
  return crc & 0xffff;
}

uint16_t CRC16(uint8_t *pBuffer,uint32_t length)
{
  uint16_t wCRC16=0;
  if (( pBuffer==0 )||( length==0 ))
    return 0;
  for ( uint32_t i = 0; i < length; i++)
    wCRC16 = calcByte(wCRC16, pBuffer[i]);
  return wCRC16;
}

byte read_dht_dat()
{
  byte result = 0;
  for(byte i=0; i< 8; i++)
  {
    while(digitalRead(DHTPIN)==LOW);
      delayMicroseconds(30);
    if (digitalRead(DHTPIN)==HIGH)
      result |=(1<<(7-i));
    while (digitalRead(DHTPIN)==HIGH);
  }
  return result;
}

void saveSensorData()
{
  if(detected && !finish)
    return;
    
  Time t = rtc.time();
  char dhtDat[5];

  myFile = SD.open(filename, FILE_WRITE);
  
  if(myFile) 
  {
    // Time
    myFile.print(t.yr);
    myFile.print(F("-"));
    myFile.print(t.mon);
    myFile.print(F("-"));
    myFile.print(t.date);
    myFile.print(F(" "));
    myFile.print(t.hr);
    myFile.print(F(":"));
    myFile.print(t.min);
    myFile.print(F(":"));
    myFile.print(t.sec);
    myFile.print(F(","));

    // Sensor - DHT
    digitalWrite(DHTPIN,LOW); // pull low data line to send signal.
    delay(30);                // Add a delay higher than 18ms so DHT11 can detect the start signal
    
    digitalWrite(DHTPIN,HIGH);
    delayMicroseconds(40);
    
    pinMode(DHTPIN,INPUT);
    
    digitalRead(DHTPIN);
    delayMicroseconds(80);    // Get DHT11 response , pull low data lineDHT11
    
    digitalRead(DHTPIN);
    delayMicroseconds(80);
    for (int i=0; i<5; i++)   // Ger Temperature and Humidity value
       dhtDat[i] = read_dht_dat();
           
    pinMode(DHTPIN,OUTPUT);
    digitalWrite(DHTPIN,HIGH);

    // humdity
    myFile.print(dhtDat[0], DEC);
    myFile.print(F("."));
    myFile.print(dhtDat[1], DEC);
    myFile.print(F(","));

    // temperature
    myFile.print(dhtDat[2], DEC);
    myFile.print(F("."));
    myFile.print(dhtDat[3], DEC);
    myFile.print(F(","));

    // Sensor - light
    myFile.println(analogRead (MHPIN));
    myFile.close();
    Serial.println(F("[succ] file save"));
  }
  else
    Serial.println(F("[fail] open file"));
}

void sendJoinReq()
{
  delay(random(100, 500));  // delay random time for avoiding the channel congestion

  uint8_t join[4] = {0}; // Construct a join message
  join[0] = 'J';
  join[1] = 'R';
  join[2] = ':';
  join[3] = CLIENT_ID;
  
  Serial.print(F("[send] "));
  Serial.print((char)join[0]);
  Serial.print((char)join[1]);
  Serial.print((char)join[2]);
  Serial.println(join[3], DEC);
  
  encrypt_decrypt.btea(join, sizeof(join), encryptkey); // encrypt the outgoing message
  manager.sendtoWait(join, sizeof(join), GATEWAY_ID); // send a Join Message
  flag = true;
}

void succJoin()
{
  Serial.println(F("[succ] Join"));
  detected = 1;
  flag = false; 
}

void listenServer()
{
  uint8_t buf[RH_RF95_MAX_MESSAGE_LEN];
  uint8_t len = sizeof(buf);
  uint8_t from;
  
  if (manager.recvfromAckTimeout(buf, &len, 100, &from))
  { 
    encrypt_decrypt.btea(buf, -len, encryptkey); // decode receive message
    
    //Get Broadcast message from Server, send a join request 
    if(buf[0] == 'B' && buf[1] == 'C' && buf[2] == ':' && buf[3] == GATEWAY_ID && from == GATEWAY_ID)                                                                                         
    {
      Serial.print(F("[recv] "));
      Serial.print((char)buf[0]);
      Serial.print((char)buf[1]);
      Serial.print((char)buf[2]);
      Serial.println(buf[3], DEC);
      delay(random(100, 500));
      sendJoinReq();
    }

    if(flag)
    {
      // successful if get join ACK, otherwise, listen for broadcast again. 
      if(buf[0] == 'J' && buf[1] == 'A'  && buf[2] == ':' && buf[3] == CLIENT_ID && from == GATEWAY_ID)  
      {
        Serial.print(F("[recv] "));
        Serial.print((char)buf[0]);
        Serial.print((char)buf[1]);
        Serial.print((char)buf[2]);
        Serial.println(buf[3], DEC);
        succJoin();
      }
    }
  }
  delay(100);
}

void sendData(int dataCnt)
{
  char data[50] = {0};
  data[0] = 'D';
  data[1] = 'S';
  data[2] = ':';
  data[3] = CLIENT_ID;
  data[4] = ':';
  data[5] = dataCnt;
  data[6] = ':';

  int dataLength = 7;
  int i = 0;
  char c;

  myFile = SD.open(filename);
  if (myFile) 
  {
    while (myFile.available())
    {
      if (i == dataCnt)
      {
        c = myFile.read();
        data[dataLength] = c;
        dataLength++;
        if (c == '\n')
          break;
      }
      else
      {
        c = myFile.read();
        if (c == '\n')
          i++;
      }
    }
    myFile.close();
//    Serial.println(F("[succ] file send"));
  }
  else
    Serial.println(F("[fail] open file"));

  if (dataLength == 7)
  {
    data[5] = -1;
  }
  uint16_t crcData = CRC16((unsigned char*)data, dataLength);

  Serial.print(F("[send] "));
  Serial.print((char)data[0]);
  Serial.print((char)data[1]);
  Serial.print((char)data[2]);
  Serial.print(data[3], DEC);
  Serial.print((char)data[4]);
  Serial.print(data[5], DEC);
  Serial.print((char)data[6]);
  for (i=7; i<dataLength; i++)
    Serial.print((char)data[i]);
  Serial.println("");
  
  unsigned char sendBuf[50] = {0};
  memcpy((char*)sendBuf, data, dataLength);
  sendBuf[dataLength] = (unsigned char)crcData;
  sendBuf[dataLength+1] = (unsigned char)(crcData>>8);
  
  encrypt_decrypt.btea(sendBuf, dataLength + 2, encryptkey);
  manager.sendtoWait(sendBuf, dataLength + 2, GATEWAY_ID);
}

void recvRequest()
{
  uint8_t buf[RH_RF95_MAX_MESSAGE_LEN];
  uint8_t len = sizeof(buf);
  uint8_t from;
  
  if (manager.recvfromAckTimeout(buf, &len, 100, &from))
  { 
    encrypt_decrypt.btea(buf, -len, encryptkey); // decode receive message
    
    // check if we receive a data request message
    if(buf[0] == 'D' && buf[1] == 'R' && buf[2] == ':' && buf[3] == CLIENT_ID && buf[4] == ':' && from == GATEWAY_ID)
    {
      Serial.print(F("[recv] "));
      Serial.print((char)buf[0]);
      Serial.print((char)buf[1]);
      Serial.print((char)buf[2]);
      Serial.print(buf[3], DEC);
      Serial.print((char)buf[4]);
      Serial.println(buf[5], DEC);
      
      sendData(buf[5]);
    }
    else if(buf[0] == 'D' && buf[1] == 'A' && buf[2] == ':' && buf[3] == CLIENT_ID && from == GATEWAY_ID)
    {
      Serial.print(F("[recv] "));
      Serial.print((char)buf[0]);
      Serial.print((char)buf[1]);
      Serial.print((char)buf[2]);
      Serial.println(buf[3], DEC);
      finish = true;
      setFilename();
    }
  }
  delay(100);
}

void loop() {
  if (millis()/SAVE_INTERVAL != save_cnt)
  {
    saveSensorData();
    save_cnt = millis()/SAVE_INTERVAL;
  }

  if(!detected)
    listenServer();
  else
  {
    if(!finish)
      recvRequest();
    else
    {
      if (millis()/IDLE_INTERVAL != idle_cnt)
      {
        detected = false;
        finish = false;
        idle_cnt = millis()/IDLE_INTERVAL;
      }
    }
  }
}
```
