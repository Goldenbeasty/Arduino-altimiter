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
#define NoOfScreens 6 //Remember to update when adding new screens
float eepromimport =  0;
#define EEPROM_sealevelpressure_addr 0
#define Batvolt A7
#define sealevelpressurechangescreen 3 //god I am good at naming variables

float maxheight;
float minheight;
float beginheight;

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

#define rotButton 4 // Normally high
#define debounce_time 500
long last_rotButton = 0;

// OLED setup
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 32 // OLED display height, in pixels

#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C // See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32

#define imageWidth 128
#define imageHeight 32

const unsigned char bitmap [] PROGMEM= //customizable bitmap, feel free to edit it
{
    0xff, 0xff, 0xff, 0xff, 0xff, 0xfe, 0x7f, 0xff, 0xff, 0xff, 0x80, 0x5f, 0xff, 0xff, 0xff, 0xff, 
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x7f, 0xff, 0xff, 0xfe, 0x2b, 0x82, 0xff, 0xff, 0xff, 0xff, 
    0xff, 0xff, 0xff, 0xff, 0xf1, 0xff, 0xbf, 0xff, 0xff, 0xf8, 0xff, 0xfd, 0x7f, 0xff, 0xff, 0xff, 
    0xff, 0xff, 0xff, 0xff, 0xc4, 0x7f, 0xbf, 0xff, 0xff, 0xe3, 0xff, 0xff, 0xd7, 0xff, 0xff, 0xff, 
    0xff, 0xff, 0xff, 0xff, 0x5e, 0x3f, 0xdf, 0xff, 0xff, 0x8f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
    0xff, 0xff, 0xff, 0xfd, 0x7f, 0xdf, 0xcf, 0xff, 0xff, 0x3f, 0xff, 0x80, 0x2f, 0xff, 0xff, 0xef, 
    0xff, 0xff, 0xff, 0xfb, 0xff, 0xff, 0xef, 0xff, 0xfe, 0xff, 0xfc, 0x00, 0x02, 0xff, 0xff, 0xef, 
    0xff, 0xf7, 0xf7, 0xff, 0xff, 0xff, 0xef, 0xfd, 0xfb, 0xff, 0xe0, 0xa0, 0x00, 0x1f, 0xef, 0x8f, 
    0xff, 0x7f, 0xbf, 0xd3, 0xff, 0xff, 0xf7, 0xdf, 0xef, 0xff, 0xab, 0x80, 0x00, 0x06, 0xff, 0x1f, 
    0xff, 0xff, 0xff, 0xfc, 0x3f, 0xff, 0xf7, 0xff, 0xff, 0xfe, 0xbc, 0x00, 0x00, 0x00, 0xfc, 0x2f, 
    0xff, 0xff, 0xff, 0x40, 0x87, 0xff, 0x7b, 0xff, 0xff, 0xfa, 0xe0, 0x08, 0x00, 0x00, 0x10, 0x3f, 
    0xfd, 0xff, 0xf8, 0x3a, 0x70, 0xff, 0xfb, 0xff, 0xff, 0xf7, 0x80, 0x37, 0xa0, 0x00, 0x00, 0x7f, 
    0xff, 0xff, 0xc2, 0x80, 0x0e, 0x3d, 0xff, 0xff, 0xff, 0xde, 0x4a, 0x5e, 0xf8, 0x80, 0x00, 0xff, 
    0xff, 0xfe, 0x8c, 0x00, 0x01, 0x87, 0xfb, 0xff, 0xff, 0xda, 0x14, 0xf7, 0xb4, 0xe0, 0x01, 0xff, 
    0xff, 0xff, 0xa0, 0x00, 0x00, 0xfb, 0xff, 0xf7, 0x7f, 0xf0, 0xf9, 0x90, 0xde, 0x7e, 0x03, 0x7f, 
    0xff, 0xea, 0x00, 0x11, 0x50, 0x1f, 0xff, 0xff, 0xff, 0xe7, 0xfb, 0x60, 0x3f, 0x3e, 0x06, 0xff, 
    0xf0, 0x00, 0x01, 0xf7, 0xfa, 0x07, 0xfd, 0xff, 0xfb, 0x9f, 0xd3, 0xda, 0x2b, 0x38, 0x0d, 0xff, 
    0xfc, 0x00, 0x0f, 0xe4, 0xae, 0x41, 0xfd, 0xff, 0xff, 0x7f, 0xfa, 0xff, 0xfd, 0xa8, 0x13, 0xff, 
    0xff, 0x00, 0x3f, 0x57, 0xbb, 0x34, 0xed, 0xff, 0xff, 0xff, 0x6d, 0x40, 0x8a, 0x90, 0x0f, 0xff, 
    0xff, 0xc0, 0x7d, 0xd0, 0x83, 0x7e, 0x7d, 0xff, 0xff, 0xfd, 0xaa, 0x54, 0x60, 0xe8, 0xaf, 0xff, 
    0xff, 0xe0, 0x3f, 0x9f, 0x06, 0xaf, 0xbb, 0xf7, 0xdf, 0xff, 0xe9, 0x49, 0x34, 0x20, 0xbf, 0xff, 
    0xff, 0xf9, 0x1f, 0xbd, 0xbb, 0x3b, 0xfd, 0xff, 0xff, 0xf7, 0xf6, 0xaa, 0xd3, 0x57, 0x7f, 0xff, 
    0xff, 0xfe, 0x0e, 0xb7, 0x55, 0x26, 0xfb, 0xff, 0xff, 0xde, 0xdf, 0x7b, 0xbf, 0x67, 0xd7, 0xff, 
    0xff, 0xff, 0x01, 0x00, 0x02, 0xdf, 0xdf, 0xff, 0xfe, 0xff, 0x79, 0xdd, 0x75, 0x55, 0x5d, 0xff, 
    0xfe, 0xff, 0xc0, 0x03, 0xbf, 0x7b, 0xfb, 0xff, 0xff, 0xf5, 0xd7, 0x67, 0xdb, 0xfb, 0xaf, 0xf7, 
    0xff, 0xfb, 0xe0, 0x35, 0xab, 0xce, 0xef, 0xff, 0x7f, 0xdf, 0x7f, 0xbd, 0x6f, 0x74, 0x7b, 0xbf, 
    0xff, 0xef, 0x84, 0xeb, 0x5e, 0x7b, 0xff, 0xfd, 0xfb, 0x75, 0xd9, 0x75, 0xad, 0x8b, 0xce, 0xff, 
    0xff, 0xff, 0xeb, 0x5d, 0xf9, 0xdf, 0xbf, 0xf7, 0xff, 0xef, 0x6f, 0xd5, 0x7a, 0xfd, 0x3b, 0xff, 
    0xff, 0xfd, 0xb9, 0xf6, 0xcf, 0xfa, 0xff, 0xff, 0xff, 0xbb, 0xbb, 0x2f, 0xd7, 0xa6, 0xff, 0xff, 
    0xfd, 0xb7, 0xeb, 0x4f, 0x7b, 0x6f, 0xff, 0xff, 0xfd, 0x7d, 0x7d, 0x7a, 0xbd, 0x7f, 0xd7, 0x6f, 
    0xff, 0xff, 0xaf, 0x7a, 0xdf, 0xaa, 0xbf, 0xff, 0xff, 0xeb, 0xeb, 0xd7, 0xeb, 0xda, 0xbd, 0xff, 
    0xff, 0xfd, 0x5c, 0xeb, 0xfa, 0x7b, 0xff, 0xbf, 0xff, 0x5f, 0xfe, 0xbd, 0x3f, 0x6b, 0xeb, 0x7f
};

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

void (*resetFunc)(void)=0;
#define watchdogtime 5000
float watchdog_begintime;

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
  if (currentscreen == 4){
    float currentalt = bmp.readAltitude(sealevelpressure);
    if (currentalt > maxheight) maxheight = currentalt;
    if (currentalt < minheight) minheight = currentalt;
    if (digitalRead(rotButton) == LOW) beginheight = minheight = maxheight = currentalt;
    default_screensetup(2);
    display.setCursor(12, 2);
    display.print(abs(currentalt - beginheight));
    if ((currentalt - beginheight) < 0) display.drawTriangle(0, 2, 5, 15, 10, 2, SSD1306_WHITE);
    else if ((currentalt - beginheight) > 0) display.drawTriangle(0, 15, 5, 2, 10, 15, SSD1306_WHITE);
    display.setCursor(2, 22);
    display.setTextSize(1);
    display.print(maxheight - minheight);
  }
  if (currentscreen == 5){ // watchdog to manually reset arduino for example when a sensor on the I2C bus has failed due to bugs etc.
    display.drawBitmap(0,0,bitmap, imageWidth, imageHeight, WHITE); 
    if (digitalRead(rotButton)) watchdog_begintime = cycletime;
    if ((cycletime - watchdog_begintime) > watchdogtime) resetFunc();    
  }

  display.display();
  display.clearDisplay();
}