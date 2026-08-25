#include "Arduino.h"

#include <string.h>

rtd_arduino_stub_state_t rtd_arduino_stub_state;

void rtd_arduino_stub_reset(void)
{
    memset(&rtd_arduino_stub_state, 0, sizeof(rtd_arduino_stub_state));
}

void rtd_arduino_stub_record_event(rtd_arduino_stub_event_t event)
{
    if (rtd_arduino_stub_state.event_count < RTD_ARDUINO_STUB_MAX_EVENTS) {
        rtd_arduino_stub_state.events[rtd_arduino_stub_state.event_count] = event;
        rtd_arduino_stub_state.event_count += 1U;
    }
}

void pinMode(uint8_t pin, uint8_t mode)
{
    rtd_arduino_stub_state.last_pin = pin;
    rtd_arduino_stub_state.last_pin_mode = mode;
    rtd_arduino_stub_record_event(RTD_ARDUINO_STUB_EVENT_PIN_MODE);
}

void digitalWrite(uint8_t pin, uint8_t value)
{
    rtd_arduino_stub_state.last_pin = pin;
    rtd_arduino_stub_state.last_pin_value = value;
    rtd_arduino_stub_record_event(RTD_ARDUINO_STUB_EVENT_PIN_WRITE);
}

void delay(unsigned long duration_ms)
{
    rtd_arduino_stub_state.last_delay_ms = duration_ms;
    rtd_arduino_stub_record_event(RTD_ARDUINO_STUB_EVENT_DELAY_MS);
}

void delayMicroseconds(unsigned int duration_us)
{
    rtd_arduino_stub_state.last_delay_us = duration_us;
    rtd_arduino_stub_record_event(RTD_ARDUINO_STUB_EVENT_DELAY_US);
}
