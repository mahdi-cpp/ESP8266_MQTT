
#include <Wire.h>

const int led = D5;

#define SDA_PIN 4  //D2
#define SCL_PIN 5  //D1

const int16_t I2C_DEV_ADDR = 0x55;

uint32_t i = 0;

const double VCC = 3.3;              // NodeMCU on board 3.3v vcc
const double R2 = 10000;             // 10k ohm series resistor
const double adc_resolution = 1023;  // 10-bit adc

const double A = 0.001129148;  // thermistor equation parameters
const double B = 0.000234125;
const double C = 0.0000000876741;


// function that executes whenever data is received from master
void receiveEvent(int howMany) {
  while (0 < Wire.available()) {
    char c = Wire.read(); /* receive byte as a character */
    Serial.print(c);      /* print the character */
  }
  Serial.println(); /* to newline */
}


// function that executes whenever data is requested from master
void requestEvent() {

  double Vout, Rth, temperature, adc_value;

  adc_value = analogRead(A0);
  Vout = (adc_value * VCC) / adc_resolution;
  Rth = (VCC * R2 / Vout) - R2;

  /*  Steinhart-Hart Thermistor Equation:
 *  Temperature in Kelvin = 1 / (A + B[ln(R)] + C[ln(R)]^3)
 *  where A = 0.001129148, B = 0.000234125 and C = 8.76741*10^-8  */
  temperature = (1 / (A + (B * log(Rth)) + (C * pow((log(Rth)), 3))));  // Temperature in kelvin

  temperature = temperature - 273.15;  // Temperature in degree celsius
  Serial.print("Temperature = ");
  Serial.print(temperature);
  Serial.println(" degree celsius");

  Wire.write("Hello NodeMCU"); /*send string on request */

 // Deep sleep mode until RESET pin is connected to a LOW signal (for example pushbutton or magnetic reed switch)
  ESP.deepSleep(0)
}

void setup() {

  pinMode(led, OUTPUT);

  Serial.begin(115200);  // start serial for output

  Wire.onReceive(receiveEvent);               /* register receive event */
  Wire.onRequest(requestEvent);               /* register request event */
  Wire.begin(SDA_PIN, SCL_PIN, I2C_DEV_ADDR); /* join i2c bus with address 8 */
}

void loop() {
}


void setup() {
  Serial.begin(9600); /* Define baud rate for serial communication */
}
