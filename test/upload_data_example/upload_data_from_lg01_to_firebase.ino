#include <Process.h>
#include <Console.h>
#include <SPI.h>
//If you use Dragino IoT Mesh Firmware, uncomment below lines.
//For product: LG01. 
#define BAUDRATE 115200

void setup()
{
  Bridge.begin(BAUDRATE);   // Initialize the Bridge
  Console.begin();
}

void loop()
{
  // Simulate Get Sensor value
  Console.println("===== Start =====");
  int sensor = random(10, 20);
  Console.println(sensor);
  Process p;
  p.runShellCommand("curl -k -X POST https://lorawan-53a5b.firebaseio.com/temperature.json -d '{ \"value\" : " + String(sensor) + "}'");
  while(p.running());
  Console.println("===== End ====="); 
  delay(2000);                
}
