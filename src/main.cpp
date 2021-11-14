#include <Arduino.h>
#include <Adafruit_BMP280.h> // 0x67
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h> // 0x3C
#include <RotaryEncoder.h>
#include <EEPROM.h>

//Variables for screens
int currentscreen = 0;
long lastcycletime = 0; // currently only on debug screen
int select_sealevel = 0;
int last_Encoder_pos_screen = currentscreen;
int rotDir = 0;
long cycletime = 0;
#define NoOfScreens 3 //Remember to update when adding new screens
float eepromimport =  0; // if migrating to floats for accruacy switch int sealevelpressure as well
#define EEPROM_sealevelpressure_addr 0
#define Batvolt A7

//BMP280 setup
Adafruit_BMP280 bmp;

#define PIN_IN1 3
#define PIN_IN2 2

float sealevelpressure = SENSORS_PRESSURE_SEALEVELHPA;

//Encoder setup
RotaryEncoder *encoder = nullptr;

void checkPosition(){
  encoder->tick(); // just call tick() to check the state.
}

#define rotButton 4
#define debounce_time 500
long last_rotButton = 0;

// OLED setup
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 32 // OLED display height, in pixels

#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C // See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

void setup() {

  EEPROM.get(EEPROM_sealevelpressure_addr, eepromimport);
  if (eepromimport < 2){
    eepromimport = SENSORS_PRESSURE_SEALEVELHPA; // Try to reset EEPROM address because of course there was already something written there
  }
  sealevelpressure = eepromimport;

  bmp.begin(BMP280_ADDRESS_ALT, BMP280_CHIPID);  // My sensor sits on 0x76 so I had to use the alternative address
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
  pinMode(Batvolt, INPUT);

  Serial.begin(9600);
}

void loop() {
  cycletime = millis();

  if(digitalRead(rotButton) == LOW and (cycletime - last_rotButton) > debounce_time){
    if (currentscreen == 1 and select_sealevel == 0){
      select_sealevel = 1;
      encoder->setPosition(sealevelpressure);
    }
    else if (currentscreen == 1 and select_sealevel == 1){
      select_sealevel = 2;
      EEPROM.put(EEPROM_sealevelpressure_addr, sealevelpressure);
      last_Encoder_pos_screen = encoder->getPosition();
      encoder->setPosition(sealevelpressure * 10);
    }
    else if (currentscreen == 1 and select_sealevel == 2){
      select_sealevel = 0;
      // sealevelpressure = sealevelpressure / 10;
      EEPROM.put(EEPROM_sealevelpressure_addr, sealevelpressure);
      last_Encoder_pos_screen = encoder->getPosition();
    }
    last_rotButton = cycletime;
  }

  if (currentscreen == 1 and select_sealevel == 1){
    sealevelpressure = encoder->getPosition();
  }
  else if (currentscreen == 1 and select_sealevel == 2){
    sealevelpressure = encoder->getPosition() / 10.0F;
    Serial.println("hello");
    Serial.print(sealevelpressure);
  }
  else if (select_sealevel == 0){
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
    if (currentscreen > (NoOfScreens -1)){
      currentscreen = NoOfScreens - 1;
    }
    else if (currentscreen < -1){
      currentscreen = -1;
    }
    last_Encoder_pos_screen = rotPos;
  }

  if (currentscreen == 0){
    display.setCursor(2,2);
    display.setTextSize(4);
    display.setTextColor(SSD1306_WHITE);
    display.print(bmp.readAltitude(sealevelpressure));
  }
  if (currentscreen == 1){
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
    display.print(cycletime - lastcycletime);
    lastcycletime = cycletime;
    display.setCursor(64,22);
    display.print(encoder->getPosition());
    display.setCursor(98,2);
    display.print(currentscreen);
    display.setCursor(98,12);
    display.print(select_sealevel);
    display.setCursor(98,22);
    display.print(analogRead(Batvolt) * (5.0 / 1023.0));
    display.setCursor(108,2);
    display.print(digitalRead(rotButton));
  }
  if (currentscreen == 2){
    display.setCursor(2,2);
    display.setTextColor(SSD1306_WHITE);
    display.setTextSize(1);
    display.print(select_sealevel);
    display.setCursor(2,12);
    display.print(digitalRead(rotButton));
    display.setCursor(2,22);
    display.print(millis());
    display.setCursor(64,2);
    display.print(cycletime);
    display.setCursor(64,12);
    display.print(last_rotButton);
    display.setCursor(64,22);
    display.print((cycletime - last_rotButton) > debounce_time);
  }

  display.display();
  display.clearDisplay();
}