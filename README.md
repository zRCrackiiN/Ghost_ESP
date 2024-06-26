# Ghost ESP Firmware

Ghost ESP is a firmware designed for ESP32 microcontrollers, offering a comprehensive WiFi and BLE testing toolkit. It provides various features that should be used responsibly

## Features

- WiFi scanning
- Station scanning
- SSID spamming
- BLE spamming for major brands (iOS, Windows, Android, Samsung)
- DIAL connect to interact with DIAL-enabled devices (YouTube, Netflix, Roku)
- Rainbow LED mode

## Board Support

Ghost ESP firmware supports the following ESP32 boards:
- ESP32 Wroom
- ESP32 S2
- ESP32 C3
- ESP32 C6
- ESP32 S3

Additional board support can be added depending on PlatformIO support and configuration in the main project's platformio.ini file..

## Notice

Please note that the BLE spam feature may not work on the ESP32 WROOM due to limited resources, which cause the ESP to crash.

## Discord 
[Ghost_ESP](https://discord.gg/vrYfcZfTK9)

## Acknowledgments

- A big thank you to [JustCallMeKoKo](https://github.com/justcallmekoko/ESP32Marauder) for providing a base to familiarize myself with the ESP32.
- Gratitude to [thibauts](https://github.com/thibauts/node-castv2-client) for the initial insights and inspiration on the CastV2 protocol.
- Appreciation to [MarcoLucidi01](https://github.com/MarcoLucidi01/ytcast/tree/master/dial) for the motivation and groundwork on the DIAL protocol, which helped in porting it to the ESP32.
