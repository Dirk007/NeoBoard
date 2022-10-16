#include <mutex>

#include <ArduinoJson.h>
#include <EspMQTTClient.h>
#include <FastLED.h>
#include <WiFi.h>

#include "config.h"

// FTR: see `include/README.md`
EspMQTTClient client(WIFI_SSID, WIFI_PASSWORD, MQTT_BROKER_IP, MQTT_CLIENT_ID);

// GPIO to use on the ESP32 - connect this one with the data line of the led strip
#define DATA_PIN 26
// Maximum number of pixels supported - this is just a wild guess at the moment
#define MAX_PIXELS 2048
// Our internal refresh rate for the display as well es for the mqtt client
#define FPS 10

// Lock to protect our "frame buffer" and global variables
std::mutex lock;

uint32_t setup_pixels = 0;
uint32_t setup_brightness = 0;
bool init_done = false;
bool changes = false;

// The "frame buffer"
CRGB pixel_buffer[MAX_PIXELS];

void nack() {
    Serial.println("NACK");
    client.publish("neoboard/commandReply", "{\"status:\": \"error\"}");
}

void ack() {
    Serial.println("ACK");
    client.publish("neoboard/commandReply", "{\"status:\": \"ok\"}");
}

// Warning: only call this function in locked state
void cls() {
    // this should be safe enough
    memset(&pixel_buffer, 0, sizeof(pixel_buffer));
    // for(uint32_t i = 0; i < MAX_PIXELS; i++) {
    //     pixels[i] = CRGB{0, 0, 0};
    // }
}

void onBoardCommand(const String& topicStr, const String& message) {
    Serial.printf("NeoBoard command topic %s; %s\n", topicStr.c_str(), message.c_str());
    if(topicStr == "neoboard/command/start") {
        StaticJsonDocument<512> doc;
        deserializeJson(doc, message);

        uint32_t pixels = doc["pixels"];
        uint32_t brightness = doc["brightness"];

        {
            const std::lock_guard<std::mutex> glock(lock);
            FastLED.addLeds<NEOPIXEL, DATA_PIN>(pixel_buffer, pixels);
            setup_pixels = pixels;
            setup_brightness = brightness;
            cls();
        }

        ack();
    }

    if(topicStr == "neoboard/command/display") {
        StaticJsonDocument<2048> doc;
        deserializeJson(doc, message);

        bool clear = doc["clear"];
        uint32_t count = doc["count"];
        if(count > MAX_PIXELS) {
            Serial.printf("WARN: Too much pixels given (%u). Maximum supported: %u", count, MAX_PIXELS);
            count = MAX_PIXELS;
        }

        {
            const std::lock_guard<std::mutex> glock(lock);
            if(clear) {
                cls();
            }

            for(uint32_t i = 0; i < count; i++) {
                uint32_t pixel_id = doc["pixels"][i][0];
                uint32_t pixel_color = doc["pixels"][i][1];

                Serial.printf("Setting pixel %u to %u\n", pixel_id, pixel_color);
                pixel_buffer[pixel_id] = CRGB(pixel_color);
                changes = true;
            }
        }

        ack();
    }
}

void onConnectionEstablished() {
    Serial.println("MQTT Connection established");
    client.subscribe("neoboard/command/#", onBoardCommand);
    client.publish("neoboard/controller", "{\"status:\": \"connected\"}");
}

void setup() {
    Serial.begin(115200);
    while(!Serial) {
        delay(1);
    };

    Serial.println("\nStart...");
    client.setOnConnectionEstablishedCallback(onConnectionEstablished);
}

uint64_t frame = 0;
void loop() {
    client.loop();

    delay(1000 / FPS);
    {
        const std::lock_guard<std::mutex> glock(lock);
        if(setup_pixels == 0 || !changes) {
            return;
        }
        changes = false;

        Serial.printf("showing frame #%d...\n", ++frame);
        FastLED.show();
    }
}