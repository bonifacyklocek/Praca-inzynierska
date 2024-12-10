#include <Arduino.h>
#include "esp_camera.h"
#include "soc/rtc_cntl_reg.h"
// #include "soc/soc.h"
// #include "driver/rtc_io.h"

#include "edge-impulse-sdk/classifier/ei_run_classifier.h"
#include "edge-impulse-sdk/dsp/image/image.hpp"

const bool sendToServer = true;

uint8_t servoPin = 14;
uint8_t pumpPin = 15;

gpio_num_t wakeupPin = GPIO_NUM_13;

#define EI_CAMERA_RAW_FRAME_BUFFER_COLS  640
#define EI_CAMERA_RAW_FRAME_BUFFER_ROWS  480

const uint8_t num_of_segments = 5;
const uint16_t raw_segment_square_size = EI_CAMERA_RAW_FRAME_BUFFER_COLS / num_of_segments;
const uint16_t cut_segment_square_size = EI_CLASSIFIER_INPUT_WIDTH;

RTC_DATA_ATTR uint8_t current_pos = 90;

const uint8_t allowed_error = 4; // in degrees

// external function declarations
void configESPCamera(
    pixformat_t format_s = PIXFORMAT_GRAYSCALE, // Choices are YUV422, GRAYSCALE, RGB565, JPEG
    framesize_t frame_size_s = FRAMESIZE_VGA,  // FRAMESIZE_ + QVGA|CIF|VGA|SVGA|XGA|SXGA|UXGA
    int quality = 10,       // 10-63 lower number means higher quality
    int brightness = 1,     // BRIGHTESS (-2 to 2)
    int contrast = 2,       // CONTRAST (-2 to 2)
    int saturation = -1,    // SATURATION (-2 to 2)
    int special_effect = 0, // SPECIAL EFFECTS (0 - No Effect, 1 - Negative, 2 - Grayscale, 3 - Red Tint, 4 - Green Tint, 5 - Blue Tint, 6 - Sepia)
    int whitebal = 1,       // WHITE BALANCE (0 = Disable , 1 = Enable)
    int awb_gain = 0,       // AWB GAIN (0 = Disable , 1 = Enable)
    int wb_mode = 0,        // WB MODES (0 - Auto, 1 - Sunny, 2 - Cloudy, 3 - Office, 4 - Home)
    int exposure_ctrl = 0,  // LEPIEJ NA 0 // EXPOSURE CONTROLS (0 = Disable , 1 = Enable)
    int aec2 = 0,           // AEC2 (0 = Disable , 1 = Enable)
    int ae_level = 1,       // AE LEVELS (-2 to 2)
    int aec_value = 1100,   // AEC VALUES (0 to 1200)
    int gain_ctrl = 1,      // GAIN CONTROLS (0 = Disable , 1 = Enable)
    int agc_gain = 10,       // AGC GAIN (0 to 30)
    int gainceiling = 4,    // GAIN CEILING (0 to 6)
    int bpc = 0,            // BPC (0 = Disable , 1 = Enable)
    int wpc = 1,            // WPC (0 = Disable , 1 = Enable)
    int raw_gma = 1,        // RAW GMA (0 = Disable , 1 = Enable)
    int lenc = 1,           // LENC (0 = Disable , 1 = Enable)
    int hmirror = 1,        // HORIZ MIRROR (0 = Disable , 1 = Enable)
    int vflip = 1,          // VERT FLIP (0 = Disable , 1 = Enable)
    int dcw = 1,            // DCW (0 = Disable , 1 = Enable)
    int colorbar = 0        // COLOR BAR PATTERN (0 = Disable , 1 = Enable)
);

void connectToWiFi() ;
void sendDataToServer(uint8_t *buf, size_t total_size, String format, String name) ;

void configServo(uint8_t servoPin) ;
void rotate(uint8_t setdeg) ;
void reset() ; 

// internal functions declarations
void processImage(camera_fb_t *fb, uint16_t raw_size, uint16_t cut_size, uint16_t offset, uint8_t *int_buf, bool do_resize) ;
static int get_signal_data(size_t offset, size_t length, float *out_ptr) ;
//

uint8_t * photo_to_print;
uint8_t * temp_buf;
uint8_t * snapshot_buf;
uint8_t * target_pos; // in deg

void runPump(){
  digitalWrite(pumpPin, HIGH);
  delay(500);
  digitalWrite(pumpPin, LOW);
}

void markTarget(uint8_t * photo_to_print, ei_impulse_result_bounding_box_t bb) {
  for (uint8_t pxh = 0; pxh < bb.width; pxh++) {
    photo_to_print[cut_segment_square_size * bb.y + bb.x + pxh] = 0;
    photo_to_print[cut_segment_square_size * (bb.y + bb.height - 1) + bb.x + pxh] = 0;
  }
  for (uint8_t pxv = 0; pxv < bb.height - 2; pxv++) {
    photo_to_print[cut_segment_square_size * (bb.y + pxv + 1) + bb.x] = 0;
    photo_to_print[cut_segment_square_size * (bb.y + pxv + 1) + bb.x + bb.width - 1] = 0;
  }
}

RTC_DATA_ATTR uint16_t photos_taken = 0;

bool targetFound(){
  
  snapshot_buf = (uint8_t *)malloc(cut_segment_square_size * cut_segment_square_size); // EI_CAMERA_RAW_FRAME_BUFFER_COLS * EI_CAMERA_RAW_FRAME_BUFFER_ROWS * EI_CAMERA_FRAME_BYTE_SIZE);
  photo_to_print = (uint8_t *)malloc(cut_segment_square_size * cut_segment_square_size);
  bool do_resize = false;

  if(raw_segment_square_size != cut_segment_square_size){
    do_resize = true;
    temp_buf = (uint8_t*)malloc(raw_segment_square_size * raw_segment_square_size);

    if (temp_buf == nullptr) {
      Serial.println("ERR: Failed to allocate temporary buffer!\n");
      return false;
    }
  }

  if (snapshot_buf == nullptr){
    Serial.println("ERR: Failed to allocate snapshot buffer!\n");
    return false;
  }

  camera_fb_t *fb = esp_camera_fb_get();

  if (!fb){
    Serial.println("Camera capture failed\n");
    return false;
  }

  const uint16_t cap = 700; // promili
  uint16_t highest_score = cap;
  uint16_t h_center_x = 0;
  
  for (uint8_t i = 0; i < num_of_segments; i++){
    processImage(fb, raw_segment_square_size, cut_segment_square_size, i * raw_segment_square_size, (do_resize)? temp_buf : snapshot_buf, do_resize);
    signal_t signal;
    ei_impulse_result_t result;

    signal.total_length = EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE;
    signal.get_data = &get_signal_data;
    
    EI_IMPULSE_ERROR res = run_classifier(&signal, &result, false);
    if (res != EI_IMPULSE_OK){
      ei_printf("ERR: Failed to run classifier (%d)\n", res);
      continue;
    }
    
    memcpy(photo_to_print, snapshot_buf, EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE);
    photo_to_print[(cut_segment_square_size/(num_of_segments*2))*(2*i+1)] = 255;
    photo_to_print[cut_segment_square_size + (cut_segment_square_size/(num_of_segments*2))*(2*i+1)] = 255;

    for (uint8_t j = 0; j < EI_CLASSIFIER_OBJECT_DETECTION_COUNT; j++){
      ei_impulse_result_bounding_box_t bb = result.bounding_boxes[j];
      if (bb.value == 0){
        continue;
      }
      if ((uint16_t)(bb.value * 1000) > highest_score){
        highest_score = (uint16_t)(bb.value *  1000);
        h_center_x = i * cut_segment_square_size + bb.x + bb.width/2;
        markTarget(photo_to_print, bb);
      }
      printf("  %s (%f) [ x: %u, y: %u, width: %u, height: %u ]\r\n",
             bb.label,
             bb.value,
             bb.x,
             bb.y,
             bb.width,
             bb.height);
    }
    if (sendToServer) {
      size_t jpg_size = 0;
      uint8_t *jpg_buf = NULL;
      bool jpg_v1 = fmt2jpg(photo_to_print, (size_t)cut_segment_square_size * cut_segment_square_size, cut_segment_square_size, cut_segment_square_size, PIXFORMAT_GRAYSCALE, 90, &jpg_buf, &jpg_size);
      sendDataToServer(jpg_buf, jpg_size, "jpg", (String)(1000 + photos_taken));
      photos_taken++;

      free(jpg_buf);
    }
  };

  esp_camera_fb_return(fb);
  fb = nullptr;
  if(snapshot_buf) free(snapshot_buf);
  snapshot_buf = nullptr;
  if (do_resize && temp_buf) free(temp_buf);
  temp_buf = nullptr;
  if(photo_to_print) free(photo_to_print);
  photo_to_print = nullptr;

  if (highest_score > cap){
    float degrees = ((float)h_center_x / (num_of_segments * cut_segment_square_size)) * 160;
    *target_pos = (uint8_t)(degrees);
    printf("\nTarget pos: %u / %u px  |  %u / 160 deg)", h_center_x, (num_of_segments * cut_segment_square_size), *target_pos);

    return true;
  }

  return false;
}

void processImage(camera_fb_t *fb, uint16_t raw_size, uint16_t cut_size, uint16_t offset, uint8_t *int_buf, bool do_resize) {

  uint16_t skip_rows = (EI_CAMERA_RAW_FRAME_BUFFER_ROWS - raw_size)/2;

  for(uint16_t i = 0; i < raw_size; i++){
      for(uint16_t j = 0; j < raw_size; j++){
        int_buf[(i * raw_size) + j] = (fb->buf)[(skip_rows + i) * EI_CAMERA_RAW_FRAME_BUFFER_COLS + j + offset];
      }
  }

  if (do_resize) {
    ei::image::processing::resize_image(
      int_buf,
      raw_size,
      raw_size,
      snapshot_buf,
      cut_size,
      cut_size,
      1);
  }
}


void setup() {

  // Disable brownout detector
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);

  Serial.begin(115200);

  configServo(servoPin);
  
  if (esp_sleep_get_wakeup_cause() != ESP_SLEEP_WAKEUP_EXT0) {
    Serial.print("Wakeup cause: uknown (reset)");

    rotate(90);
  } else {
    Serial.print("Wakeup cause: PIR Sensor activation!");
  }
    Serial.print("Initializing the camera module...");
  configESPCamera();
  delay(20);
  Serial.println("Camera OK!");

  pinMode(pumpPin, OUTPUT);

  target_pos = (uint8_t *)malloc(sizeof(uint8_t));
  if (target_pos != NULL) {
    *target_pos = 0;
  } else {
    Serial.print("\nBłąd alokowania pamięci");
  }

  if(sendToServer) {
      connectToWiFi();
      delay(200);  // Wait for stable connection
  }

  int16_t error;
  int16_t new_pos;

  printf("\n[Idle pos: %u ]\n", current_pos);
  while (targetFound()) {
    error = *target_pos - 80;
    printf("\n[Current position: %u ] [Error: %d]", current_pos, error);
    if (error <= allowed_error && error >= -1 * allowed_error){
      Serial.println("\nTarget in line. Water pump is ON!");
      runPump();
      break;
    } else {
      printf("\nRotating by %d...", error);
      new_pos = current_pos - error;
      if(new_pos > 180) new_pos = 180; 
      if(new_pos < 0) new_pos = 0; 
      rotate(new_pos);
      current_pos = new_pos;
      delay(1000 + 6*abs(error));
      continue;
    }
  }
  Serial.println("\nJobs done!");

  if(target_pos) free(target_pos);

  // Bind Wakeup to GPIO13 going HIGH
  delay(1500);
  esp_sleep_enable_ext0_wakeup(wakeupPin, 1);

  Serial.println("Entering sleep mode\n");
  delay(200);

  // Enter deep sleep mode
  esp_deep_sleep_start();
}

void loop() {

}

static int get_signal_data(size_t offset, size_t length, float *out_ptr) {
    for (size_t i = 0; i < length; i++) {
        out_ptr[i] = (snapshot_buf + offset)[i] * 0xff;
    }
    return EIDSP_OK;
}