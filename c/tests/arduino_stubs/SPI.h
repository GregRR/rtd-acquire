#ifndef RTD_ACQUIRE_TEST_SPI_H
#define RTD_ACQUIRE_TEST_SPI_H

#include <stddef.h>
#include <stdint.h>

#include "Arduino.h"

#define LSBFIRST 0U
#define MSBFIRST 1U
#define SPI_MODE0 0x00U
#define SPI_MODE1 0x04U
#define SPI_MODE2 0x08U
#define SPI_MODE3 0x0CU

class SPISettings {
public:
    SPISettings(uint32_t clock, uint8_t bit_order, uint8_t data_mode)
        : clock_hz(clock), bit_order_value(bit_order), data_mode_value(data_mode)
    {
    }

    uint32_t clock_hz;
    uint8_t bit_order_value;
    uint8_t data_mode_value;
};

class SPIClass {
public:
    SPIClass()
        : begin_calls(0U), end_calls(0U), begin_transaction_calls(0U),
          end_transaction_calls(0U), transfer_calls(0U), last_clock_hz(0U),
          last_bit_order(0U), last_data_mode(0U)
    {
    }

    void begin()
    {
        begin_calls += 1U;
        rtd_arduino_stub_record_event(RTD_ARDUINO_STUB_EVENT_SPI_BEGIN);
    }

    void end()
    {
        end_calls += 1U;
        rtd_arduino_stub_record_event(RTD_ARDUINO_STUB_EVENT_SPI_END);
    }

    void beginTransaction(SPISettings settings)
    {
        begin_transaction_calls += 1U;
        last_clock_hz = settings.clock_hz;
        last_bit_order = settings.bit_order_value;
        last_data_mode = settings.data_mode_value;
        rtd_arduino_stub_record_event(
            RTD_ARDUINO_STUB_EVENT_SPI_BEGIN_TRANSACTION
        );
    }

    uint8_t transfer(uint8_t value)
    {
        transfer_calls += 1U;
        rtd_arduino_stub_record_event(RTD_ARDUINO_STUB_EVENT_SPI_TRANSFER);
        return (uint8_t)(value ^ 0xFFU);
    }

    void endTransaction()
    {
        end_transaction_calls += 1U;
        rtd_arduino_stub_record_event(
            RTD_ARDUINO_STUB_EVENT_SPI_END_TRANSACTION
        );
    }

    unsigned int begin_calls;
    unsigned int end_calls;
    unsigned int begin_transaction_calls;
    unsigned int end_transaction_calls;
    unsigned int transfer_calls;
    uint32_t last_clock_hz;
    uint8_t last_bit_order;
    uint8_t last_data_mode;
};

#endif
