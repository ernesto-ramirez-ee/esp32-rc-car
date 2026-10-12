#ifndef Pins_Arduino_h
#define Pins_Arduino_h

#include <stdint.h>

#define EESD_C3_USB

#define PIN_NEOPIXEL 10
// User LED
static const uint8_t LED_BUILTIN = 3;
#define BUILTIN_LED LED_BUILTIN  // backward compatibility
#define LED_BUILTIN LED_BUILTIN  // allow testing #ifdef LED_BUILTIN

//static const uint8_t BOOT_PIN = 9;

static const uint8_t TX = 21;
static const uint8_t RX = 20;

static const uint8_t SDA = 4;
static const uint8_t SCL = 5;

static const uint8_t SS = 10;
static const uint8_t MOSI = 7;
static const uint8_t MISO = 2;
static const uint8_t SCK = 6;

static const uint8_t A0 = 0;
static const uint8_t A1 = 1;
static const uint8_t A2 = 2;
static const uint8_t A3 = 3;
static const uint8_t A4 = 4;


static const uint8_t D0 = 2;
static const uint8_t D1 = 3;
static const uint8_t D2 = 4;
static const uint8_t D3 = 5;
static const uint8_t D4 = 6;
static const uint8_t D5 = 7;
static const uint8_t D6 = 21;
static const uint8_t D7 = 20;
static const uint8_t D8 = 8;
static const uint8_t D9 = 9;
static const uint8_t D10 = 10;


#endif /* Pins_Arduino_h */
