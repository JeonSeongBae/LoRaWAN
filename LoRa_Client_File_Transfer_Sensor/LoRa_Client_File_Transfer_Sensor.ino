// LoRa Client Transfer Protocol
// By John Ganser
// 12/4/2017
//*Part of code sourced from LoRa Client Example
//////////////////////////////////////////////////////////////////////////////////////////


#include <SPI.h>
#include <RH_RF95.h>
#include <SD.h> //*****Added this library for SD card functionality
#include <DHT.h> //*****Added this library for DHT22 sensor functionality


// Singleton instance of the radio driver
RH_RF95 rf95;

//***********************Declare Variables*************************************

float frequency = 915.0;

int ChipSelect = 5; //*****Put the pin matching the microSD breakout board digital out pin

File myFileNew;//*****Used for the SD card
File myFileOld;

DHT dht;//*****Required for DHT sensor

String oldContents[100];//Define an array of size 10

int i = 0;//*****Used for SD card counter
unsigned int myCount = 0;//*****Used for SD card counter
String myString;//*****Used for SD card counter

int count = 0;//*****Counter for keeping track of minutes

int const initialSize = 1500;//array size
//uint8_t Array[];//define the array with an index of the variable initialSize 

uint8_t message;

void setup() 
{
  Serial.begin(9600);
  while (!Serial) ; // Wait for serial port to be available
  Serial.println("Starting LoRa client");
  if (!rf95.init())
    Serial.println("Initialization failed.");
  Serial.print("Initializing SD card...");
  if (!SD.begin(ChipSelect)) {
      Serial.println("Something when wrong on the SD card initialization. SD card inserted?");
      return;
        }
  Serial.println("Initialization successful.");

  SD.remove("TEST.TXT"); //Remove any files on SD card on startup
  SD.remove("TESTOLD.TXT"); 
  
  dht.setup(3); //***** Required for DHT22 sensor
  
  // Setup ISM frequency
  rf95.setFrequency(frequency);
  // Setup Power,dBm
  rf95.setTxPower(13);
  // Defaults after init are 434.0MHz, 13dBm, Bw = 125 kHz, Cr = 4/5, Sf = 128chips/symbol, CRC on
}
void loop()
{
  ////////////////////Data Collection/////////////////////////////////////////////////////////////////
  
  //*****Collect DHT22 sensor data and assign to declared variables***********************
  float humidity = dht.getHumidity();
  float temperature = dht.getTemperature();
  float tempF = temperature*1.8 + 32;
  //*****End collecting sensor data*******************************************************
  
  //*****Open and write to SD log file****************************************************
  myFileNew = SD.open("TEST.TXT", FILE_WRITE);
  //*****Keep track of how many times data has been recorded******************************
  count = myFileNew.size()/25;
  
  /////////////////////////////////////SD Card Managment/////////////////////////////////////
  
  if (count == 20) {//*****If count is greater than one month then purge collected data into a holder *****
 
    myFileNew = SD.open("TEST.TXT", FILE_READ); //Open "test.txt", very important to use "FILE_READ" as it reads from top of file instead of bottom
    Serial.println("Copying new contents from TEST.TXT to onboard array...");
    i = 0; //reset i
    for (;i < count; i++){      
      oldContents[i] = myFileNew.readStringUntil('\n'); //Copy the contents from the file to array 
    }
    Serial.println("Purging TEST.TXT file...");
    SD.remove("TEST.TXT"); //Remove the purged file from the SD card
    myFileOld = SD.open("TESTOLD.TXT", FILE_WRITE); //Open "TESTOLD.TXT
    Serial.println("Creating TESTOLD.TXT and transfering array to it...");
    i = 0 ; //reset i
    for (;i < count; i++){
      myFileOld.println(oldContents[i]); //Write the string to file from array
    }
    myFileOld.close(); //Close the file  
    Serial.println("Recreating TEST.TXT...");
    myFileNew = SD.open("TEST.TXT", FILE_WRITE);//Create testfilenew.txt again
    count = 0;//reset count
    Serial.println("File Transfer to Archive Complete!");
  }

  /////////////////////////////////////////////////////////////////////////////////////////////

  ///////////////////////Build data string from sensor input and write to SD Card//////////////
  String dataString = "1A," + String(count + 1) + "," + String(humidity) + "," + String(temperature)+ "," +  String(tempF);
  myFileNew.println(dataString);//*****String of data being written to log 
  Serial.println(dataString + "," + String(count) + "," + myFileNew.size()/25);//*****Print to serial line for debugging purposes
  myFileNew.close();
  //*****End writing to SD log file*******************************************************
  ////////////////////////////////////////////////////////////////////////////////////////////

  //////////////////////Listen for Gateway and Reply Back if Match/////////////////////////////\
  
  Serial.println("Listening for ping from gateway...");

  uint8_t ping[RH_RF95_MAX_MESSAGE_LEN];//Define message array and size
  uint8_t pingResponse[RH_RF95_MAX_MESSAGE_LEN];
  uint8_t payload[RH_RF95_MAX_MESSAGE_LEN];
  
  if (rf95.waitAvailableTimeout(1000)& count > 18){ //Wait for a message to be received or timeout after 1 second & the count of data collected is greater than or equal to 300
      rf95.recv(ping,sizeof(ping));
      String pingR = (char*)ping;
      Serial.println("Ping heard! " + pingR); //Write to serial that a ping was heard and write the value as well
    if (pingR == "IoTGateway"){  //If ping of "IoTGateway" is recieved then transmit back the ID of the node
        String response = "1A";
        response.getBytes(pingResponse,5);
        rf95.send(pingResponse,5);
        rf95.waitPacketSent(); //Wait until packet is completely sent
        String pingResponseS = (char*)pingResponse;
        Serial.println("Ping matches, sending pingresponse..." + pingResponseS);//Write to serial that ping matches and send response ID back
      if (rf95.waitAvailableTimeout(2000)){ //Wait for a response back or at least one second
            rf95.recv(ping,sizeof(ping));
            pingR = (char*)ping;
        if (pingR == "Transmit1A"){
              Serial.println("Transmit1A command recieved! Sending out collected data to gateway!");
              myFileOld = SD.open("TESTOLD.TXT", FILE_READ); //Open the stored data on the SD card for old data collected, starting at the top
    
              Serial.println("Sending collected data to gateway...");
              Serial.println("Sending old data first. Size: " + String (myFileOld.size()) );
              myCount = 0;
              for (myCount == 0; myCount < myFileOld.size();){ //Loop through the SD card file and send all stored data
                  myString =  myFileOld.readStringUntil('\n');//read string until the end of line character ('\n') is found then assign it to myString
                  myCount = myCount + myString.length()+ 1; //increment count
                  myString.getBytes(payload,sizeof(payload));
                  rf95.send(payload,sizeof(payload));
                  rf95.waitPacketSent(2000); 
                  delay(1000); 
                  Serial.println(String(myString) + "," + String(myCount));
              }
              Serial.println("Finished sending old data to gateway");
              SD.remove("TEXTOLD.TXT");//After sending get rid of the file
    
              myFileNew = SD.open("TEST.TXT", FILE_READ);

              Serial.println("Sending new data second. Size: " + String (myFileNew.size()) );
              myCount = 0;
              for (myCount == 0; myCount < myFileNew.size();){ //Loop through the SD card file and send all stored data
                  myString =  myFileNew.readStringUntil('\n');//read string until the end of line character ('\n') is found then assign it to myString
                  myCount = myCount + myString.length()+ 1; //increment count
                  uint8_t payload[25];
                  myString.getBytes(payload,sizeof(payload));
                  rf95.send(payload,sizeof(payload));
                  rf95.waitPacketSent(2000);    
                  Serial.println(String(myString) + "," + String(myCount));
                  delay(1000); 
              }
              Serial.println("Finished sending new data to gateway");
              SD.remove("TEXT.TXT");//After sending get rid of the file 
        }
      }
    }
  }
  
 delay(1000);//*****Set sampling rate to delay(ms)
 
}// End of void loop

