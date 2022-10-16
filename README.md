# NeoBoard

A simple controller for NeoPixel LED strips that can be managed via MQTT, interfaced by a ESP32 WiFi connection.

## Code setup

This project uses the awesome [PlatformIO](https://platformio.org/).

Before flashing this piece of code onto your ESP32, adapt [the config](include/README.md) to match your local configuration. Also you want to change the data pin you want to use. But this is optional if you just want to use GPIO26 like me.

## Notes

This project is WIP and explicitly not made to be fit for any use outside my personal little project (animating a [NCC 1701-E plexiglas master systems board installation](https://www.lcarsstudio.com/product/star-trek-schematic-lcars-uss-enterprise-1701-e-large-plexiglas-panel-print) depending on gitlab build states, cpu utilization, etc.). 

Also, the firmware built will be without OTA firmware update functionality. I made this step to have enough free space on the ESP32 for this fat firmware (Wifi, MQTT, JSON, FastLED, ...) which would otherwise maybe not fit.

## Contributing
Any help to make it more stable and cool are welcome. Just file a PR.

## Messages

In general: send the given Messages using the given topic to the MQTT broker on which the ESP32 listens for commands. You need to do that for example with [mosquitto_pub](https://mosquitto.org/man/mosquitto_pub-1.html) from any other device. This is how the project is intended to work. 

### Setup

To set up the ESP32 to match your installation, you need to initialize the ESP32 with some infos like how many LEDs (pixels) are on your strip.

The maximum supported number of LEDs is 2048 at the moment.

Topic: `neoboard/command/start`
Payload: 
```json
{
    "pixels": 200,
    "brightness": 50
}
```

where `pixels` is the amount of pixels / LEDs on the strip, and `brightness` some currently unused post-modifier for brightness on submitted values.

### Display

Used to set/unset pixels on the LED strip.

Topic: `neoboard/command/display`
Payload: 
```json
{
    "clear": true,
    "count": 2,
    "pixels": [
        [1, 44561], [2, 255]
    ]
}
```

where `clear` can be set to `true` to set all pixels to black before applying the new pixels. If set to `false`, existing pixels will be not be altered.

`count` must be set at the moment to advertise the number of pixels in the `pixels` array. Every `pixel` in the `pixels` array is itself a 2 entry sized array itself containing the sequential position of the pixel to set (0 = first LED, 1 = second, ...) followed by the color colde.

### Examples

Setup an installation with 50 LED strip (most likely you will be using much more):
```bash
mosquitto_pub -h 192.168.1.104 -p 1883 -t neoboard/command/start -m '\
{
  "pixels": 50,
  "brightness": 50
}'
```

Switch on some LED and toggle a red one every second:

```bash
while [ 1 ]; do mosquitto_pub -h 192.168.1.104 -p 1883 -t neoboard/command/display -m '{"clear":true,
  "count": 6,
  "pixels": [
    [9,10000], [8,10000], [7,0], [16,278190080], [20,0], [21,0]
  ]
}' && sleep 1 && mosquitto_pub -h 192.168.1.104 -p 1883 -t neoboard/command/display -m '{"clear":true,
  "count": 6,
  "pixels": [
    [9,10000], [8,10000], [7,0], [16,278190080], [20,13172736], [21,13172736]
  ]
}' && sleep 1; done
```

## License
[MIT](LICENSE)