#pragma once

/**

  MOTOR CONTROL
  -------------
  nSLEEP IN1 IN2 OUT1   OUT2  DESCRIPTION
  0       X   X   Hi-Z  Hi-Z  Low-power sleep mode
  1       0   0   Hi-Z  Hi-Z  Coast (H-bridge Hi-Z)
  1       0   1   L     H     Reverse (OUT2 → OUT1)
  1       1   0   H     L     Forward (OUT1 → OUT2)
  1       1   1   L     L     Brake (low-side slow decay)


  Download the ESPAsyncWebServer dependency from https://github.com/me-no-dev/ESPAsyncWebServer 
  and add to the project.

  Install the ArduinoWebsockets and AsyncTCP dependencies via the Library Manager of the IDE


  Reference Tutorial: https://nkmakes.github.io/2020/09/02/esp32-tank-robot-joystick-http-web-control/


*/

#include "config.h"
#include "web.h"
#include <Arduino.h>
// #include <ArduinoWebsockets.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>

namespace tendy::constants {
constexpr int LEFT_RGB[] = { 26, 20, 21 };
constexpr int RIGHT_RGB[] = { 12, 10, 11 };
constexpr int RGB_PWM_CHANNEL[] = { 5, 6, 7 };

constexpr int BUZZER = 33;
constexpr int BUZZER_PWM_CHANNEL = 4;  // Channel No. 4 for PWM for Buzzer

constexpr int ULTRASONIC[] = { 13, 14 };  // {Trigger, Echo}

constexpr int BATT_SENSE = 3;

constexpr int LINE_SENSE[] = { 8, 9 };   // {Right, Left} i.e. when the robot's rear side faces towards you
constexpr int LIGHT_SENSE[] = { 2, 1 };  // {Right, Left} i.e. when the robot's rear side faces towards you

constexpr int RIGHT_MOTOR[] = { 34, 35 };  // {M1IN1, M1IN2} i.e. when the robot's rear side faces towards you
constexpr int LEFT_MOTOR[] = { 4, 5 };     // {M2IN1, M2IN2} i.e. when the robot's rear side faces towards you
constexpr int RIGHT_MOTOR_PWM_CHANNEL[] = { 0, 1 };
constexpr int LEFT_MOTOR_PWM_CHANNEL[] = { 2, 3 };

constexpr int MOTOR_SLEEP = 15;  // An Active Low Signal, i.e., LOW --> Sleep Mode | HIGH --> Wake Mode

constexpr int LED_PWM_FREQ = 12000;      // Set the PWM Frequency of the RGB LEDs to 12kHz.
constexpr int LED_PWM_RESOLUTION = 11;   // Max Resolution = 11 bits | Min. Resolution = 7 bit
constexpr int MOTOR_PWM_FREQ = 50000;    // Set the PWM Frequency of the Motor Driver to 50kHz. Max frq. = 100kHz
constexpr int MOTOR_PWM_RESOLUTION = 9;  // Max. Resolution = 9 bits | Min. Resolution = 2 bits

constexpr int LEDC_CLOCK_FREQ = 80000000;  // The APB Clock frequency 80MHz

constexpr int MAX_PWM_CHANNELS = 8;  // Maximum independent PWM Channels for ESP32S2

}

namespace tendy::utility {
// Ref: https://www.espressif.com/sites/default/files/documentation/esp32_technical_reference_manual_en.pdf#ledpwm

using namespace tendy::constants;

constexpr int calc_pwm_resolution(int freq) {
  // float freq_div = LEDC_CLOCK_FREQ / freq;
  // float res = log(freq_div) / log(2);       // log base 2 from first principles
  // float res = log((LEDC_CLOCK_FREQ / freq)) / log(2);       // log base 2 from first principles

  // return (int)(round(res*10)/10);
  return (int)(round((log((LEDC_CLOCK_FREQ / freq)) / log(2)) * 10) / 10);
}

constexpr int calc_max_pwm_resolution_count(int resolution_in_bits) {
  return pow(2, resolution_in_bits);
}

/**
    @param percentage_duty Set between 0 and 1;
  */
float setMotorPWM(float percentage_duty) {
  static constexpr int max_resolution_count = calc_max_pwm_resolution_count(MOTOR_PWM_RESOLUTION);

  return percentage_duty * max_resolution_count;
}

/**
    @param percentage_duty Set between 0 and 1;
  */
float setLEDPWM(float percentage_duty) {
  static constexpr int max_resolution_count = calc_max_pwm_resolution_count(LED_PWM_RESOLUTION);

  return percentage_duty * max_resolution_count;
}

static uint8_t pin_to_channel[MAX_PWM_CHANNELS] = { 0 };
void configurePWMFor(int pin, int freq) {
  static int channel = 0;

  assert((channel < MAX_PWM_CHANNELS) && "PWM Channels for ESP32S2 are only 8. Maximum reached!");

  ledcSetup(channel, freq, calc_pwm_resolution(freq));

  ledcAttachPin(pin, channel);

  pin_to_channel[channel] = pin;

  ++channel;
}

void configurePWMFor(int pin, int chan, int freq, int res) {
  ledcSetup(chan, freq, res);
  ledcAttachPin(pin, chan);
}

}

namespace tendy::setup {

using namespace tendy::constants;
using namespace tendy::utility;

void theRGBLEDs() {
  for (int i = 0; i < 3; i++) {
    configurePWMFor(LEFT_RGB[i], RGB_PWM_CHANNEL[i], LED_PWM_FREQ, LED_PWM_RESOLUTION);
    configurePWMFor(RIGHT_RGB[i], RGB_PWM_CHANNEL[i], LED_PWM_FREQ, LED_PWM_RESOLUTION);
  }
}

void theBuzzer() {
  pinMode(BUZZER, OUTPUT);
}

void theBatterySensor() {
  pinMode(BATT_SENSE, INPUT);
}

void theMotors() {
  pinMode(MOTOR_SLEEP, OUTPUT);

  for (int i = 0; i < 2; i++) {
    configurePWMFor(RIGHT_MOTOR[i], RIGHT_MOTOR_PWM_CHANNEL[i], MOTOR_PWM_FREQ, MOTOR_PWM_RESOLUTION);
    configurePWMFor(LEFT_MOTOR[i], LEFT_MOTOR_PWM_CHANNEL[i], MOTOR_PWM_FREQ, MOTOR_PWM_RESOLUTION);
  }

  digitalWrite(MOTOR_SLEEP, HIGH);
}

void theUltrasonicSensor() {
}



}

namespace tendy::wifi {
// using namespace websockets;
using namespace tendy::config;

AsyncWebServer http_webserver(HTTP_WEBSERVER_PORT);  // HTTP Server on port 80
AsyncWebSocket websocket_server("/ws");              // access at ws://[esp ip]/ws
// AsyncEventSource events("/events"); // event source (Server-Sent events)

void setupWIFI() {
  Serial.begin(115200);

  WiFi.softAP(WIFI_SSID, WIFI_PASSWORD);
  IPAddress IP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(IP);

  // WiFi.mode(WIFI_STA);
  // WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  // if (WiFi.waitForConnectResult() != WL_CONNECTED) {
  //     Serial.printf("WiFi Failed!\n");
  //     return;
  // }

  // Serial.print("IP Address: ");
  // Serial.println(WiFi.localIP());


  websocket_server.onEvent([](AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len) {
    switch (type) {
      case WS_EVT_CONNECT:
        {
          printf("ws[%s][%u] connect\n", server->url(), client->id());
          client->printf("Hello Client %u :)", client->id());
          client->ping();
          break;
        }
      case WS_EVT_DISCONNECT:
        {
          //client disconnected
          printf("ws[%s][%u] disconnect: %u\n", server->url(), client->id());   
          break;
        }
      case WS_EVT_PONG: {
        //pong message was received (in response to a ping request maybe)
        printf("ws[%s][%u] pong[%u]: %s\n", server->url(), client->id(), len, (len)?(char*)data:"");
        break;
      }
      case WS_EVT_DATA: {
        AwsFrameInfo * info = (AwsFrameInfo*)arg;
        if(info->opcode == WS_TEXT){
          data[len] = 0;
          printf("%s\n", (char*)data);
        }
        else {
          for(size_t i=0; i < info->len; i++){
            printf("%02x ", data[i]);
          }
          printf("\n");
        }
        break;
      }
    }
  });


    http_webserver.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
      IPAddress remote_ip = request->client()->remoteIP();
      Serial.println("[" + remote_ip.toString() + "] HTTP GET request of " + request->url());

      AsyncWebServerResponse *response = request->beginResponse_P(200, "text/html", INDEX_HTML_GZ, sizeof(INDEX_HTML_GZ));
      response->addHeader("Content-Encoding", "gzip");
      request->send(response);
    });

  http_webserver.onNotFound([](AsyncWebServerRequest *request) {
    IPAddress remote_ip = request->client()->remoteIP();
    Serial.println("[" + remote_ip.toString() + "] HTTP GET request of " + request->url());

    request->send_P(404, "text/plain", "Not found");
  });


  // attach Websocket
  http_webserver.addHandler(&websocket_server);

  // attach AsyncEventSource
  // http_webserver.addHandler(&events);

  // start the server
  http_webserver.begin();
  // websocket_server.listen(WEBSOCKET_SERVER_PORT);  // Websocket server on port 82
  // Serial.print("Is server live? ");
  // Serial.println(websocket_server.available());
}

// void handleWebSocketClient(const PartialMessageCallback cb) {
//   auto client = websocket_server.accept();
//   client.onMessage(cb);
//   while (client.available()) {
//     client.poll();
//   }
// }



}

namespace tendy::main {

/**
    @param pin: Digital Pin to write the PWM signals to
    @param pwm: PWM value in foating point, 
  */
void pmwAnalogWrite(int channel, float pwm) {
  ledcWrite(channel, pwm);
}
}