// TFT_eSPI_memory
//
// animated GIF image stored in FLASH memory
//
#define NORMAL_SPEED  // Comment out for rame rate for render speed test

// Load GIF library
#include <AnimatedGIF.h>
AnimatedGIF gif;
// from image to c file
#include "os.h"


#define GIF_IMAGE os   //  define name in .h file


#include <SPI.h>
#include <TFT_eSPI.h>

TFT_eSPI tft = TFT_eSPI();

void GIFDraw(GIFDRAW *pDraw);

void setup() {
  Serial.begin(115200);

  tft.begin();

  tft.init();
  uint16_t calData[5] = { 214, 3723, 287, 3577, 7 };
  tft.setTouch(calData);

#ifdef USE_DMA
  tft.initDMA();
#endif
  tft.setRotation(0);
  tft.fillScreen(TFT_BLACK);

  gif.begin(BIG_ENDIAN_PIXELS);
  if (gif.open((uint8_t *)GIF_IMAGE, sizeof(GIF_IMAGE), GIFDraw))
{
  Serial.printf("Successfully opened GIF; Canvas size = %d x %d\n", gif.getCanvasWidth(), gif.getCanvasHeight());
  tft.startWrite(); // The TFT chip slect is locked low
  while (gif.playFrame(true, NULL))
  {
    yield();
  }
  gif.close();
  tft.endWrite(); // Release TFT chip select for other SPI devices
}
  tft.fillScreen(TFT_BLACK);
}


void loop(){
  uint16_t x_ = 0, y_ = 0; // To store the touch coordinates
  // int x = (tft.width()  - 300) / 2 - 1;
  // int y = (tft.height() - 300) / 2 - 1;
  // drawArrayJpeg(mic_test, sizeof(mic_test), x, y); // Draw a jpeg image stored in memory at x,y
  // Pressed will be set true is there is a valid touch on the screen
  bool pressed = tft.getTouch(&x_, &y_);

  // Draw a white spot at the detected coordinates
  if (pressed) {
    if (x_ < 160){
      tft.fillScreen(TFT_PURPLE);
    }
    else if (x_ >= 160){
      tft.fillScreen(TFT_BLUE);
    }
    tft.fillCircle(x_, y_, 2, TFT_WHITE);
    Serial.print("x,y = ");
    Serial.print(x_);
    Serial.print(",");
    Serial.println(y_);
  }

}