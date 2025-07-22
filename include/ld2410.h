
#include <Arduino.h>

class LD2410
{
public:
    float *maxMovingDistanceRange;
    float *maxStillDistanceRange;
    int movingSensitivities[9] = {0};
    int stillSensitivities[9] = {0};
    float *noneDuration;

    long lastPeriodicMillis = millis();

    void setNumbers(float *maxMovingDistanceRange_, float *maxStillDistanceRange_, float *noneDuration_);

    void sendCommand(char *commandStr, char *commandValue, int commandValueLen);

    int twoByteToInt(char firstByte, char secondByte);

    void handlePeriodicData(char *buffer, int len);

    void handleACKData(char *buffer, int len);

    void readline(int readch, char *buffer, int len);

    void setConfigMode(bool enable);

    void queryParameters();

    void setEngineeringMode(bool enable);

    void setMaxDistancesAndNoneDuration(int maxMovingDistanceRange, int maxStillDistanceRange, int noneDuration);

    void factoryReset();

    void reboot();

    void setBaudrate(int index);

    private:
        bool lastCommandSuccess;
};