#include <SPI.h>
#include <RH_RF95.h>

// Singleton instance of the radio driver
RH_RF95 rf95;
int index;
float frequency = 868.0;


#include <dht.h>
dht DHT;

void setup() 
{
  Serial.begin(9600);
  if (!rf95.init())
    Serial.println("init failed");
  // Setup ISM frequency
  rf95.setFrequency(frequency);
  // Setup Power,dBm
  rf95.setTxPower(13);

  // Setup Spreading Factor (6 ~ 12)
  rf95.setSpreadingFactor(7);
  
  // Setup BandWidth, option: 7800,10400,15600,20800,31200,41700,62500,125000,250000,500000
  //Lower BandWidth for longer distance.
  rf95.setSignalBandwidth(125000);
  
  // Setup Coding Rate:5(4/5),6(4/6),7(4/7),8(4/8) 
  rf95.setCodingRate4(5);

  // Setup Excel
  Serial.println("CLEARDATA");
  Serial.println("LABEL,Time,Distance,Bandwidth,Index,Success"); // 라벨 작성
  index = 1;
}

void loop()
{
  Serial.print("DATA,TIME,100M,125000,");
  Serial.print(String(index));
  index++;
  // Send a message to LoRa Server
  uint8_t data[] = "1";

  rf95.send(data, sizeof(data));
  rf95.waitPacketSent();
  // Now wait for a reply
  uint8_t buf[RH_RF95_MAX_MESSAGE_LEN];
  uint8_t len = sizeof(buf);

  if (rf95.waitAvailableTimeout(3000))
  { 
    // Should be a reply message for us now   
    if (rf95.recv(buf, &len))
   {
      Serial.print(",");
      Serial.println((char*)buf);
    }
    else
    {
      Serial.println(",0");
    }
  }
  else
  {
      Serial.println(",0");
  }
  delay(1000);
}


