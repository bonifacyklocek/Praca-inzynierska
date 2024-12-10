#include <Arduino.h>
#include "esp_camera.h"
// #include "soc/soc.h"
// #include "soc/rtc_cntl_reg.h"
// #include "driver/rtc_io.h"

// Pin definitions for CAMERA_MODEL_AI_THINKER
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

void configESPCamera(
  pixformat_t format_s,
  framesize_t frame_size_s,
  int quality,
  int brightness,
  int contrast,
  int saturation,
  int special_effect,
  int whitebal,
  int awb_gain,
  int wb_mode,
  int exposure_ctrl,
  int aec2,
  int ae_level,
  int aec_value,
  int gain_ctrl,
  int agc_gain,
  int gainceiling,
  int bpc,
  int wpc,
  int raw_gma,
  int lenc,
  int hmirror,
  int vflip,
  int dcw,
  int colorbar
){
  // Configure Camera parameters

  // Object to store the camera configuration parameters
  camera_config_t config;

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
  config.pin_sccb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = format_s;

  // Select lower framesize if the camera doesn't support PSRAM
  if (psramFound()) {
    Serial.println("PSARM found");
    config.frame_size = frame_size_s;
    config.jpeg_quality = quality; 
    config.fb_count = 1;
  } else {
    Serial.println("PSARM not found");
    config.frame_size = FRAMESIZE_SVGA;
    config.jpeg_quality = quality + 2;
    config.fb_count = 1;
  }

  // Initialize the Camera
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    return;
  }

  // Camera quality adjustments
  sensor_t * s = esp_camera_sensor_get();

  s->set_brightness(s, brightness);
  s->set_contrast(s, contrast);
  s->set_saturation(s, saturation);
  s->set_special_effect(s, special_effect);
  s->set_whitebal(s, whitebal);
  s->set_awb_gain(s, awb_gain);
  s->set_wb_mode(s, wb_mode);
  s->set_exposure_ctrl(s, exposure_ctrl);
  s->set_aec2(s, aec2);
  s->set_ae_level(s, ae_level);
  s->set_aec_value(s, aec_value);
  s->set_gain_ctrl(s, gain_ctrl);
  s->set_agc_gain(s, agc_gain);
  s->set_gainceiling(s, (gainceiling_t)gainceiling);
  s->set_bpc(s, bpc);
  s->set_wpc(s, wpc);
  s->set_raw_gma(s, raw_gma);
  s->set_lenc(s, lenc);
  s->set_hmirror(s, hmirror);
  s->set_vflip(s, vflip);
  s->set_dcw(s, dcw);
  s->set_colorbar(s, colorbar);

}