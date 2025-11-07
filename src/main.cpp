//////////////////////////////////////////////////////////////////////
////////////// ttgommi-leipzig by Martin Mittrenga ///////////////////
//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////

#include <Arduino.h>
#include <U8g2lib.h>
#include <RadioLib.h>
#include <Adafruit_NeoPixel.h>
#include <Pangodream_18650_CL.h>
#include <Preferences.h>                //lib for flashstoreage
#include <bitmaps.h>
#include <rom/rtc.h>
#include "config.h"

#ifdef U8X8_HAVE_HW_SPI
#include <SPI.h>
#endif

#ifdef U8X8_HAVE_HW_I2C
#include <Wire.h>
#endif

uint32_t cpu_frequency = 0;
uint32_t xtal_frequency = 0;
uint32_t apb_frequency = 0;

String name = "ttgommi1";               // Device Name
String version = "T0.01";               // Frimeware Version
String mode = "send";

char buf_version[5];
char buf_name[12];
char buf_bV[5];
char buf_bL[4];
char buf_init[12];
char buf_print[32];            

///////////////////////////////////////////////
///////////////////////////////////////////////
///////////////////////////////////////////////

unsigned long lastSendTime = 0;                   // Last send time
unsigned long lastGetBattery = 0;
unsigned long lastDisplayPrint = 0;


int defaultBrightnessDisplay = 255;   // value from 1 to 255
int defaultBrightnessLed = 255;       // value from 1 to 255
int bL = 0;
int waitSend = 10000;

///////////////////////////////////////////////
///////////////////////////////////////////////
///////////////////////////////////////////////

double bV = 0;

bool clkState;
bool lastClkState;
bool initSuccess = LOW;
bool initSuccess2 = LOW;
bool connected = LOW;
bool connectedInit = LOW;
bool connectedState = LOW;
bool initBattery = LOW;
bool batteryAttention = LOW;
bool batteryAttentionState = LOW;
bool expired = LOW;

#define RELAI_PIN           14
#define LED_PIN             12
#define LED_COUNT            3

#define LED_PIN_INTERNAL    25        //Accumulator Function Declaration
#define ADC_PIN             35
#define CONV_FACTOR       1.85        //1.7 is fine for the right voltage
#define READS               20

#define loadWidth           50        //Bitmap Declaration
#define loadHeight          50
#define logoWidth          128
#define logoHeight          64
#define loraWidth          128
#define loraHeight          64
#define batteryWidth        29
#define batteryHeight       15
#define signalWidth         21
#define signalHeight        18
#define lineWidth            2
#define lineHeight          10

#define LORA_MISO           19        //Pinout Section
#define LORA_MOSI           27
#define LORA_SCLK            5
#define LORA_CS             18
#define LORA_RST            23
#define LORA_IRQ            26        //Must be a Hardware Interrupt Pin

#define DISPLAY_CLK         22
#define DISPLAY_DATA        21

U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE, /* clock=*/ DISPLAY_CLK, /* data=*/ DISPLAY_DATA);   // ESP32 Thing, HW I2C with pin remapping

Pangodream_18650_CL BL(ADC_PIN, CONV_FACTOR, READS);

Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);

uint32_t nocolor = strip.Color(0, 0, 0);
uint32_t red = strip.Color(255, 0, 0);
uint32_t green = strip.Color(0, 90, 0);
uint32_t blue = strip.Color(0, 0, 255);
uint32_t amber = strip.Color(255, 50, 0);
uint32_t white = strip.Color(255, 255, 255);

uint32_t default_color = blue;

//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////

void printLoad(int color, int wait, int count) {

  u8g2.setDrawColor(color);

  for (int i=0; i < count; i++) {
    // load 1
    u8g2.firstPage();
    do { u8g2.drawXBM(39, 12, loadWidth, loadHeight, load1); }
    while (u8g2.nextPage());
    delay(wait);
    // load 2
    u8g2.firstPage();
    do { u8g2.drawXBM(39, 12, loadWidth, loadHeight, load2); }
    while (u8g2.nextPage());
    delay(wait);
    // load 3
    u8g2.firstPage();
    do { u8g2.drawXBM(39, 12, loadWidth, loadHeight, load3); }
    while (u8g2.nextPage());
    delay(wait);
    // load 4
    u8g2.firstPage();
    do { u8g2.drawXBM(39, 12, loadWidth, loadHeight, load4); }
    while (u8g2.nextPage());
    delay(wait);
    // load 5
    u8g2.firstPage();
    do { u8g2.drawXBM(39, 12, loadWidth, loadHeight, load5); }
    while (u8g2.nextPage());
    delay(wait);
    // load 6
    u8g2.firstPage();
    do { u8g2.drawXBM(39, 12, loadWidth, loadHeight, load6); }
    while (u8g2.nextPage());
    delay(wait);
    // load 7
    u8g2.firstPage();
    do { u8g2.drawXBM(39, 12, loadWidth, loadHeight, load7); }
    while (u8g2.nextPage());
    delay(wait);
    // load 8
    u8g2.firstPage();
    do { u8g2.drawXBM(39, 12, loadWidth, loadHeight, load8); }
    while (u8g2.nextPage());
    delay(wait);
    }
}

void printLora(int color) {
  u8g2.setDrawColor(color);
  u8g2.firstPage();
  do { u8g2.drawXBM(0, 0, loraWidth, loraHeight, lora); }
  while (u8g2.nextPage());
}

//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////

void printDisplay(String message) {   // tx Transmit Message,  rx Receive Message,   txAdr Receive Address

  sprintf(buf_name, "%s", name);         // byte
  sprintf(buf_print,"%s", message);

if ((millis() - lastGetBattery > 10000) || (initBattery == LOW)) {
    bV = BL.getBatteryVolts();
    bL = BL.getBatteryChargeLevel();
    snprintf(buf_bV, 5, "%f", bV);
    snprintf(buf_bL, 4, "%d", bL);
    initBattery = HIGH;
    lastGetBattery = millis();
  }

  u8g2.clearBuffer();					      // clear the internal memory

  //Battery Level Indicator
  u8g2.setFont(u8g2_font_6x13_tf);
  u8g2.setDisplayRotation(U8G2_R0);
  u8g2.setDrawColor(1);
  u8g2.drawStr(67,12,buf_bL);
  u8g2.drawStr(87,12,"%");
  //u8g2.drawStr(67,25,buf_bV);       // write something to the internal memory
  //u8g2.drawStr(87,25,"V");

  //Address Indicator
  u8g2.setFont(u8g2_font_6x13_tf);
  u8g2.setDrawColor(1);
  u8g2.drawXBM(58, 3, lineWidth, lineHeight, line1);
  u8g2.setDrawColor(1);
  u8g2.drawStr(3,12,buf_name);
  u8g2.drawStr(3,30,buf_print);

  //Battery Indicator
  u8g2.setFont(u8g2_font_6x13_tf);
  u8g2.setDrawColor(0);
  if ((bL >= 0) && (bL <= 10)) {
    batteryAttention = HIGH;
  }
  if ((bL >= 11) && (bL <= 25)) {
    u8g2.drawXBM(99, 0, batteryWidth, batteryHeight, battery1);
    batteryAttention = LOW;
  }
  if ((bL >= 26) && (bL <= 50)) {
    u8g2.drawXBM(99, 0, batteryWidth, batteryHeight, battery2);
    batteryAttention = LOW;
    }
  if ((bL >= 51) && (bL <= 75)) {
    u8g2.drawXBM(99, 0, batteryWidth, batteryHeight, battery3);
    batteryAttention = LOW;
  }
  if ((bL >= 76) && (bL <= 100)) {
    u8g2.drawXBM(99, 0, batteryWidth, batteryHeight, battery4);
    batteryAttention = LOW;
  }

  //Battery Attention Indicator
  if ((batteryAttention == HIGH)) {
    batteryAttentionState = !batteryAttentionState;
    if ((batteryAttentionState == HIGH)) {
      u8g2.drawXBM(99, 0, batteryWidth, batteryHeight, battery0);
    }
    if ((batteryAttentionState == LOW)) {
      u8g2.drawXBM(99, 0, batteryWidth, batteryHeight, battery1);
    } 
  }

  u8g2.sendBuffer();

  lastDisplayPrint = millis();
}

//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////

void tally(uint32_t color) {
  strip.fill(color, 0, LED_COUNT);
  strip.show();
}

uint32_t chooseColor(byte color) {
  if (color == 0x00) {return nocolor;}
  if (color == 0x01) {return red;}
  if (color == 0x02) {return green;}
  if (color == 0x03) {return amber;}
  return nocolor;
}

void relai(bool state) {
  digitalWrite(RELAI_PIN, state);
}

//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////

void setup() {

  setCpuFrequencyMhz(80);               // Set CPU Frequenz 240, 160, 80, 40, 20, 10 Mhz
  
  cpu_frequency = getCpuFrequencyMhz();
  xtal_frequency = getXtalFrequencyMhz();
  apb_frequency = getApbFrequency();

//////////////////////////////////////////////////////////////////////

  Serial.begin(115200);
  while(!Serial);
  delay(2000);  // Give time to switch to the serial monitor

  pinMode(LED_PIN_INTERNAL, OUTPUT);
  pinMode(RELAI_PIN, OUTPUT);

  u8g2.begin();
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_6x10_tf);
  u8g2.setContrast(defaultBrightnessDisplay);                  

  strip.begin();    
  strip.setBrightness(defaultBrightnessLed);    
  strip.show();

  printLora(1);
  delay(2000);
  tally(blue);
  delay(200);
  tally(nocolor);
  printLoad(1, 20, 2);
  tally(blue);
  delay(200);
  tally(nocolor);
  printLoad(1, 20, 4);
  tally(blue);
  delay(200);
  tally(nocolor);
  printLoad(1, 20, 4);
  tally(blue);
  delay(200);
  tally(nocolor);

  Serial.println(F("\nSetup ... "));
  Serial.println(F("Initialise the radio"));
  Serial.println("Version "+ version);

  sprintf(buf_init, "%s", "Initialise!");
  u8g2.drawStr(3,30,buf_init);
  sprintf(buf_version, "%s", version);
  u8g2.drawStr(99,60,buf_version);

  u8g2.sendBuffer();
  delay(2000);


  int16_t state = radio.begin();

  debug(state != RADIOLIB_ERR_NONE, F("Initialise radio failed"), state, true);
  
  // Setup the OTAA session information
  state = node.beginOTAA(joinEUI, devEUI, nwkKey, appKey);

  debug(state != RADIOLIB_ERR_NONE, F("Initialise node failed"), state, true);

  Serial.println(F("Join ('login') the LoRaWAN Network"));
  printDisplay("Join LoRa!");
  //delay(1000);
  state = node.activateOTAA();

  debug(state != RADIOLIB_LORAWAN_NEW_SESSION, F("Join failed"), state, true);

  Serial.println(F("Ready!\n"));
  printDisplay("Ready!");
  //delay(1000);

}

//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////

void loop() {

  // Send Mode
  while (mode == "send") {

    if (millis() - lastSendTime > waitSend) {  

      Serial.println(F("Sending uplink"));
      printDisplay("Sending!");
      tally(blue);
      //relai(HIGH);
      //delay(200);
      tally(nocolor);

      // This is the place to gather the sensor inputs
      // Instead of reading any real sensor, we just generate some random numbers as example
      uint8_t value1 = radio.random(100);
      uint16_t value2 = radio.random(2000);

      // Build payload byte array
      uint8_t uplinkPayload[3];
      uplinkPayload[0] = value1;
      uplinkPayload[1] = highByte(value2);   // See notes for high/lowByte functions
      uplinkPayload[2] = lowByte(value2);
      
      // Perform an uplink
      int16_t state = node.sendReceive(uplinkPayload, sizeof(uplinkPayload));
      debug(state < RADIOLIB_ERR_NONE, F("Error in sendReceive"), state, false);    

      // Check if a downlink was received 
      // (state 0 = no downlink, state 1/2 = downlink in window Rx1/Rx2)
      if(state > 0) {
        Serial.println(F("Received a downlink"));
        printDisplay("Downlink!");
        //delay(200);
      } else {
        Serial.println(F("No downlink received"));
        printDisplay("No downlink");
        //delay(200);
      }

      Serial.print(F("Next uplink in 60 seconds"));
      printDisplay("Next Uplink 60s");
      //delay(200);

      lastSendTime = millis();

    }
  }
}

//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////