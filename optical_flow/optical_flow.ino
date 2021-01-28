/*
  Main code, where the other component used from.
*/

#include "src/espcam/camera.h"
#include "flow.hpp"
#include "video_streaming.hpp"

//test block size <128x128(default )
int left   =0;
int top    =0;
int right  = 128;
int buttom = 128;
//original image size
int o_width  =0;
int o_height =0;

void setup() {
  Serial.begin(115200);
  Serial.setDebugOutput(true);
  Serial.println();

  camera_setup(); // setting up and configuring the camera
 // for debugging, the camera image can be viewed online, but it will slow down the device (maybe blocking the flow calculation)
  start_video_streaming();
  
  // setting up and configuring the flow computer, initializing it's image buffer
  camera_fb_t * fb = esp_camera_fb_get();
  // TODO: check/use fb->width (fb->height, fb->len)
  Serial.printf("fb->width: %d, fb->height: %d, fb->len: %d\n", fb->width, fb->height, fb->len); // fb->width: 128, fb->height: 160, fb->len: 20480
  left = 0;//can be changed, the test block left
  top = 0; //can be changed, the test block top
  right = 128;//can be changed, the test block right
  buttom  = 128;//can be changed, the test block buttom
  o_width = fb->width; //the original width
  o_height = fb->height;//the original height
  flow_setup(fb->buf,left,top,right,buttom,o_width,o_height);
  esp_camera_fb_return(fb); // return the buffer to the pool

 
}


void loop() {
  camera_fb_t * fb = esp_camera_fb_get();
  if (!fb) {
    Serial.println("Camera capture failed");
    return; // ESP_FAIL;
  }

  int dt_us;
  float flow_rate_x, flow_rate_y;
 // int flow_quality = getFlow(fb->buf, dt_us, flow_rate_x, flow_rate_y);
  int flow_quality = getFlow(fb->buf, dt_us, flow_rate_x, flow_rate_y,left,top,right,buttom,o_width,o_height);

  uint16_t ground_distance = 1;//get_distance(); // from lidar

  //float flow_comp_m_x = - pixel_flow_x / focal_length / ( (time_now - time_prev) / 1000000.0f) * ground_distance;

  // TODO: calculate pixel flow
  int16_t pixel_flow_x = flow_rate_x;
  int16_t pixel_flow_y = flow_rate_y;

  float speed_x = tan(flow_rate_x) * (float)ground_distance*flow_quality;
  float speed_y = tan(flow_rate_y) * (float)ground_distance*flow_quality;

 // Serial.printf(" dt: %6d us,\tx: %7.3f,\ty: %7.3f,\tquality: %4d,\tlidar: %" PRIu16 " \n", dt_us, speed_x, speed_y, flow_quality, ground_distance);
 if (abs(speed_x)>1 || abs(speed_y)>1){
  Serial.printf(" x: %7.3f,\ty: %7.3f,\tlidar: %" PRIu16 " \n",  speed_x, speed_y, ground_distance);
 }



  esp_camera_fb_return(fb); // return the buffer to the pool
}
