/* 
.TFT_eSPI_memory
.animated GIF image stored in FLASH memory
*/
#define NORMAL_SPEED  // .Comment out for rame rate for render speed test

//. Load GIF library
//. from image to c file

#include <AnimatedGIF.h>
#include <SPI.h>
#include <TFT_eSPI.h>
#include "os_mac_win_x.h"

#define GIF_IMAGE os_mac_win_x   //. define name in .h file

TFT_eSPI tft = TFT_eSPI();
AnimatedGIF gif;

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
}

#ifdef NORMAL_SPEED //. Render at rate that is GIF controlled
void loop()
{
    //? Put your main code here, to run repeatedly:
    if (gif.open((uint8_t *)GIF_IMAGE, sizeof(GIF_IMAGE), GIFDraw))
    {
        Serial.printf("Successfully opened GIF; Canvas size = %d x %d\n", gif.getCanvasWidth(), gif.getCanvasHeight());
        tft.startWrite(); //. The TFT chip slect is locked low
        while (gif.playFrame(true, NULL))
        {
        yield();
        }
        gif.close();
        tft.endWrite(); //. Release TFT chip select for other SPI devices
    }
}
#else //? Test maximum rendering speed
void loop()
{
    long lTime = micros();
    int iFrames = 0;

    if (gif.open((uint8_t *)GIF_IMAGE, sizeof(GIF_IMAGE), GIFDraw))
    {
        tft.startWrite(); // For DMA the TFT chip slect is locked low
        while (gif.playFrame(false, NULL))
        {
        // Each loop renders one frame
        iFrames++;
        yield();
        }
        gif.close();
        tft.endWrite(); // Release TFT chip select for other SPI devices
        lTime = micros() - lTime;
        Serial.print(iFrames / (lTime / 1000000.0));
        Serial.println(" fps");
  }
}
#endif