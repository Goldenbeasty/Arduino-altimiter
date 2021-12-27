#include <Arduino.h>
#include <Adafruit_BMP280.h> // 0x67
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h> // 0x3C
#include <RotaryEncoder.h>
#include <EEPROM.h>
#include <avr/sleep.h>

//Variables for screens
int currentscreen = 0;
long lastcycletime = 0; // currently only on debug screen
int select_sealevel = 0;
int last_Encoder_pos_screen = currentscreen;
int rotDir = 0;
long cycletime = 0;
#define NoOfScreens 4 //Remember to update when adding new screens
float eepromimport =  0; // if migrating to floats for accruacy switch int sealevelpressure as well
#define EEPROM_sealevelpressure_addr 0
#define Batvolt A7
#define sealevelpressurechangescreen 3 //god I am good at naming variables

//sleeping configuration
float gotosleepat = 0;
#define time_until_sleep 10000

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

void wakeup(){
  sleep_disable();
  detachInterrupt(PIN_IN1);
  attachInterrupt(digitalPinToInterrupt(PIN_IN1), checkPosition, CHANGE);
}

void default_screensetup(int textsize){
  display.setCursor(2,2);
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(textsize);
}

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
                  Adafruit_BMP280::STANDBY_MS_63); /* Standby time. */

  encoder = new RotaryEncoder(PIN_IN1, PIN_IN2, RotaryEncoder::LatchMode::FOUR3);
  attachInterrupt(digitalPinToInterrupt(PIN_IN1), checkPosition, CHANGE);
  attachInterrupt(digitalPinToInterrupt(PIN_IN2), checkPosition, CHANGE);

  display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS); //initialize display
  encoder->setPosition(sealevelpressure);

  pinMode(rotButton, INPUT_PULLUP);
  pinMode(Batvolt, INPUT);
}

void loop() {
  cycletime = millis(); // dynamic variable that is going to be called multiple times, so I call it here once, might n even be remotely needed

  if(digitalRead(rotButton) == LOW and (cycletime - last_rotButton) > debounce_time){ // mess of code that is responsible for setting sealevel pressure
    if (currentscreen == sealevelpressurechangescreen and select_sealevel == 0){ // if currently on changing screen and in screen changing mode
      select_sealevel = 1;
      encoder->setPosition(sealevelpressure);
    }
    else if (currentscreen == sealevelpressurechangescreen and select_sealevel == 1){
      select_sealevel = 2;
      EEPROM.put(EEPROM_sealevelpressure_addr, sealevelpressure);
      last_Encoder_pos_screen = encoder->getPosition();
      encoder->setPosition(sealevelpressure * 10);
    }
    else if (currentscreen == sealevelpressurechangescreen and select_sealevel == 2){
      select_sealevel = 0;
      EEPROM.put(EEPROM_sealevelpressure_addr, sealevelpressure);
      last_Encoder_pos_screen = encoder->getPosition();
    }
    last_rotButton = cycletime;
  }

  if (currentscreen == sealevelpressurechangescreen and select_sealevel == 1){ // if it is being changed, just dump it to the variable
    sealevelpressure = encoder->getPosition();
  }
  else if (currentscreen == sealevelpressurechangescreen and select_sealevel == 2){ // same but for tens
    sealevelpressure = encoder->getPosition() / 10.0F;
  }
  else if (select_sealevel == 0){ // change screens once per cycle even if scroll is faster
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
    if (currentscreen > (NoOfScreens - 1)){
      currentscreen = NoOfScreens - 1;
    }
    else if (currentscreen < -1){ 
      currentscreen = -1; // sleep screen
    }
    last_Encoder_pos_screen = rotPos;
  }
  if (currentscreen != -1 and gotosleepat != 0){ // sets allow sleep to false if it already isn't and id current screen isn't the sleep screen
    gotosleepat = 0;
  }
  if (currentscreen == -1){ // in case of sleep screen
    if (gotosleepat == 0){
      gotosleepat = cycletime + time_until_sleep; // calculate time when to go to sleep
    }
    if (gotosleepat != 0 and cycletime > gotosleepat){ // send to sleep if sleep is allowed and the time is up
      sleep_enable();
      detachInterrupt(PIN_IN1);
      attachInterrupt(digitalPinToInterrupt(PIN_IN1), wakeup, CHANGE);
      set_sleep_mode(SLEEP_MODE_PWR_DOWN);
      sleep_cpu();
    }
  }

  if (currentscreen == 0){ // first screen
    default_screensetup(4);
    display.print(bmp.readAltitude(sealevelpressure));
  }
  if (currentscreen == 1){
    default_screensetup(1);
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
    default_screensetup(1);
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
    display.setCursor(96,2);
  }
  if (currentscreen == sealevelpressurechangescreen){
    default_screensetup(2);
    display.print((bmp.readPressure()) / 100.0F);
    display.setCursor(2, 18);
    if (select_sealevel != 0){
      display.setTextColor(BLACK,WHITE);
    }
    display.print(sealevelpressure);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(87, 4);
    display.setTextSize(1);
    display.print(bmp.readAltitude(sealevelpressure));
  }

  display.display();
  display.clearDisplay();
}