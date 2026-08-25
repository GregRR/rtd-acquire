#ifndef RTD_ACQUIRE_TEST_ARDUINO_H
#define RTD_ACQUIRE_TEST_ARDUINO_H

#include <stddef.h>
#include <stdint.h>

#define LOW 0U
#define HIGH 1U
#define INPUT 0U
#define OUTPUT 1U

#define RTD_ARDUINO_STUB_MAX_EVENTS 64U

typedef enum {
    RTD_ARDUINO_STUB_EVENT_SPI_BEGIN = 1,
    RTD_ARDUINO_STUB_EVENT_PIN_WRITE = 2,
    RTD_ARDUINO_STUB_EVENT_PIN_MODE = 3,
    RTD_ARDUINO_STUB_EVENT_SPI_BEGIN_TRANSACTION = 4,
    RTD_ARDUINO_STUB_EVENT_SPI_TRANSFER = 5,
    RTD_ARDUINO_STUB_EVENT_SPI_END_TRANSACTION = 6,
    RTD_ARDUINO_STUB_EVENT_SPI_END = 7,
    RTD_ARDUINO_STUB_EVENT_DELAY_MS = 8,
    RTD_ARDUINO_STUB_EVENT_DELAY_US = 9
} rtd_arduino_stub_event_t;

typedef struct {
    rtd_arduino_stub_event_t events[RTD_ARDUINO_STUB_MAX_EVENTS];
    size_t event_count;
    uint8_t last_pin;
    uint8_t last_pin_value;
    uint8_t last_pin_mode;
    unsigned long last_delay_ms;
    unsigned int last_delay_us;
} rtd_arduino_stub_state_t;

extern rtd_arduino_stub_state_t rtd_arduino_stub_state;

void rtd_arduino_stub_reset(void);
void rtd_arduino_stub_record_event(rtd_arduino_stub_event_t event);
void pinMode(uint8_t pin, uint8_t mode);
void digitalWrite(uint8_t pin, uint8_t value);
void delay(unsigned long duration_ms);
void delayMicroseconds(unsigned int duration_us);

#endif
