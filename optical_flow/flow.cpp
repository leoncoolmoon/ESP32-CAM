
#include <esp_timer.h>
#include "src/flow_calc/flow_px4.hpp"


OpticalFlowPX4 * opticalFlowPX4;
#define FOCAL_LENGTH 200
//angle 62, EFL 4.3mm, pixel size 2.2um res 1600x1200
//const float focal_length_px = (global_data.param[PARAM_FOCAL_LENGTH_MM]) / (4.0f * 6.0f) * 1000.0f; //original focal lenght: 12mm pixelsize: 6um, binning 4 enabled
//float focal_length = 250;
#define OUTPUT_RATE 1000 // the maximum times a flow reported per second
#define TEST_BLOCK_WIDTH 128
#define TEST_BLOCK_HEIGHT 128

void flow_setup(uint8_t *init_img, int left, int top,int right, int buttom ,int o_width, int o_height) {
  // OpticalFlowPX4(float f_length_x, float f_length_y, int ouput_rate = 15, int img_width = 64, int img_height = 64, int search_size = 6, int flow_feature_threshold = 30, int flow_value_threshold = 3000);
  // note: img_height not used, img_width is used and the algorith assume a square image!
  /*
  left = 0;//can be changed, the test block left
  top = 0; //can be changed, the test block top
  right = fb->width;//can be changed, the test block right
  buttom  = fb->height;//can be changed, the test block buttom
  o_width = fb->width; //the original width
  o_height = fb->height;//the original height
  */
  opticalFlowPX4 = new OpticalFlowPX4(FOCAL_LENGTH, FOCAL_LENGTH, OUTPUT_RATE, TEST_BLOCK_WIDTH,TEST_BLOCK_HEIGHT);//setup memory size for the test

  int a; float b; // placeholders, not used
  // init the image buffer: for the first calculation, we will need a reference image to compare with
  opticalFlowPX4->calcFlow(init_img, a, a, b, b,left,top,right,buttom,o_width,o_height); // this will not compute anything when first called, but will the the base sample for compare.
}

int getFlow(uint8_t *img_current, int &dt_us, float &flow_x, float &flow_y, int left, int top,int right, int buttom ,int o_width, int o_height) {
  int64_t time_now = esp_timer_get_time(); // time in microseconds since boot

  int flow_quality = opticalFlowPX4->calcFlow(img_current, (uint32_t)time_now, dt_us, flow_x, flow_y,left,top,right,buttom,o_width,o_height);

  return flow_quality;
}
