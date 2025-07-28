#include <ld2410.h>
#include <Arduino.h>

LD2410 ld2410;

void setup()
{
    ld2410.init();
}

void loop()
{
    const int max_line_length = 80;
    static char buffer[max_line_length];
    while (Serial.available())
    {
        ld2410.readline(Serial.read(), buffer, max_line_length);
        Serial.println(ld2410.hasTarget);
    }


}