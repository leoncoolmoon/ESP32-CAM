/*
   https://github.com/espressif/esp32-camera/blob/master/README.md#jpeg-http-stream
*/


#include "esp_camera.h"
#include "esp_http_server.h"
#include "esp_timer.h"
#include <Arduino.h>
#include <WiFi.h>
#include "index_html_gz.h"
#include "src/flow_calc/blkcp.hpp"

const char* ssid = "ddkk";
const char* password = "0425271998";
bool initialized = false;
#define HEAD_WIDTH 64

#define PART_BOUNDARY "123456789000000000000987654321"
static const char* _STREAM_CONTENT_TYPE = "multipart/x-mixed-replace;boundary=" PART_BOUNDARY;
static const char* _STREAM_BOUNDARY = "\r\n--" PART_BOUNDARY "\r\n";
static const char* _STREAM_PART = "Content-Type: image/jpeg\r\nContent-Length: %u\r\n\r\n";
uint8_t * _m_img = NULL;
int _m_left = 0, _m_top = 0, _m_right = 0, _m_buttom = 0, _m_o_width = 160, _m_o_height = 128;
pixformat_t _m_format;


esp_err_t paring_httpd_handler(httpd_req_t *req) {
  esp_err_t res = ESP_OK;
  String request = String(req->uri);
  Serial.print( "required uri:");
  Serial.println( request );
  if (request.startsWith("/paring") ) { //ID and code is for bluetooth paring
    request.remove(0, 8); //"&ID="|"&code="
  }
  return res;
}
esp_err_t config_httpd_handler(httpd_req_t *req) {
  esp_err_t res = ESP_OK;
  String request = String(req->uri);
  Serial.print( "required uri:");
  Serial.println( request );

  if (request.startsWith("/config") ) { // config the test box
    request.remove(0, 8); //"&clear"| "&test"+minY+"~"+minX+"~"+maxY+"~"+maxX"|
    if (request.startsWith("&clear")) {
      _m_left   = 0;
      _m_top    = 0;
      _m_right  = 0;
      _m_buttom = 0;
      res = ESP_OK;
    } else if (request.startsWith("&test")) {
      request.remove(0, 5);
      int i = request.indexOf("~");
      _m_top    = request.substring(0, i).toInt();
      request.remove(0, (i + 1));
      i = request.indexOf("~");
      _m_left   = request.substring(0, i).toInt();
      request.remove(0, (i + 1));
      i = request.indexOf("~");
      _m_buttom = request.substring(0, i).toInt();
      request.remove(0, (i + 1));
      _m_right  = request.toInt();
      res = ESP_OK;
    }
  } else {
    res = ESP_FAIL;
  }
  return res;
}
esp_err_t index_httpd_handler(httpd_req_t *req) {
  String request = String(req->uri);
  Serial.print( "required uri:");
  Serial.println( request );
  httpd_resp_set_type(req, "text/html");
  httpd_resp_set_hdr(req, "Content-Encoding", "gzip");
  return httpd_resp_send(req, (const char *)index_html_gz, index_html_gz_len);

}
esp_err_t jpg_stream_httpd_handler(httpd_req_t *req) {
  esp_err_t res = ESP_OK;
  char * part_buf[HEAD_WIDTH];
  size_t _test_jpg_buf_len = 0;
  uint8_t * _test_jpg_buf = NULL;
  size_t _jpg_buf_len = 0;
  uint8_t * _jpg_buf = NULL;
  size_t _fm_buf_len = 0;
  uint8_t * _fm_buf = NULL;

  String request = String(req->uri);
  Serial.print( "required uri:");
  Serial.println( request );

  /*
    request min -> stream small test image  if the size of test block is 0 then stream the full image
    request max -> stream the full image
    request config -> update the current config
    is sth wrong -> quite the loop
  */

  if ( request.startsWith("/stream")) { //stream the video
   // Serial.println( "stream" );
    request.remove(0, 8); //"&max" | "&min"
   // Serial.println( request );
    res = httpd_resp_set_type(req, _STREAM_CONTENT_TYPE);
    if (res != ESP_OK) {
      Serial.print(" resp_set_type error: STREAM");
      Serial.println(res);
      return res;
    }
    while (true) {
      if ( request.equals("&min") && (_m_right - _m_left) * ( _m_buttom - _m_top)  * _m_o_width * _m_o_height != 0) {
       // Serial.println( "min" );
        _fm_buf_len = (_m_right - _m_left) * (_m_buttom - _m_top);
        _fm_buf = (uint8_t*) malloc(_fm_buf_len);

        block_memcpy(_fm_buf, _m_img,  _m_left,  _m_top, _m_right,  _m_buttom, _m_o_width, _m_o_height, 1);

        if (! fmt2jpg(_fm_buf, _fm_buf_len, (_m_right - _m_left), (_m_buttom - _m_top), _m_format, 80, &_test_jpg_buf, &_test_jpg_buf_len) ) {
          Serial.println("flagment JPEG compression failed");
          res = ESP_FAIL;
        }
        if (res == ESP_OK) {
          size_t hlen = snprintf((char *)part_buf, HEAD_WIDTH, _STREAM_PART, _test_jpg_buf_len);
          res = httpd_resp_send_chunk(req, (const char *)part_buf, hlen);
        }
        if (res == ESP_OK) {
          res = httpd_resp_send_chunk(req, (const char *)_test_jpg_buf, _test_jpg_buf_len);
        }
        if (res == ESP_OK) {
          res = httpd_resp_send_chunk(req, _STREAM_BOUNDARY, strlen(_STREAM_BOUNDARY));
        }
        free(_test_jpg_buf);
        free(_fm_buf);
      } else if ( request.equals("&max") ) {
       // Serial.println( "max" );
        if (! fmt2jpg(_m_img, _m_o_width * _m_o_height, _m_o_width, _m_o_height, _m_format, 80, &_jpg_buf, &_jpg_buf_len) ) {
          Serial.println(" JPEG compression failed");
          res = ESP_FAIL;
        }
        if (res == ESP_OK) {
          size_t hlen = snprintf((char *)part_buf, HEAD_WIDTH, _STREAM_PART, _jpg_buf_len);

          res = httpd_resp_send_chunk(req, (const char *)part_buf, hlen);
        }
        if (res == ESP_OK) {
          res = httpd_resp_send_chunk(req, (const char *)_jpg_buf, _jpg_buf_len);
        }
        if (res == ESP_OK) {
          res = httpd_resp_send_chunk(req, _STREAM_BOUNDARY, strlen(_STREAM_BOUNDARY));
        }
        free(_jpg_buf);
        if (res != ESP_OK) {
          break;
        }
      }else{
        Serial.println(" resp not exist!");
        res = ESP_FAIL;
        break;
        }

    }
  }

  if (res != ESP_OK) {
    Serial.print(" resp_set_type error: ");
    Serial.println(res);

  }
  return res;
}

//void displayFrame() {}
void start_video_streaming() {
  Serial.println("Connecting to WiFi");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");

  httpd_handle_t index_httpd = NULL;
  httpd_handle_t stream_httpd = NULL;
  httpd_config_t config = HTTPD_DEFAULT_CONFIG();
  config.server_port = 80;

  httpd_uri_t index_uri = {
    .uri       = "/",
    .method    = HTTP_GET,
    .handler   = index_httpd_handler,
    .user_ctx  = NULL
  };

  httpd_uri_t stream_uri = {
    .uri       = "/stream",
    .method    = HTTP_GET,
    .handler   = jpg_stream_httpd_handler,
    .user_ctx  = NULL
  };

  httpd_uri_t config_uri = {
    .uri       = "/config",
    .method    = HTTP_GET,
    .handler   = config_httpd_handler,
    .user_ctx  = NULL
  };

  httpd_uri_t paring_uri = {
    .uri       = "/paring",
    .method    = HTTP_GET,
    .handler   = paring_httpd_handler,
    .user_ctx  = NULL
  };
  Serial.printf("Starting web server on port: '%d'\n", config.server_port);
  if (httpd_start(&index_httpd, &config) == ESP_OK) {
    httpd_register_uri_handler(index_httpd, &index_uri);
    httpd_register_uri_handler(index_httpd, &config_uri);
    httpd_register_uri_handler(index_httpd, &paring_uri);
  }
  config.server_port += 1;
  config.ctrl_port += 1;
  if (httpd_start(&stream_httpd, &config) == ESP_OK) {
    httpd_register_uri_handler(stream_httpd, &stream_uri);
  }
  Serial.print("Camera Stream Ready! Go to: http://");
  Serial.println(WiFi.localIP());
}
void video_streaming(uint8_t *img, pixformat_t format, int &left, int &top, int &right, int &buttom, int &o_width, int &o_height) {
  _m_img = img;
  _m_format = format;
  _m_o_width  = o_width ;
  _m_o_height = o_height;


  left       = _m_left    ;
  top        = _m_top     ;
  right      = _m_right   ;
  buttom     = _m_buttom  ;



  if (!initialized) {
    start_video_streaming();
    initialized = true;
    return;
  }
  //displayFrame();
}
