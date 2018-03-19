#define EI_NOTEXTERNAL
#define EI_NOTPORTC
#define EI_NOTPORTD
#include <EnableInterrupt.h>
#include <FastLED.h>
#include <avr/sleep.h>
#include <avr/interrupt.h>


// io
#define USB_DETECT 9  // atmega pin 15
#define USB_LED   12  // atmega pin 18 (MISO)
#define LED_OUT_1 A2  // atmega pin 25
#define LED_OUT_2 A1  // atmega pin 24
#define LED_OUT_3 A0  // atmega pin 23

// SPI pins 
#define MOSI 11 // atmega 17
#define MISO 12 // atmega 18
#define SCK 13  // atmega 19

// LED specific
#define NUM_STRIPS 3
#define NUM_LEDS_PER_STRIP 30
#define NUM_LEDS NUM_STRIPS*NUM_LEDS_PER_STRIP

// variables
CRGBArray<NUM_LEDS> leds;
bool USBStatus;

void setup() {
  Serial.begin(57600);
  pinMode(USB_DETECT, INPUT);
  pinMode(USB_LED, OUTPUT);
  enableInterrupt(USB_DETECT, usbChange, CHANGE);
  FastLED.addLeds<NEOPIXEL, LED_OUT_1>(leds, 0, NUM_LEDS_PER_STRIP);
  FastLED.addLeds<NEOPIXEL, LED_OUT_2>(leds, NUM_LEDS_PER_STRIP, NUM_LEDS_PER_STRIP);
  FastLED.addLeds<NEOPIXEL, LED_OUT_3>(leds, 2*NUM_LEDS_PER_STRIP, NUM_LEDS_PER_STRIP);
  usbChange();
}

void loop() {    
  Serial.print(".");
  rainbowFromMiddle();
}

void rainbowFromMiddle() {
  static uint8_t hue;
  for(int i = NUM_LEDS/2; i > 0; i--){
    rainbowPulseStep(i, hue);
  }
}

void rainbowFromEnds() {
  static uint8_t hue;
  for(int i = 0; i < NUM_LEDS/2; i++){
    rainbowPulseStep(i, hue);
  }
}

void rainbowPulseStep(int i, uint8_t &hue){
  leds.fadeToBlackBy(5);
  leds[i] = CHSV(hue++, 255, 255);
  leds(NUM_LEDS/2,NUM_LEDS-1) = leds(NUM_LEDS/2 - 1, 0);
  FastLED.show();
  delay(66);
}

void fadeToBlack(){
  for(int i = 31; i < 255; i += 8){
    leds.fadeToBlackBy(i);
    FastLED.show();
    delay(66);
  }
}

void fadeToWhite(){
  for(int i = 0; i < NUM_LEDS; i++){
    leds[i] = CRGB::White;
  }
  
  for(int i = 1; i < 64; i++){
    FastLED.setBrightness(i);
    FastLED.show();
    delay(66);
  }
}

void usbChange(){
  if(digitalRead(USB_DETECT) == HIGH){
    digitalWrite(USB_LED, HIGH);
    USBStatus = true;
    fadeToWhite();
  } else {
    digitalWrite(USB_LED, LOW);
    USBStatus = false;
    fadeToBlack();
    // "sleep" while USB is unplugged
    while(USBStatus == false){
      delay(33);
    }
  }
}


