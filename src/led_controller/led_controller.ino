#include <FastLED.h>

// io
#define USB_DETECT 5  // atmega pin 11
#define LED_OUT_1 6   // atmega pin 12
#define LED_OUT_2 7   // atmega pin 13
#define LED_OUT_3 8   // atmega pin 14

// SPI pins 
#define SS 10   // atmega 15
#define MOSI 11 // atmega 17
#define MISO 12 // atmega 18
#define SCK 13  // atmega 19

// LED specific
#define NUM_STRIPS 3
#define NUM_LEDS 30

CRGBArray<NUM_LEDS> leds;

void setup() {
  Serial.begin(9600);
  pinMode(USB_DETECT, INPUT);
  pinMode(LED_OUT_2, OUTPUT);
  FastLED.addLeds<NEOPIXEL, LED_OUT_1>(leds, NUM_LEDS);
}

void loop() {
  Serial.println("serial output!?");
  if(digitalRead(USB_DETECT) == HIGH){
    digitalWrite(LED_OUT_2, HIGH);
  } else {
    digitalWrite(LED_OUT_2, LOW);
  }
  static uint8_t hue;
  for(int i = 0; i < NUM_LEDS/2; i++){
    leds.fadeToBlackBy(10);

    leds[i] = CHSV(hue++, 255, 255);

    leds(NUM_LEDS/2,NUM_LEDS-1) = leds(NUM_LEDS/2 - 1, 0);
    FastLED.delay(99);
  }
}

