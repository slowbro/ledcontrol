//vim: ts=4 sw=4 noexpandtab

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

// LED helper
#define LED_ORDER_LTOR 0
#define LED_ORDER_RTOL 1
#define LED_STRIPLEN_EVEN 0
#define LED_STRIPLEN_ODD 1

// variables
static NEOPIXEL<LED_OUT_1> ch1;
static NEOPIXEL<LED_OUT_2> ch2;
static NEOPIXEL<LED_OUT_3> ch3;
CRGBArray<NUM_LEDS> leds;
int ledOrder[NUM_LEDS];
int ledCount = 0;
String output = "";

/*************************
 * MAIN/SETUP FUNCTIONS  *
 *************************/
void setup() {
	Serial.begin(57600);
	
	// set up i/o pins
	pinMode(USB_DETECT, INPUT);
	pinMode(USB_LED, OUTPUT);
	
	// set up inturrupts for USB
	enableInterrupt(USB_DETECT, usbChange, CHANGE);

	// add the LED strips to FastLED
	addLedStrip(&ch1, NUM_LEDS_PER_STRIP, LED_ORDER_RTOL);
	addLedStrip(&ch2, NUM_LEDS_PER_STRIP, LED_ORDER_RTOL);
	addLedStrip(&ch3, NUM_LEDS_PER_STRIP, LED_ORDER_LTOR);
	
	// probe usb status at boot
	usbChange();
}

void loop() {    
	Serial.println("loop!");
	//rainbowFromMiddle();
	//fillLtoR(CRGB::Purple);
	rainbowSlideFromMiddle();
}


/*************************
 *  GEOMETRY FUNCTIONS   *
 *************************/

// returns the "real world" X-position for an LED.
int X(int x){
	return ledOrder[x];
}

// overflow-safe X-position
int Xsafe(int x){
	if( x > NUM_LEDS ) return -1;
	return ledOrder[x];
}

// returns LED_STRIPLEN_EVEN/LED_STRIPLEN_ODD (basically)
int evenodd(){
	return NUM_LEDS % 2;
}

// returns the ledOrder index number for the
// middle LED. in the case of even (two 'middles')
// it returns the left-most 'middle'
int middle(){
	if(evenodd() == LED_STRIPLEN_EVEN) return NUM_LEDS / 2 - 1;
	return (NUM_LEDS + 1) / 2 - 1;
}


/*************************
 * LED HELPER FUNCTIONS  *
 *************************/

// adds a strip of LEDS in a certain order
void addLedStrip(CLEDController *channel, int num_leds, int order){
	Serial.println(output + "LedCount is " + ledCount);

	CFastLED::addLeds(channel, leds, ledCount, num_leds);
	
	for(int i = 0; i < num_leds; i++){
		if(order == LED_ORDER_LTOR){
			ledOrder[ledCount+i] = ledCount+i;
		} else { // RtoL
			ledOrder[ledCount+i] = ledCount+((num_leds-1)-i);
		}
	}

	ledCount += num_leds;
}


/*************************
 *      ANIMATIONS       *
 *************************/

void fillLtoR(CRGB color){
	FastLED.setBrightness(196);
	for(int i = 0; i < NUM_LEDS; i++){
		String output = "Filling LED";
		Serial.println(output + i + " at position " + X(i));
		leds.fadeToBlackBy(3);
		leds[X(i)] = color;
		FastLED.show();
		delay(66);
	}
	FastLED.show();
}

void rainbowSlideFromMiddle(){
	static uint8_t hue;
	int mid = middle();
	CRGB oldcolor_cur;
	// first, store the old mid color and bump the value
	CRGB oldcolor_next = leds[mid];
	leds[mid] = CHSV(hue++, 255, 255);
	if(evenodd() == LED_STRIPLEN_EVEN){
		leds[mid+1] = leds[mid];
	}
	for(int i = mid-1; i >= 0; i--){
		// now, 'slide' the old color down the strip in both directions
		oldcolor_cur = leds[X(i)];
		leds[X(i)] = oldcolor_next;
		// and mirror it to the other side
		int mirror = mid+(mid - i);
		if(evenodd() == LED_STRIPLEN_EVEN){
			mirror++;
		}
		leds[Xsafe(mirror)] = oldcolor_next;
		oldcolor_next = oldcolor_cur;
	}
	FastLED.show();
	delay(66);
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
	leds[X(i)] = CHSV(hue++, 255, 255);
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

	for(int i = 1; i < 196; i++){
		FastLED.setBrightness(i);
		FastLED.show();
		delay(33);
	}
}


/*************************
 *  USB/SLEEP FUNCTIONS  *
 *************************/

// probe USB status and do something accordingly
void usbChange(){
	if(digitalRead(USB_DETECT) == HIGH){
		digitalWrite(USB_LED, HIGH);
		fadeToWhite();
	} else {
		digitalWrite(USB_LED, LOW);
		fadeToBlack();
		sleepNow();
	}
}

// sleep the MCU in the highest sleep mode
void sleepNow(){
	set_sleep_mode(SLEEP_MODE_PWR_DOWN);
	sei();
	sleep_enable();
	sleep_cpu();
	sleep_disable();
	cli();
}
