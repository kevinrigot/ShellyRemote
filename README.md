Description:
====
Small device to be able to turn on/off and adjust the brightness of the connected light (Using Shelly Dimmer 2)

![Alt text](./3dprint/20220206_154200.jpg?raw=true "Printed device 1")

Features:
---------------
- Turn on/off; adjust brightness of a Shelly Dimmer 2 device.
- After 10 sec of inactivity, enter deep sleep.
- Push button to reset the ESP (=wake up)
- Voltage tracking:
    - Send the voltage level to MQTT
    - If battery voltage is lower than 3.5V; Blink 4 times and cutoff. Might not be necessary (behind the TP4056)
- Upload over the air

Components (Final version):
---------------
- Wemos D1 mini (ESP8266)
- Rotary encoder with push button
- Momentary self reset button with led
- TP4056
- Old battery from cellphone
- Mini switch
- Shelly Dimmer 2
- resistors

Environment variables:
---------------
- PIO_WIFI_SSID
- PIO_WIFI_PASSWORD
- PIO_MQTT_IP
- PIO_MQTT_USER
- PIO_MQTT_PASSWORD

OTA Instruction:
---------------
1. First upload the code. Make sure the upload_protocol and upload_port in platformio.ini are commented.
2. Upload over the air by enabling the upload_protocol and upload_port in platformio.ini. Make sure to have the correct ip.
3. To prevent deep sleep; publish a retained message on device_id + "/sleep_mode" = OFF
4. To go back in deep sleep mode; publish a retained message on device_id + "/sleep_mode" = ON

Remarks:
---------------
- OTA seems to be broken?

To do:
---------------
- Currently I have a reset button to wake up the ESP from deep sleep. Those 2 buttons are confusing. Find a way to have only the Rotary.

Schema/photos:
---------------
![Alt text](./schema/Schema_wemos_bb.png?raw=true "Breadboard - Prototype")
![Alt text](./schema/20220206_162307.jpg?raw=true "Photo")
![Alt text](./3dprint/3dprint-1.png?raw=true "3D print model")
![Alt text](./3dprint/3dprint-2.png?raw=true "3D print model 2")
![Alt text](./3dprint/20220206_154200.jpg?raw=true "Printed device 1")
![Alt text](./3dprint/20220206_154206.jpg?raw=true "Printed device 2")


Reference:
---------------
- https://randomnerdtutorials.com/esp8266-voltage-regulator-lipo-and-li-ion-batteries/