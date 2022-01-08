Reference:
- https://randomnerdtutorials.com/esp8266-voltage-regulator-lipo-and-li-ion-batteries/

Components:
- NodeMcu (ESP8266)
- MCP1700-3302E/TO (3.3V LDO Voltage regulator)
- Rotary encoder with push button
- Shelly Dimmer 2
- Led, resistors

OTA Instruction:
1. First upload the code. Make sure the upload_protocol and upload_port in platformio.ini are commented.
2. Upload over the air by enabling the upload_protocol and upload_port in platformio.ini. Make sure to have the correct ip.

Schema:
![Alt text](./schema/Schema_bb_png?raw=true "Breadboard")
![Alt text](./schema/20220103_230530.jpg?raw=true "Photo")