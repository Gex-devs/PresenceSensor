#include <Arduino.h>
#include <ld2410.h>
#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <UniversalTelegramBot.h>

#define BOT_TOKEN "5776848893:AAF1RwHhN8XctsWf8R-OJAmIPZKvjAQsnqE"

String ssid = "Ged-IOT";
String pass = "IOT@pass22";

// LD2410 ld2410;

const unsigned long BOT_MTBS = 1000; // mean time between scan messages

WiFiClientSecure secured_client;
UniversalTelegramBot bot(BOT_TOKEN, secured_client);
unsigned long bot_lasttime; // last time messages' scan has been done
bool Start = false;

void handleNewMessages(int numNewMessages)
{
  Serial.println("handleNewMessages");
  Serial.println(String(numNewMessages));

  for (int i = 0; i < numNewMessages; i++)
  {
    String chat_id = bot.messages[i].chat_id;
    String text = bot.messages[i].text;

    String from_name = bot.messages[i].from_name;
    if (from_name == "")
      from_name = "Guest";

    if (text == "/send_test_action")
    {
      bot.sendChatAction(chat_id, "typing");
      delay(4000);
      bot.sendMessage(chat_id, "Did you see the action message?");

      // You can't use own message, just choose from one of bellow

      // typing for text messages
      // upload_photo for photos
      // record_video or upload_video for videos
      // record_audio or upload_audio for audio files
      // upload_document for general files
      // find_location for location data

      // more info here - https://core.telegram.org/bots/api#sendchataction
    }

    if (text == "/start")
    {
      String welcome = "Welcome to Universal Arduino Telegram Bot library, " + from_name + ".\n";
      welcome += "This is Chat Action Bot example.\n\n";
      welcome += "/send_test_action : to send test chat action message\n";
      bot.sendMessage(chat_id, welcome);
    }
  }
}

void setup()
{
  // Serial.begin(256000);
  Serial.begin(9600); // Debug Serial on NodeMCU

  // Connect wifi
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, pass);

  while (WiFi.waitForConnectResult() != WL_CONNECTED)
  {
    delay(5000);
    ESP.restart();
  }

  // Setup OTA
  ArduinoOTA.begin();
  Serial.println("Ready");

  // Configure Time
  configTime(0, 0, "pool.ntp.org"); // get UTC time via NTP
  time_t now = time(nullptr);
  while (now < 24 * 3600)
  {
    Serial.print(".");
    delay(100);
    now = time(nullptr);
  }
  Serial.println(now);
  // ld2410.setBaudrate(256000);
}

void loop()
{
  ArduinoOTA.handle();
  // const int max_line_length = 80;
  // static char buffer[max_line_length];

  // while (Serial.available()) // Read serial
  // {
  //   // ld2410.readline(Serial.read(), buffer, max_line_length);
  //   // Serial1.println(ld2410.hasTarget);
  // }

  if (millis() - bot_lasttime > BOT_MTBS)
  {
    int numNewMessages = bot.getUpdates(bot.last_message_received + 1);

    while (numNewMessages)
    {
      Serial.println("got response");
      handleNewMessages(numNewMessages);
      numNewMessages = bot.getUpdates(bot.last_message_received + 1);
    }

    bot_lasttime = millis();
  }
}
