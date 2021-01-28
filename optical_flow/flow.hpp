
#pragma once

void flow_setup(uint8_t *init_img, int left, int top,int right, int buttom ,int o_width, int o_height);
int getFlow(uint8_t *img_current, int &dt_us, float &flow_x, float &flow_y, int left, int top,int right, int buttom ,int o_width, int o_height);
