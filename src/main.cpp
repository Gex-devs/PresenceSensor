#include <Arduino.h>


void setup()
{
  
}
void loop
{
  const int max_line_length = 80;
  static char buffer[max_line_length];
  while (available()) // Read serial
  {
    readline(read(), buffer, max_line_length);
  }
}
