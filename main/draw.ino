#include <SPI.h>
// #define SPI_FREQUENCY  80000000
// #define SPI_READ_FREQUENCY 80000000
#include <TFT_eSPI.h>      // Hardware-specific library

TFT_eSPI tft = TFT_eSPI(); // Invoke custom library

//------------------------------------------------------------------------------------------

void setup() {
  // Use serial port
  Serial.begin(115200);

  // Initialise the TFT screen
  tft.init();

  // Set the rotation to the orientation you wish to use in your project before calibration
  // (the touch coordinates returned then correspond to that rotation only)
  tft.setRotation(0);

  // Calibrate the touch screen and retrieve the scaling factors
  // touch_calibrate();

  uint16_t calData[5] = { 214, 3723, 287, 3577, 7 };
  tft.setTouch(calData);

  // Clear the screen
  tft.fillScreen(TFT_BLACK);
  tft.drawCentreString("Touch screen to test!",tft.width()/2, tft.height()/2, 2);
}

//------------------------------------------------------------------------------------------

void loop(void) {
  uint16_t x = 0, y = 0; // To store the touch coordinates

  // Pressed will be set true is there is a valid touch on the screen
  bool pressed = tft.getTouch(&x, &y);

  // Draw a white spot at the detected coordinates
  if (pressed) {
    tft.fillCircle(x, y, 2, TFT_WHITE);
    Serial.print("x,y = ");
    Serial.print(x);
    Serial.print(",");
    Serial.println(y);
  }
}

