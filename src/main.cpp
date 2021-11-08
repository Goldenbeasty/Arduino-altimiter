#include <Arduino.h>
#include <Adafruit_BMP280.h> // 0x67
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h> // 0x3C
#include <RotaryEncoder.h>

//Variables for screens
int currentscreen = 1;
int lastcycletime = 0;
bool select_sealevel = false;
int last_Encoder_pos_screen = currentscreen;
int rotDir = 0;

//BMP280 setup
Adafruit_BMP280 bmp;

#define PIN_IN1 2
#define PIN_IN2 3

int sealevelpressure = SENSORS_PRESSURE_SEALEVELHPA;

//Encoder setup
RotaryEncoder *encoder = nullptr;

void checkPosition(){
  encoder->tick(); // just call tick() to check the state.
}

#define rotButton 4
#define debounce_time 100
uint32_t last_rotButton = 0;

// OLED setup
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 32 // OLED display height, in pixels

#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C // See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

void setup() {
  bmp.begin(BMP280_ADDRESS_ALT, BMP280_CHIPID);  // My sensor sits on 0x76 so I had to use the alternative address

      Serial.begin(9600); //DEBUG

  bmp.setSampling(Adafruit_BMP280::MODE_NORMAL,     /* Operating Mode. */
                  Adafruit_BMP280::SAMPLING_X2,     /* Temp. oversampling */
                  Adafruit_BMP280::SAMPLING_X16,    /* Pressure oversampling */
                  Adafruit_BMP280::FILTER_X16,      /* Filtering. */
                  Adafruit_BMP280::STANDBY_MS_500); /* Standby time. */

  encoder = new RotaryEncoder(PIN_IN1, PIN_IN2, RotaryEncoder::LatchMode::FOUR3);
  attachInterrupt(digitalPinToInterrupt(PIN_IN1), checkPosition, CHANGE);
  attachInterrupt(digitalPinToInterrupt(PIN_IN2), checkPosition, CHANGE);

  display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS); //initialize display
  encoder->setPosition(sealevelpressure);

  pinMode(rotButton, INPUT_PULLUP);
}

void loop() {
  uint32_t cycletime = millis();

  if(digitalRead(rotButton) == LOW and (cycletime - last_rotButton) > debounce_time){
    if (currentscreen == 1 and select_sealevel != true){
      select_sealevel = true;
      encoder->setPosition(sealevelpressure);
    }
    else if (currentscreen == 1 and select_sealevel){
      select_sealevel = false;
      last_Encoder_pos_screen = encoder->getPosition();
      // encoder->setPosition(currentscreen);
      // Serial.print("end"); //DEBUG
    }
    last_rotButton = cycletime;
  }

  if (currentscreen == 1 and select_sealevel){
    sealevelpressure = encoder->getPosition();
  }
  else if (select_sealevel != true){
    int rotPos = encoder->getPosition();
    if (rotPos == last_Encoder_pos_screen){
      rotDir = 0; 
    }
    else if (rotPos > last_Encoder_pos_screen){
      rotDir = 1;
    }
    else if (rotPos < last_Encoder_pos_screen){
      rotDir = -1;
    }
    
    currentscreen = currentscreen + rotDir;
    last_Encoder_pos_screen = rotPos;
  }

  if (currentscreen == 0){
    display.setCursor(2,2);
    display.setTextSize(4);
    display.setTextColor(SSD1306_WHITE);
    display.print(bmp.readAltitude(sealevelpressure));
  }
  if (currentscreen > 0){
    display.setCursor(2,2);
    display.setTextColor(SSD1306_WHITE);
    display.setTextSize(1);
    display.print(bmp.readAltitude(sealevelpressure));
    display.setCursor(2,12);
    display.print(bmp.readPressure());
    display.setCursor(2,22);
    display.print(sealevelpressure);
    display.setCursor(64,2);
    display.print(bmp.readTemperature());
    display.setCursor(64,12);
    // display.print(cycletime - lastcycletime);
    lastcycletime = cycletime;
    display.setCursor(64,22);
    display.print(encoder->getPosition());
    display.setCursor(98,2);
    display.print(currentscreen);
    display.setCursor(98,12);
    display.print(select_sealevel);
  }


      Serial.println(currentscreen); //DEBUG
  display.display();
  display.clearDisplay();
}