#include <Arduino.h>
#include <Adafruit_BMP280.h> // 0x67
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h> // 0x3C
#include <RotaryEncoder.h>

Adafruit_BMP280 bmp;

#define PIN_IN1 2
#define PIN_IN2 3

void checkPosition();

RotaryEncoder *encoder = nullptr;

void checkPosition()
{
  encoder->tick(); // just call tick() to check the state.
}

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 32 // OLED display height, in pixels

#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C // See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

int sealevelpressure = SENSORS_PRESSURE_SEALEVELHPA;

float last = 0; //DEBUG

void setup() {
  Serial.begin(9600);

  if (!bmp.begin(BMP280_ADDRESS_ALT, BMP280_CHIPID)) { // My sensor sits on 0x76 so I had to use the alternative address
    Serial.println(F("Could not find a valid BMP280 sensor, check wiring or "
                      "try a different address!"));
    while (1) delay(10);
  }

  /* Default settings from datasheet. */
  bmp.setSampling(Adafruit_BMP280::MODE_NORMAL,     /* Operating Mode. */
                  Adafruit_BMP280::SAMPLING_X2,     /* Temp. oversampling */
                  Adafruit_BMP280::SAMPLING_X16,    /* Pressure oversampling */
                  Adafruit_BMP280::FILTER_X16,      /* Filtering. */
                  Adafruit_BMP280::STANDBY_MS_500); /* Standby time. */

  encoder = new RotaryEncoder(PIN_IN1, PIN_IN2, RotaryEncoder::LatchMode::FOUR3);
  attachInterrupt(digitalPinToInterrupt(PIN_IN1), checkPosition, CHANGE);
  attachInterrupt(digitalPinToInterrupt(PIN_IN2), checkPosition, CHANGE);

  display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS); //initialize display

}

void loop() {
  
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(2);
  display.setCursor(2,2);
  display.print(encoder->getPosition());

  Serial.print(F("Temperature = "));
  Serial.print(bmp.readTemperature());
  Serial.println(" *C");

  Serial.print(F("Pressure = "));
  Serial.print(bmp.readPressure());
  Serial.println(" Pa");

  Serial.print(F("Approx altitude = "));
  Serial.print(bmp.readAltitude(1003.25));
  Serial.println(" m");

  Serial.println();

  display.setCursor(32,2);
  display.print(bmp.readAltitude(sealevelpressure));

  display.setCursor(2,16); //DEBUG
  display.print(micros() - last); //DEBUG
  last = micros(); //DEBUG

  display.display();
  display.clearDisplay();
}