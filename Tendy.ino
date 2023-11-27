#include "tendy.hpp"

float led_pwm = 0.0;

void setup() {
  tendy::setup::theRGBLEDs();

  // float led_pwm = tendy::utility::setLEDPWM(0.25);

  // // Turn on the BLUE LED to 25% intensity
  // tendy::main::pmwAnalogWrite(tendy::constants::RGB_PWM_CHANNEL[2], led_pwm);

  // tendy::setup::theMotors();

  // float motor_pwm = tendy::utility::setMotorPWM(0.75);

  // // Spin both motors FORWARD at about 75% Speed
  // for (int i = 0; i < 2; i++) {
  //   motor_pwm = (i == 1) ? 0:motor_pwm;
  //   tendy::main::pmwAnalogWrite(tendy::constants::RIGHT_MOTOR_PWM_CHANNEL[i], motor_pwm*1.00655);
  //   tendy::main::pmwAnalogWrite(tendy::constants::LEFT_MOTOR_PWM_CHANNEL[i], motor_pwm);
  // }

  tendy::wifi::setupWIFI();

}

void loop() {
  tendy::wifi::websocket_server.cleanupClients();
}


// void websocketCallback(websockets::WebsocketsMessage msg) {
//   // Websocket data is in the string format: LValue,RValue
//   int commaIndex = msg.data().indexOf(',');
//   float LValue = msg.data().substring(0, commaIndex).toInt();
//   float RValue = msg.data().substring(commaIndex + 1).toInt();
  
//   int ch = (LValue < 0 && RValue > 0) ? 0:(LValue > 0 && RValue > 0) ? 1:2;
//   led_pwm = (LValue == 0 && RValue < 0) ? abs(RValue):abs(LValue); 
//   tendy::main::pmwAnalogWrite(tendy::constants::RGB_PWM_CHANNEL[ch], led_pwm);
// }
