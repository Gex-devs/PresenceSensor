
#include <Arduino.h>

#define CHECK_BIT(var, pos) (((var) >> (pos)) & 1)

enum TargetState
{
    TARGET = 0x00,
    MOVINGTARGET = 0x01,
    STILTTARGET = 0x02,
    MOVINGSTILTTARGET = 0x03
};

class LD2410
{
public:
    float *maxMovingDistanceRange;
    float *maxStillDistanceRange;
    
    bool hasMovingTarget;
    bool hasStillTarget;

    int  MovingTargetDistance;
    int MovingTargetEnergy;
    
    int StillTargetDistance;
    int StillTargetEnergy;
    
    int DetectDistance;

    int movingSensitivities[9] = {0};
    int stillSensitivities[9] = {0};

    float *noneDuration;

    bool hasTarget;
    bool movingTarget;
    bool stillTarget;

    long lastPeriodicMillis = millis();

    void setNumbers(float *maxMovingDistanceRange_, float *maxStillDistanceRange_, float *noneDuration_)
    {

        maxMovingDistanceRange = maxMovingDistanceRange_;
        maxStillDistanceRange = maxStillDistanceRange_;
        noneDuration = noneDuration_;
    }

    void sendCommand(char *commandStr, char *commandValue, int commandValueLen)
    {
        lastCommandSuccess = false;

        // frame start bytes
        Serial.write(0xFD);
        Serial.write(0xFC);
        Serial.write(0xFB);
        Serial.write(0xFA);
        // length bytes
        int len = 2;
        if (commandValue != nullptr)
            len += commandValueLen;
        Serial.write(lowByte(len));
        Serial.write(highByte(len));
        // command string bytes
        Serial.write(commandStr[0]);
        Serial.write(commandStr[1]);
        // command value bytes
        if (commandValue != nullptr)
        {
            for (int i = 0; i < commandValueLen; i++)
            {
                Serial.write(commandValue[i]);
            }
        }
        // frame end bytes
        Serial.write(0x04);
        Serial.write(0x03);
        Serial.write(0x02);
        Serial.write(0x01);
        delay(50);
    }

    int twoByteToInt(char firstByte, char secondByte)
    {
        return (int16_t)(secondByte << 8) + firstByte;
    }

    void handlePeriodicData(char *buffer, int len)
    {
        if (len < 12)
            return; // 4 frame start bytes + 2 length bytes + 1 data end byte + 1 crc byte + 4 frame end bytes
        if (buffer[0] != 0xF4 || buffer[1] != 0xF3 || buffer[2] != 0xF2 || buffer[3] != 0xF1)
            return; // check 4 frame start bytes
        if (buffer[7] != 0xAA || buffer[len - 6] != 0x55 || buffer[len - 5] != 0x00)
            return; // data head=0xAA, data end=0x55, crc=0x00
        /*
          Data Type: 6th byte
          0x01: Engineering mode
          0x02: Normal mode
        */
        char dataType = buffer[5];
        /*
          Target states: 9th byte
          0x00 = No target
          0x01 = Moving targets
          0x02 = Still targets
          0x03 = Moving+Still targets
        */
        char stateByte = buffer[8];
        hasTarget = stateByte;
        /*
          Reduce data update rate to prevent home assistant database size glow fast
        */
        long currentMillis = millis();
        if (currentMillis - lastPeriodicMillis < 1000)
            return;
        lastPeriodicMillis = currentMillis;

        hasMovingTarget = (CHECK_BIT(stateByte, 0)); 
        hasStillTarget = (CHECK_BIT(stateByte, 1));

        /*
          Moving target distance: 10~11th bytes
          Moving target energy: 12th byte
          Still target distance: 13~14th bytes
          Still target energy: 15th byte
          Detect distance: 16~17th bytes
        */
        int newMovingTargetDistance = twoByteToInt(buffer[9], buffer[10]);
        if (MovingTargetDistance  != newMovingTargetDistance)
            MovingTargetDistance = newMovingTargetDistance;

        int newMovingTargetEnergy = buffer[11];
        if (MovingTargetEnergy != newMovingTargetEnergy)
            MovingTargetEnergy = newMovingTargetEnergy;

        int newStillTargetDistance = twoByteToInt(buffer[12], buffer[13]);
        if (StillTargetDistance  != newStillTargetDistance)
            StillTargetDistance = newStillTargetDistance;

        int newStillTargetEnergy = buffer[14];
        if (StillTargetEnergy  != newStillTargetEnergy)
            StillTargetEnergy = buffer[14];

        int newDetectDistance = twoByteToInt(buffer[15], buffer[16]);
        if (DetectDistance  != newDetectDistance)
            DetectDistance = newDetectDistance;

        if (dataType == 0x01)
        { // engineering mode
          // todo: support engineering mode data
        }
        
    }

    void handleACKData(char *buffer, int len)
    {
        if (len < 10)
            return;
        if (buffer[0] != 0xFD || buffer[1] != 0xFC || buffer[2] != 0xFB || buffer[3] != 0xFA)
            return; // check 4 frame start bytes
        if (buffer[7] != 0x01)
            return;
        if (twoByteToInt(buffer[8], buffer[9]) != 0x00)
        {
            lastCommandSuccess = false;
            return;
        }
        lastCommandSuccess = true;
        switch (buffer[6])
        {
        case 0x61: // Query parameters response
        {
            if (buffer[10] != 0xAA)
                return; // value head=0xAA
            /*
              Moving distance range: 13th byte
              Still distance range: 14th byte
            */
            *maxMovingDistanceRange = buffer[12];
            *maxStillDistanceRange = buffer[13];
            /*
              Moving Sensitivities: 15~23th bytes
              Still Sensitivities: 24~32th bytes
            */
            for (int i = 0; i < 9; i++)
            {
                movingSensitivities[i] = buffer[14 + i];
            }
            for (int i = 0; i < 9; i++)
            {
                stillSensitivities[i] = buffer[23 + i];
            }
            /*
              None Duration: 33~34th bytes
            */
            *noneDuration = twoByteToInt(buffer[32], buffer[33]);
        }
        break;
        default:
            break;
        }
    }

    void readline(int readch, char *buffer, int len)
    {
        static int pos = 0;

        if (readch >= 0)
        {
            if (pos < len - 1)
            {
                buffer[pos++] = readch;
                buffer[pos] = 0;
            }
            else
            {
                pos = 0;
            }
            if (pos >= 4)
            {
                if (buffer[pos - 4] == 0xF8 && buffer[pos - 3] == 0xF7 && buffer[pos - 2] == 0xF6 && buffer[pos - 1] == 0xF5)
                {
                    handlePeriodicData(buffer, pos);
                    pos = 0; // Reset position index ready for next time
                }
                else if (buffer[pos - 4] == 0x04 && buffer[pos - 3] == 0x03 && buffer[pos - 2] == 0x02 && buffer[pos - 1] == 0x01)
                {
                    handleACKData(buffer, pos);
                    pos = 0; // Reset position index ready for next time
                }
            }
        }
        return;
    }

    void setConfigMode(bool enable)
    {
        char cmd[2] = {enable ? 0xFF : 0xFE, 0x00};
        char value[2] = {0x01, 0x00};
        sendCommand(cmd, enable ? value : nullptr, 2);
    }

    void queryParameters()
    {
        char cmd_query[2] = {0x61, 0x00};
        sendCommand(cmd_query, nullptr, 0);
    }

    void setEngineeringMode(bool enable)
    {
        char cmd[2] = {enable ? 0x62 : 0x63, 0x00};
        sendCommand(cmd, nullptr, 0);
    }

    void setMaxDistancesAndNoneDuration(int maxMovingDistanceRange, int maxStillDistanceRange, int noneDuration)
    {
        char cmd[2] = {0x60, 0x00};
        char value[18] = {0x00, 0x00, lowByte(maxMovingDistanceRange), highByte(maxMovingDistanceRange), 0x00, 0x00, 0x01, 0x00, lowByte(maxStillDistanceRange), highByte(maxStillDistanceRange), 0x00, 0x00, 0x02, 0x00, lowByte(noneDuration), highByte(noneDuration), 0x00, 0x00};
        sendCommand(cmd, value, 18);
        queryParameters();
    }

    void factoryReset()
    {
        char cmd[2] = {0xA2, 0x00};
        sendCommand(cmd, nullptr, 0);
    }

    void reboot()
    {
        char cmd[2] = {0xA3, 0x00};
        sendCommand(cmd, nullptr, 0);
        // not need to exit config mode because the ld2410 will reboot automatically
    }

    void setBaudrate(int index)
    {
        char cmd[2] = {0xA1, 0x00};
        char value[2] = {index, 0x00};
        sendCommand(cmd, value, 2);
    }

private:
    bool lastCommandSuccess;
};