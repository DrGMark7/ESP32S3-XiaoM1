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

}
