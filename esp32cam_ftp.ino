/*
 * ESP32cam_ftp
 * Blog: http://www.gsampallo.com/blog/?p=686
 * Twitter: @gsampallo.com
 */
#include "esp_camera.h"
#include "soc/soc.h"           // Disable brownour problems
#include "soc/rtc_cntl_reg.h"  // Disable brownour problems
#include "driver/rtc_io.h"
#include <WiFi.h>
#include <WiFiClient.h>   
#include "ESP32_FTPClient.h"

#include <NTPClient.h> //For request date and time
#include <WiFiUdp.h>
#include "time.h"

char* ftp_server = "FTP_SERVER";
char* ftp_user = "USER";
char* ftp_pass = "PASSWORD";
char* ftp_path = "/timelapse/";

const char* WIFI_SSID = "network";
const char* WIFI_PASS = "password";

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", (-3600*3), 60000);

ESP32_FTPClient ftp (ftp_server,ftp_user,ftp_pass, 5000, 2);

// Pin definition for CAMERA_MODEL_AI_THINKER
#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM      0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27

#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5
#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22

camera_config_t config;

void setup() {
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0); //disable brownout detector
 
  Serial.begin(115200);

  WiFi.begin(WIFI_SSID, WIFI_PASS);
  
  Serial.println("Connecting Wifi...");
  while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.println("Connecting to WiFi..");
      
  }
  Serial.println("IP address: ");
      
  Serial.println(WiFi.localIP());

  initCamera();

  timeClient.begin();
  timeClient.update();


  Serial.println(timeClient.getFullFormattedTimeForFile());

  ftp.OpenConnection();
}

void loop() {
  timeClient.update();
  takePhoto();
  delay(1000);
}

void initCamera() {
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG; 
  
  if(psramFound()){
    config.frame_size = FRAMESIZE_UXGA;//FRAMESIZE_UXGA; // FRAMESIZE_ + QVGA|CIF|VGA|SVGA|XGA|SXGA|UXGA
    config.jpeg_quality = 10;
    config.fb_count = 2;
  } else {
    config.frame_size = FRAMESIZE_UXGA;
    config.jpeg_quality = 12;
    config.fb_count = 1;
  }  
  // Init Camera
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    return;
  }  
}

void takePhoto() {

      
  camera_fb_t * fb = NULL;
  
  
  // Take Picture with Camera
  fb = esp_camera_fb_get();  
  if(!fb) {
    Serial.println("Camera capture failed");
    return;
  }

  /*
   * Upload to ftp server
   */
  

  ftp.ChangeWorkDir(ftp_path);
  ftp.InitFile("Type I");

  String nombreArchivo = timeClient.getFullFormattedTimeForFile()+".jpg"; // AAAAMMDD_HHMMSS.jpg
  Serial.println("Subiendo "+nombreArchivo);
  int str_len = nombreArchivo.length() + 1; 
 
  char char_array[str_len];
  nombreArchivo.toCharArray(char_array, str_len);
  
  ftp.NewFile(char_array);
  ftp.WriteData( fb->buf, fb->len );
  ftp.CloseFile();
  
  /*
   * Free
   */
  esp_camera_fb_return(fb); 
}
