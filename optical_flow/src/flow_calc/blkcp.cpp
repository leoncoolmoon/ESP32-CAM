#include <stdint.h>
#include <cstring>
void block_memcpy(uint8_t * img_old, uint8_t * img_current,int size, int left, int top,int right, int buttom,int o_width, int o_height,int step){
 /*
  left = 0;//can be changed, the test block left
  top = 0; //can be changed, the test block top
  right = fb->width;//can be changed, the test block right
  buttom  = fb->height;//can be changed, the test block buttom
  o_width = fb->width; //the original buffer width
  o_height = fb->height;//the original buffer height
  */
  int p_scr = 0;
  int p_des = 0;
  int copy_width = (right-left)*step;
  for (int i=top; i<buttom; i++){
            p_scr = i*o_width+left;
            p_des = (i-top)*copy_width;
	  memcpy(&img_old[p_des], &img_current[p_scr],copy_width);
  }
  //memcpy(img_old, img_current,int size)
  
}