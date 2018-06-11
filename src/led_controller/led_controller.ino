//vim: ts=4 sw=4 noexpandtab
#include <FastLED.h>
#include <avr/sleep.h>
#include <avr/interrupt.h>

// io (rev 1.0 board)
#define USB_DETECT 5  // atmega pin 11
#define USB_LED    6  // atmega pin 12
#define LED_OUT_1 A2  // atmega pin 25
#define LED_OUT_2 A1  // atmega pin 24
#define LED_OUT_3 A0  // atmega pin 23

// SPI pins 
#define CSEL 10 // atmega 16 (flash csel)
#define MOSI 11 // atmega 17
#define MISO 12 // atmega 18
#define SCK  13 // atmega 19

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
CRGB leds[NUM_LEDS];
int ledOrder[NUM_LEDS];
int ledCount = 0;
String output = "";
volatile int intr = 0;
int brightness = 0;
bool locked = false;

// animation switching
void (*previousAnimation)();
void (*currentAnimation)();
void (*nextAnimation)();
CEveryNMilliseconds changeTimer(500);
bool timerReady = false;
bool waitForAnimationComplete = true;
unsigned int animationStep = 0;
CEveryNMilliseconds animationTimer(33);

// serial related
String serialString = "";
boolean serialComplete = false;
enum serialAction {
	sLock,
	sUnlock,
	sNotification,
	sStatus,
	sBrightness,
	sWhite,
	sRainbow,
	sUnknown
};

/*************************
 * MAIN/SETUP FUNCTIONS  *
 *************************/
void setup() {
	Serial.begin(57600);
	Serial.println("Initializing...");
	serialString.reserve(512);
	
	// set up i/o pins
	pinMode(USB_DETECT, INPUT);
	pinMode(USB_LED, OUTPUT);
	pinMode(CSEL, OUTPUT);
	
	// set up interrupts for USB
	cli();
	PCICR  |= 0b00000100; // enable port d
	PCMSK2 |= 0b00100000; // enable PCICR21
	sei();

	// add the LED strips to FastLED
	addLedStrip(&ch1, NUM_LEDS_PER_STRIP, LED_ORDER_RTOL);
	addLedStrip(&ch2, NUM_LEDS_PER_STRIP, LED_ORDER_RTOL);
	addLedStrip(&ch3, NUM_LEDS_PER_STRIP, LED_ORDER_LTOR);
	
	// probe usb status at boot
	usbChange();

	// setup initial animation
	currentAnimation  = &animRainbowSlideFromMiddle;
	previousAnimation = &animRainbowSlideFromMiddle;

	Serial.println("Initialized.");
}

void loop() {    
	// process incoming commands
	if(serialComplete){
		handleSerial(serialString);
		serialString = "";
		serialComplete = false;
	}

	// process interrupts
	if(intr) handleInterrupt();

	// process animation changes
	EVERY_N_MILLISECONDS(33){
		if(timerReady && changeTimer){
			previousAnimation = currentAnimation;
			currentAnimation  = nextAnimation;
			timerReady = false;
		}
	}

	// run the current animation
	EVERY_N_MILLISECONDS(10){
		if(animationTimer){
			animationTimer.reset();
			(*currentAnimation)();
			FastLED.show();
		}
	}
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
	if( x > NUM_LEDS ) return NUM_LEDS;
	if( x < 0 ) return 0;
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
 *   ANIMATION HELPERS   *
 *************************/

// set the next animation, to be cut to after delayTime millis
void setNextAnimation(void (*nextAnim)(), uint32_t delayTime){
	changeTimer.setPeriod(delayTime);
	nextAnimation = nextAnim;
	timerReady = true;
	changeTimer.reset();
}


/*************************
 *      ANIMATIONS       *
 *************************/

// twinkling lights
void animTwinkle(){
	if (random8() < 64){
		leds[random16(NUM_LEDS)] = CRGB::White;
	}
	fadeToBlackBy(leds, NUM_LEDS, 32);
}

// animation to play when there is a notification
void animNotification(){
	// save the current state
	CRGB state[NUM_LEDS];
	memcpy8(state, leds, sizeof(leds));
	// empty the LED array ("set to black")
	//memset(leds, 0, NUM_LEDS*3);
	FastLED.setBrightness(96);
	FastLED.show();
	for(int i=0;i<NUM_LEDS/2;i++){
		if(intr) return;
		fadeToBlackBy(leds, NUM_LEDS, 8);
		leds[X(i)] = CRGB::Red;
		leds[X((NUM_LEDS-1)-i)] = CRGB::Red;
		FastLED.delay(16);
	}
	// reverse runner
	for(int o=NUM_LEDS/2;o>0;o--){
		if(intr) return;
		fadeToBlackBy(leds, NUM_LEDS, 8);
		leds[X(o-1)] = CRGB::Red;
		leds[X((NUM_LEDS-1)-(o-1))] = CRGB::Red;
		FastLED.delay(16);
	}
	// return to the saved state
	memcpy8(leds, state, sizeof(state));
	// fade in quickly
	for(int i=16;i<brightness;i++){
		FastLED.setBrightness(i);
		FastLED.delay(8);
	}
}

void animFillLtoR(CRGB color){
	FastLED.setBrightness(196);
	for(int i = 0; i < NUM_LEDS; i++){
		if(intr) return;
		String output = "Filling LED";
		Serial.println(output + i + " at position " + X(i));
		for(int i=0;i<NUM_LEDS;i++){
			leds[i].fadeToBlackBy(3);
		}
		leds[X(i)] = color;
		FastLED.show();
		delay(66);
	}
}

// basically the default animation right now
void animRainbowSlideFromMiddle(){
	animationTimer.setPeriod(66);
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
		if(intr) return;
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
}

void animFadeToBlack(){
	for(; brightness >0; brightness--){
		if(intr) return;
		FastLED.setBrightness(brightness);
		FastLED.show();
		delay(33);
	}
	FastLED.setBrightness(0);
	FastLED.show();
}

void animFadeToWhite(){
	Serial.println("Fading in...");
	fill_solid(leds, NUM_LEDS, CRGB::White);

	for(; brightness < 196; brightness++){
		if(intr) return;
		FastLED.setBrightness(brightness);
		FastLED.show();
		delay(33);
	}
	Serial.println("Fade complete.");
}

// this should become 'animSolidColor(CRGB)' when i get around to extending currentAnimation
void animSolidWhite(){
	fill_solid(leds, NUM_LEDS, CRGB::White);
}

/*************************
 *  USB/SLEEP FUNCTIONS  *
 *************************/

// pinchange interrupt for USB
ISR(PCINT2_vect){
	cli();
	intr = 1;
	delay(150);
	sei();
}

// handle interrupts
void handleInterrupt(){
	int currIntr = intr;
	intr = 0;
	switch(currIntr){
		case 1: // USB change interrupt
			usbChange();
			break;
	}
}

// probe USB status and do something accordingly
void usbChange(){
	if(digitalRead(USB_DETECT) == HIGH){
		digitalWrite(USB_LED, HIGH);
		animFadeToWhite();
	} else {
		digitalWrite(USB_LED, LOW);
		animFadeToBlack();
		// reset the animation if locked
		// so we don't resume the 'locked' animation when re-docking
		if(locked){
			locked = false;
			currentAnimation = previousAnimation;
		}
		sleepNow();
	}
}

// sleep the MCU in the highest sleep mode
void sleepNow(){
	set_sleep_mode(SLEEP_MODE_PWR_DOWN);
	sleep_enable();
	sleep_cpu();
	sleep_disable();
}


/*************************
 *   SERIAL FUNCTIONS    *
 *************************/

// handle received byte
void serialEvent(){
	while(Serial.available()){
		char inChar = (char)Serial.read();
		if(inChar == '\n' || inChar == '\r'){
			serialComplete = true;
			continue;
		} else if(serialComplete) {
			return;
		}
		serialString += inChar;
	}
}

// translate an input string to the appropriate command-enum
serialAction serialTranslate(String input){
	if(input == "lock") return sLock;
	if(input == "unlock") return sUnlock;
	if(input == "notification") return sNotification;
	if(input == "status") return sStatus;
	if(input == "brightness") return sBrightness;
	if(input == "white") return sWhite;
	if(input == "rainbow") return sRainbow;
	return sUnknown;
}

// handle the command
void handleSerial(String input){
	switch(serialTranslate(input)){
		case sLock:
			Serial.println("Got lock command");
			locked = true;
			setNextAnimation(animTwinkle, 200);
			break;
		case sUnlock:
			Serial.println("Got unlock command");
			locked = false;
			setNextAnimation(previousAnimation, 200);
			break;
		case sNotification:
			Serial.println("Notify!");
			if(!locked && currentAnimation != animNotification){
				setNextAnimation(currentAnimation, 500);
				currentAnimation = &animNotification;
			}
			break;
		case sStatus:
			Serial.println("Current status:");
			Serial.println(output+FastLED.getFPS()+" FPS");
			break;
		case sBrightness:
			Serial.println(output+"Brightness: "+brightness);
			break;
		case sWhite:
			setNextAnimation(animSolidWhite, 1);
			break;
		case sRainbow:
			setNextAnimation(animRainbowSlideFromMiddle, 1);
			break;
		case sUnknown:
		default:
			Serial.println("Invalid command: "+input);
			break;
	}
}
