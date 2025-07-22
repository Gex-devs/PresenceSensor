#include <Arduino.h>
#include <ld2410.h>

LD2410 ld2410;

void setup()
{
  Serial.begin(256000);
  Serial1.begin(9600); // Debug Serial on NodeMCU
  
  ld2410.setBaudrate(256000);
}
void loop()
{
  const int max_line_length = 80;
  static char buffer[max_line_length];

  while (Serial.available()) // Read serial
  {
    ld2410.readline(Serial.read(), buffer, max_line_length);
    Serial1.println(ld2410.hasTarget);
  }
}
