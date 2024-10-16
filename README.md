# DIY Digital Dice with Sound And ESP32
## Description

### **simple project making DIY Digital Dice with Sound using DFplayer mini, MPU6050, OLED SSD1306, and ESP32**

- [x] ### NEW!! .. Now the ESP32 is connected to MQTT Broker that can control the Roll via it and receive the new roll number. 

## Connection:

| Pins ( modules --> ESP32 Pins) | Description                                                  |
| ------------------------------ | ------------------------------------------------------------ |
| RX ----> GPIO16                | RX of DFplayer mini To ESP32 GPIO pin For UART connection    |
| TX ----> GPIO17                | TX of DFplayer mini To ESP32 GPIO pin For UART connection    |
| VCC--> 5volt                   | For powering Any module                                      |
| GND--> Ground                  | For powering Any module                                      |
| SDA --> GPIO21                 | Serial Data Pin For I2C communication between OLED, MPU6050,and ESP32 |
| SCL --> GPIO22                 | Serial Clock Pin For I2C communication between OLED, MPU6050,and ESP32 |
| Buttons1 --> GPIO12            | Button's first pin connect to GND and the other pin of the button to GPIO pin |
| Buttons1 --> GPIO13            | Button's first pin connect to GND and the other pin of the button to GPIO pin |
