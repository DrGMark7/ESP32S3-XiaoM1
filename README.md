# Xiao-M1
**Xiao-M1** is a thoughtfully designed Arduino project aimed at facilitating communication with deaf individuals. The working principle is converts voice to text or allows you to draw a picture to describe something, providing a helpful tool for more accessible and convenient communication with the deaf community.

## Application
- Chat application
- Drawing application

## List of hardware devices used
- 2 x ESP32-S3 Devkit
- 2 x ILI9341 2.4-inch screen LCD TFT with touch pen
- 2 x INMP441
- 2 x Active Buzzer

### Libraries
- SPI.h
- WiFi.h
- SPIFFS.h
- FreeRTOS.h
- driver/i2s.h
- [TFT_eSPI.h](https://github.com/Bodmer/TFT_eSPI)
- [AnimatedGIF.h](https://github.com/bitbank2/AnimatedGIF)
- [JPEGDecoder.h](https://github.com/Bodmer/JPEGDecoder)
- [HTTPClient.h](https://github.com/espressif/arduino-esp32/tree/master/libraries/HTTPClient)
- [WebSocketClient.h](https://github.com/brandenhall/Arduino-Websocket/tree/master)

## Directories
```
📱Xiao-M1
┃
┣━━ 📁client
┃   ┣>  client.ino          # main program
┃   ┣>  GIFDraw.ino         # function to play GIF
┃   ┣>  os.h                # GIF in c array
┃   ┣>  main_screen.h       # main screen image in c array
┃   ┣>  messenger.h         # in message app screen in c array
┃   ┣>  draw.h              # in drawing app in c array
┃
┣━━ 📁server
┃   ┣>  httpserver.js       # server for receive voice message
┃   ┣>  wsserver.py         # server for send text message
┃
┣>  LICENSE               # license file
┃
┗>  README.md
```

## Other tools
- [covert image to c array](https://notisrac.github.io/FileToCArray/) use for covert image to c array
- [Edit GIF file](https://ezgif.com) use for edit GIF file
