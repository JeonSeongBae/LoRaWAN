// LoRa Server Transfer Protocol
// By John Ganser
// 12/4/2017
// *Part of code sourced from LoRa Server Example
//////////////////////////////////////////////////////////////////////////////////////////

#define BAUDRATE 115200
 
#include <Console.h>
#include <SPI.h>
#include <RH_RF95.h>
#include <FileIO.h>

// Singleton instance of the radio driver
RH_RF95 rf95;

int led = A2;
float frequency = 915.0;
String ping = "IoTGateway";
String response = "Transmit1A";

int previoustime = 0;
int currenttime = 0;

void setup() 
{
  pinMode(led, OUTPUT);     
  Bridge.begin(BAUDRATE);
  Console.begin();
  FileSystem.begin();//******added from logger_usb_flash script
  File dataFile = FileSystem.open("/mnt/sda1/data/datalog.csv", FILE_APPEND);
  while (!Console) ; // Wait for console port to be available
  Console.println("Start Sketch");
  if (!rf95.init())
    Console.println("init failed");
    // Setup ISM frequency
    rf95.setFrequency(frequency);
    // Setup Power,dBm
    rf95.setTxPower(13);
    // Defaults BW Bw = 125 kHz, Cr = 4/5, Sf = 128chips/symbol, CRC on
    Console.print("Listening on frequency: ");
    Console.println(frequency);
}

void loop()
{
File dataFile = FileSystem.open("/mnt/sda1/data/datalog.csv", FILE_APPEND);

  /////////////////////////Send ping out to let node pick up///////////////////
  uint8_t payload[11];
  ping.getBytes(payload,11);
  rf95.send(payload,11);
  rf95.waitPacketSent(200);
  Console.println("Ping sent");
  ////////////////////////Listen for any reponses//////////////////////////////
  uint8_t pingResponse[10];
  if (rf95.waitAvailableTimeout(2000)){
      rf95.recv(pingResponse,10);
      String responseping = (char*)pingResponse;
      Console.println("Ping response heard!" + responseping);
  /////////////////////////////////////////////////////////////////////////////
  
  ////////////////////////Does response match node ID?/////////////////////////
    if (responseping == "1A") { //If 1A node ID then send out response of "Transmit1A"
        Console.println("Response match");
        response.getBytes(payload,sizeof(payload));
        rf95.send(payload,sizeof(payload));
        rf95.waitPacketSent(2000);
        Console.println("Response " + response + "sent!");
        Receive(); //call Receive function to recieve data from node
    } 
  }
  ////////////////////////////////////////////////////////////////////////////
  
  previoustime  = millis()-previoustime; //Keep track of time
  Serial.println(String(previoustime) + String(millis()));//Debug check for time
}   



void Receive(){
  
   Console.println("Debug check - 'recieve' function");
   int count = 40; //counter to keep track of incoming messages

   int i = 0;
   
   for (i == 0; i < count; i++){; //Loop through recieving function while count is less than 300
      
      if (rf95.waitAvailableTimeout(3000)){ //Loop while message available or at least 3 seconds has elapsed
   
        uint8_t buf[RH_RF95_MAX_MESSAGE_LEN];
        uint8_t len = sizeof(buf);
        Console.println("Recieving data from 1A...");
        
          if (rf95.recv(buf, &len)){;
            Console.println("Message recieved from 1A - writing to inserted flash drive");
            digitalWrite(led, HIGH);
            RH_RF95::printBuffer("request: ", buf, len);
            String dataString;
            dataString += getTimeStamp();
            dataString += " , ";
            dataString += rf95.lastRssi();
            dataString += " , ";
            dataString += rf95.lastSNR();
            dataString += " ,1A, ";
            dataString += (char*)buf;
   
            Console.print("got request: ");
            Console.println((char*)buf);
            Console.print("RSSI: ");
            Console.print(rf95.lastRssi(), DEC);
            Console.print(" , SNR: ");
            Console.print(rf95.lastSNR());
            String response = "1A";
            Console.println(" , Node ID: " + response);
            digitalWrite(led, LOW);
  
                  //****************** Writing to USB Flash *************************************
                  File dataFile = FileSystem.open("/mnt/sda1/data/datalog.csv", FILE_APPEND);    
                  if (dataFile) {
                    dataFile.print(dataString);
                    delay(1000);
                  }  
                  // if the file isn't open, pop up an error:
                  else {
                    Console.println("error opening datalog.csv");
                  } 
                  //***************** End Writing to USB Flash **********************************           
          }
      }
   }
}
  
String getTimeStamp(){
  String result;
  Process time;
  // date is a command line utility to get the date and the time 
  // in different formats depending on the additional parameter 
  time.begin("date");
  time.addParameter("+%D-%T");  // parameters: D for the complete date mm/dd/yy
                                //             T for the time hh:mm:ss    
  time.run();  // run the command

  // read the output of the command
    while(time.available()>0) {
      char c = time.read();
      if(c != '\n')
        result += c;
    }   
  return result;
}


