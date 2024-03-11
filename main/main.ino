#include <SPI.h>
#include <WiFi.h>
#include <SPIFFS.h>
#include <FreeRTOS.h>
#include <TFT_eSPI.h> 
#include <driver/i2s.h>
#include <HTTPClient.h>
#include <AnimatedGIF.h>
#include <JPEGDecoder.h>
#include <WebSocketClient.h>

#include "os.h"
#include "draw.h"
#include "messenger.h"
#include "main_screen.h"


#define I2S_WS 2
#define I2S_SD 42
#define I2S_SCK 1
#define I2S_PORT I2S_NUM_0
#define I2S_SAMPLE_RATE   (16000)
#define I2S_SAMPLE_BITS   (16)
#define I2S_READ_LEN      (16 * 1024)
#define RECORD_TIME       (4) //Seconds
#define I2S_CHANNEL_NUM   (1)
#define FLASH_RECORD_SIZE (I2S_CHANNEL_NUM * I2S_SAMPLE_RATE * I2S_SAMPLE_BITS / 8 * RECORD_TIME)

#define GIF_IMAGE os

#define NORMAL_SPEED
#define minimum(a,b)     (((a) < (b)) ? (a) : (b))

File file;
AnimatedGIF gif;

HTTPClient client;
WiFiClient WFclient;
String receivedData;

TaskHandle_t touch_voice_rec;

WebSocketClient webSocketClient;
SemaphoreHandle_t dataSemaphore;

TFT_eSPI tft = TFT_eSPI();

const char filename[] = "/recordingE2.wav";
const int headerSize = 44;
bool isWIFIConnected;

const char* ssid     = "Reaws iPhone";
const char* password = "++++++++";

char host[] = "172.20.10.8";
char path[] = "";

int flash_wr_size = 0;

int page = 1;
int count = 0;
int current_page = 0;
uint16_t x = 0, y = 0; // To store the touch coordinates
int x_ = (tft.height()  - 240) / 2;
int y_ = (tft.width() - 320) / 2;
bool pressed;
bool isPressed = false;
bool message_status = false;

void GIFDraw(GIFDRAW *pDraw); //* GIFDraw file

void setup() {
  // put your setup code here, to run once:
    Serial.begin(115200);
    dataSemaphore = xSemaphoreCreateMutex(); 

    tft.begin();
    tft.setRotation(0); //. set screen to landscape
    tft.fillScreen(TFT_BLACK);

    gif.begin(BIG_ENDIAN_PIXELS);
    if (gif.open((uint8_t *)GIF_IMAGE, sizeof(GIF_IMAGE), GIFDraw))
    {
        tft.startWrite(); //. The TFT chip slect is locked low
        while (gif.playFrame(true, NULL)){
            yield();
        }
        gif.close();
        tft.endWrite(); //. Release TFT chip select for other SPI devices
    }

    connect_wifi();
    connect_websocket(34567);

    SPIFFSInit();
    i2sInit();
    // uint16_t calData[5] = { 198, 3563, 259, 3540, 7 };
    // tft.setTouch(calData);
    //? ------- function name ---------------------- stack --- order --
    xTaskCreate(touch_screen,"touch screen"          ,4096,NULL,4,NULL);
    xTaskCreate(check_page,"check page"              ,2048,NULL,3,NULL);
    xTaskCreate(DataTaskreceive,"receiveimg message" ,4096,NULL,3,NULL);
    // xTaskCreate(check_pressbutton,"check_pressbutton",2048,NULL,3,NULL);
    // xTaskCreate(voice_record,"recording voice"       ,2048,NULL,2,NULL);
    

    // xTaskCreate(i2s_adc, "i2s_adc", 1024 * 4, NULL, 1, NULL);
    // delay(500);
}

void loop() {
  // put your main code here, to run repeatedly:
}

void connect_wifi() {

    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        Serial.print(WiFi.status()+" ");
        delay(500);
        Serial.print(".");
    }

    Serial.println("");
    Serial.println("WiFi connected");
    isWIFIConnected = true;
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());

}

void SPIFFSInit(){
    if(!SPIFFS.begin(true)){
        Serial.println("SPIFFS initialisation failed!");
        while(1) yield();
    }
    // const char filename2[] = "/recording.wav";

    SPIFFS.remove(filename);
    file = SPIFFS.open(filename, FILE_WRITE);
    if(!file){
        Serial.println("File is not available!");
    }
    byte header[headerSize];
    wavHeader(header, FLASH_RECORD_SIZE);

    file.write(header, headerSize);
    listSPIFFS();
}

void i2sInit(){
  i2s_config_t i2s_config = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
    .sample_rate = I2S_SAMPLE_RATE,
    .bits_per_sample = i2s_bits_per_sample_t(I2S_SAMPLE_BITS),
    .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
    .communication_format = i2s_comm_format_t(I2S_COMM_FORMAT_I2S | I2S_COMM_FORMAT_I2S_MSB),
    .intr_alloc_flags = 0,
    .dma_buf_count = 64,
    .dma_buf_len = 1024,
    .use_apll = 1
  };

  i2s_driver_install(I2S_PORT, &i2s_config, 0, NULL);

  const i2s_pin_config_t pin_config = {
    .bck_io_num = I2S_SCK,
    .ws_io_num = I2S_WS,
    .data_out_num = -1,
    .data_in_num = I2S_SD
  };

  i2s_set_pin(I2S_PORT, &pin_config);
}

void i2s_adc_data_scale(uint8_t * d_buff, uint8_t* s_buff, uint32_t len)
{
    uint32_t j = 0;
    uint32_t dac_value = 0;
    for (int i = 0; i < len; i += 2) {
        dac_value = ((((uint16_t) (s_buff[i + 1] & 0xf) << 8) | ((s_buff[i + 0]))));
        d_buff[j++] = 0;
        d_buff[j++] = dac_value * 256 / 2048;
    }
}

void i2s_adc(void *arg)
{
    int i2s_read_len = I2S_READ_LEN;
    size_t bytes_read;

    char* i2s_read_buff = (char*) calloc(i2s_read_len, sizeof(char));
    uint8_t* flash_write_buff = (uint8_t*) calloc(i2s_read_len, sizeof(char));

    i2s_read(I2S_PORT, (void*) i2s_read_buff, i2s_read_len, &bytes_read, portMAX_DELAY);
    i2s_read(I2S_PORT, (void*) i2s_read_buff, i2s_read_len, &bytes_read, portMAX_DELAY);
    
    //? Change to Dynamic fileszie
    //? Button for check
    // byte header[headerSize];
    // wavHeader(header, FLASH_RECORD_SIZE);

    // file.write(header, headerSize);
    // listSPIFFS();
   
    Serial.print("==================== ");
    Serial.print("isPrseesd = ");
    Serial.print(isPressed);
    Serial.println(" ====================");
    Serial.println(" *** Recording Start *** ");
    while (flash_wr_size < FLASH_RECORD_SIZE) {
        //read data from I2S bus, in this case, from ADC.
        i2s_read(I2S_PORT, (void*) i2s_read_buff, i2s_read_len, &bytes_read, portMAX_DELAY);
        //example_disp_buf((uint8_t*) i2s_read_buff, 64);
        //save original data from I2S(ADC) into flash.
        i2s_adc_data_scale(flash_write_buff, (uint8_t*)i2s_read_buff, i2s_read_len);
        file.write((const byte*) flash_write_buff, i2s_read_len);
        flash_wr_size += i2s_read_len;

        Serial.println(flash_wr_size); //. log for size file in Bytes
        ets_printf("Sound recording %u%%\n", flash_wr_size * 100 / FLASH_RECORD_SIZE);
        ets_printf("Never Used Stack Size: %u\n", uxTaskGetStackHighWaterMark(NULL));
    }
    file.close();

    free(i2s_read_buff);
    i2s_read_buff = NULL;
    free(flash_write_buff);
    flash_write_buff = NULL;
    
    listSPIFFS();
    //. Prepare this process can send anytime
    if(isWIFIConnected){
      uploadFile();
      flash_wr_size = 0;
    }
}

void example_disp_buf(uint8_t* buf, int length)
{
    printf("======\n");
    for (int i = 0; i < length; i++) {
        printf("%02x ", buf[i]);
        if ((i + 1) % 8 == 0) {
            printf("\n");
        }
    }
    printf("======\n");
}

void wavHeader(byte* header, int wavSize){
    header[0] = 'R';
    header[1] = 'I';
    header[2] = 'F';
    header[3] = 'F';
    unsigned int fileSize = wavSize + headerSize - 8;
    header[4] = (byte)(fileSize & 0xFF);
    header[5] = (byte)((fileSize >> 8) & 0xFF);
    header[6] = (byte)((fileSize >> 16) & 0xFF);
    header[7] = (byte)((fileSize >> 24) & 0xFF);
    header[8] = 'W';
    header[9] = 'A';
    header[10] = 'V';
    header[11] = 'E';
    header[12] = 'f';
    header[13] = 'm';
    header[14] = 't';
    header[15] = ' ';
    header[16] = 0x10;
    header[17] = 0x00;
    header[18] = 0x00;
    header[19] = 0x00;
    header[20] = 0x01;
    header[21] = 0x00;
    header[22] = 0x01;
    header[23] = 0x00;
    header[24] = 0x80;
    header[25] = 0x3E;
    header[26] = 0x00;
    header[27] = 0x00;
    header[28] = 0x00;
    header[29] = 0x7D;
    header[30] = 0x01;
    header[31] = 0x00;
    header[32] = 0x02;
    header[33] = 0x00;
    header[34] = 0x10;
    header[35] = 0x00;
    header[36] = 'd';
    header[37] = 'a';
    header[38] = 't';
    header[39] = 'a';
    header[40] = (byte)(wavSize & 0xFF);
    header[41] = (byte)((wavSize >> 8) & 0xFF);
    header[42] = (byte)((wavSize >> 16) & 0xFF);
    header[43] = (byte)((wavSize >> 24) & 0xFF);
  
}

void listSPIFFS(void) {
    Serial.println(F("\r\nListing SPIFFS files:"));
    static const char line[] PROGMEM =  "=================================================";

    Serial.println(FPSTR(line));
    Serial.println(F("  File name                              Size"));
    Serial.println(FPSTR(line));

    fs::File root = SPIFFS.open("/");
    if (!root) {
        Serial.println(F("Failed to open directory"));
        return;
    }
    if (!root.isDirectory()) {
        Serial.println(F("Not a directory"));
        return;
    }

    fs::File file = root.openNextFile();
    while (file) {

        if (file.isDirectory()) {
        Serial.print("DIR : ");;
        String fileName = file.name();
        Serial.print(fileName);
        } else {
        String fileName = file.name();
        Serial.print("  " + fileName);
        // File path can be 31 characters maximum in SPIFFS
        int spaces = 33 - fileName.length(); // Tabulate nicely
        if (spaces < 1) spaces = 1;
        while (spaces--) Serial.print(" ");
        String fileSize = (String) file.size();
        spaces = 10 - fileSize.length(); // Tabulate nicely
        if (spaces < 1) spaces = 1;
        while (spaces--) Serial.print(" ");
        Serial.println(fileSize + " bytes");
        }

        file = root.openNextFile();
    }

    Serial.println(FPSTR(line));
    Serial.println();
    delay(1000);
}

void uploadFile(){
    file = SPIFFS.open(filename, FILE_READ);
    if(!file){
        Serial.println("FILE IS NOT AVAILABLE!");
        return;
    }
    String filename = "recordingE2";
    uint8_t* data_filename = (uint8_t*)filename.c_str();

    Serial.println("===> Upload FILE to Node.js Server");
    //! This is Dynamics IP if Change WiFi IP must be Change same
    client.begin("http://172.20.10.8:8888/uploadAudio"+filename);
    client.addHeader("Content-Type", "audio/wav");
    int httpResponseCode = client.sendRequest("POST",&file, file.size());
    Serial.println();
    Serial.print("httpResponseCode : ");
    Serial.println(httpResponseCode);
    // Send Message with websocket protocal 
    if(httpResponseCode == 200){
        String response = client.getString();
        Serial.println("==================== Transcription ====================");
        Serial.println(response);
        Serial.println("====================      End      ====================");
    }else{
        Serial.println("Error");
    }
    file.close();
    client.end(); //! ระวังการปิด Client 
    sendDataTask("E2");


}

void connect_websocket(int port){
    webSocketClient.path = path;
    webSocketClient.host = host;

    if (WFclient.connect(host, port)) {
        Serial.println("Connected");
    } else {
        Serial.println("Connection failed.");
    }

    if (webSocketClient.handshake(WFclient)) {
        Serial.println("Handshake successful");
    } else {
        Serial.println("Handshake failed.");
    }
}

void DataTaskreceive(void *parameter) {

    //. receive data must be async function
    while (1) {
        String data;
        webSocketClient.getData(data);

        if (data.length() > 0) {
            xSemaphoreTake(dataSemaphore, portMAX_DELAY);
            receivedData = data;
            Serial.print("Received data: ");
            Serial.println(receivedData); //. receive message from other client 
            xSemaphoreGive(dataSemaphore);
            message_status = true;
        }
        vTaskDelay(10 / portTICK_PERIOD_MS);


        //. if receive message = "Please Try again" must user send message again
    }
}

void sendDataTask(String message) {
    webSocketClient.sendData(message);

    //. Send name device for send voice message
}

//+ ====================================== SCREEN AREA ==========================================

void touch_screen(void* args)
{
    while (true)
    {
      //! รีเซ็ท ค่า x,y ใหม่ตลอด
      x = 0; y = 0;
      pressed = tft.getTouch(&x, &y); //. รับค่า touch screen x,y
      // Serial.print("x,y : ");
      // Serial.print(x);
      // Serial.print(",");
      // Serial.println(y);
      // เซ็ทหน้า
      delay(10);
      set_page();
      if (page == 2 and x > 35 and y > 45 and x < 75 and y < 85){
          // Serial.println("in touch box");
          isPressed = true;
          delay(10);
      }
      else{
          isPressed = false;
          delay(10);
      }

      if (isPressed and page == 2)
      {
        i2s_adc(NULL);
        delay(10);
      }
      // Serial.println(message_status);
      if (message_status and page == 2)
      {
        receive_message(NULL);
        delay(10);
      }

      if (page == 3){
        tft.fillCircle(x, y, 2, TFT_WHITE);
        delay(10);
      }
      // Serial.print("isPressed = ");
      // Serial.println(isPressed);

    }
}



void check_page(void* args)
{
    //. page 1 คือ หน้า main screen page 2 คือ ในแอป
    while(true)
    {
        // Serial.println("in check page");
        // Serial.print("page = ");
        // Serial.println(page);
        //. กดเข้าแอป (กำหนดกรอบแอป กับ เช็คหน้าว่าอยู่ถูกหน้ามั้ย)
        delay(10);
        if (x > 15 and x < 90 and y > 135 and y < 220 and page == 1 )
        {
            page = 2;
            delay(10);
        }
        //. ออกจากแอป
        if (x > 0 and x <= 40 and y > 225 and page == 2)
        {
            page = 1;
            delay(10);
        }
        if (x > 130 and x < 190 and y > 130 and y < 200 and page == 1)
        {
          uint16_t calData[5] = { 198, 3563, 259, 3540, 7 };
          tft.setTouch(calData);
          page = 3;
          delay(10);
        }
    }
}

//. โชว์หน้าจอ พวก main screen, app
void set_page()
{
    //. เช็คก่อนว่า page ปัจจุบัน เท่ากับ page ล่าสุดหรือป่าว ถ้าไม่ก็เปลี่ยนหน้า
    if (current_page != page)
    {
        if (page == 1)
        {
          Serial.println("====================== in page 1 ======================");
          drawArrayJpeg(main_screen, sizeof(main_screen), x_, y_);
        }
        else if (page == 2)
        {
          Serial.println("====================== in page 2 ======================");
          drawArrayJpeg(messenger, sizeof(messenger), x_, y_);
        }
        else if (page == 3)
        {
          Serial.println("====================== in page 2 ======================");
          drawArrayJpeg(draw, sizeof(draw), x_, y_);
        }
    }
    //. ให้ page ปัจจุบัน เท่ากับ page ล่าสุด
    current_page = page;
}

// //. ฟังก์ชันไว้ใช้อัดเสียง
// void voice_record(void* args)
// {
//   Serial.println("=================== in voice record ===================");
//   Serial.print("==================== ");
//   Serial.print("isPrseesd = ");
//   Serial.print(isPressed);
//   Serial.println(" ====================");
//     while (true){
//         if (isPressed)
//         {
//           // vTaskSuspendAll();
//           // xTaskCreatePinnedToCore(touch_screen_, "touch_screen", 1024 * 4, NULL, 1, NULL,1);
//           xTaskCreate(i2s_adc,"i2s_adc",8192,NULL,20,NULL);
//           // i2s_adc(NULL);
//           delay(500);
//             //! Prepre this Area for Sound Recording
//         }
//     }
// }


//. ใช้โชว์ข้อความ chat
void receive_message(void* args)
{
  tft.setCursor(145, 40 + 20*count, 2);
  tft.setTextColor(TFT_BLACK,TFT_LIGHTGREY);  tft.setTextSize(0.5);
  tft.println(receivedData);
  count++;
  message_status = false;
  if (count == 7){
    drawArrayJpeg(messenger, sizeof(messenger), x_, y_);
    count = 0;
  }

}

//!==================================== ข้างล่างนี้ ไว็ใช้ display รูป บนจอ ===========================================

void drawArrayJpeg(const uint8_t arrayname[], uint32_t array_size, int xpos, int ypos) {

    int x = xpos;
    int y = ypos;

    JpegDec.decodeArray(arrayname, array_size);
    renderJPEG(x, y);
}

void renderJPEG(int xpos, int ypos) {

    //. retrieve infomration about the image
    uint16_t *pImg;
    uint16_t mcu_w = JpegDec.MCUWidth;
    uint16_t mcu_h = JpegDec.MCUHeight;
    uint32_t max_x = JpegDec.width;
    uint32_t max_y = JpegDec.height;

    //. Jpeg images are draw as a set of image block (tiles) called Minimum Coding Units (MCUs)
    //. Typically these MCUs are 16x16 pixel blocks
    //. Determine the width and height of the right and bottom edge image blocks
    uint32_t min_w = minimum(mcu_w, max_x % mcu_w);
    uint32_t min_h = minimum(mcu_h, max_y % mcu_h);

    //. save the current image block size
    uint32_t win_w = mcu_w;
    uint32_t win_h = mcu_h;

    //. record the current time so we can measure how long it takes to draw an image
    uint32_t drawTime = millis();

    //. save the coordinate of the right and bottom edges to assist image cropping
    //. to the screen size
    max_x += xpos;
    max_y += ypos;

    //. read each MCU block until there are no more
    while (JpegDec.read()) {
        //. save a pointer to the image block
        pImg = JpegDec.pImage ;

        //. calculate where the image block should be drawn on the screen
        int mcu_x = JpegDec.MCUx * mcu_w + xpos;  // Calculate coordinates of top left corner of current MCU
        int mcu_y = JpegDec.MCUy * mcu_h + ypos;

        //. check if the image block size needs to be changed for the right edge
        if (mcu_x + mcu_w <= max_x) win_w = mcu_w;
        else win_w = min_w;

        //. check if the image block size needs to be changed for the bottom edge
        if (mcu_y + mcu_h <= max_y) win_h = mcu_h;
        else win_h = min_h;

        //. copy pixels into a contiguous block
        if (win_w != mcu_w)
        {
            uint16_t *cImg;
            int p = 0;
            cImg = pImg + win_w;
            for (int h = 1; h < win_h; h++)
            {
                p += mcu_w;
                for (int w = 0; w < win_w; w++)
                {
                *cImg = *(pImg + w + p);
                cImg++;
                }
            }
        }

        //. calculate how many pixels must be drawn
        uint32_t mcu_pixels = win_w * win_h;

        tft.startWrite();

        //. draw image MCU block only if it will fit on the screen
        if (( mcu_x + win_w ) <= tft.width() && ( mcu_y + win_h ) <= tft.height())
        {
            //. Now set a MCU bounding window on the TFT to push pixels into (x, y, x + width - 1, y + height - 1)
            tft.setAddrWindow(mcu_x, mcu_y, win_w, win_h);
            //. Write all MCU pixels to the TFT window
            while (mcu_pixels--) {
                //. Push each pixel to the TFT MCU area
                tft.pushColor(*pImg++);
            }
        }
        else if ( (mcu_y + win_h) >= tft.height()) JpegDec.abort(); // Image has run off bottom of screen so abort decoding
        tft.endWrite();
    }
    drawTime = millis() - drawTime;
}