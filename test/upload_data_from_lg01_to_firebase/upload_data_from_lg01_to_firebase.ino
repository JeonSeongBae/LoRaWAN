#include <Process.h>
#include <SPI.h>
#include <Console.h>

void setup()
{
  Bridge.begin();   // Initialize the Bridge
  Console.begin();
}

void loop()
{
  // Simulate Get Sensor value
  Console.println("===== Start =====");
  int sensor = random(10, 20); 
  Process p;
  p.runShellCommand("curl -k -X POST https://lg01-ba3b9.firebaseio.com/temperature.json -d '{ \"value\" : " + String(sensor) + "}'");
  while(p.running());
  Console.println("===== End ====="); 
  delay(2000);                
}
