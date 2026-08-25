#include <SPI.h>

#include <RtdAcquire.h>

static const uint8_t MAX31865_CS_PIN = 10U;

static rtd_acquire_arduino_avr_spi_context_t spi_context;
static rtd_acquire_spi_t spi_hal;
static rtd_acquire_delay_t delay_hal;

static rtd_acquire_diagnostic_t diagnostics[
    RTD_ACQUIRE_MAX31865_MAX_DIAGNOSTICS
];
static rtd_acquire_native_evidence_t native_evidence[
    RTD_ACQUIRE_MAX31865_MAX_NATIVE_EVIDENCE
];
static rtd_acquire_measurement_t measurement;
static bool adapter_ready = false;

static const rtd_acquire_spi_settings_t spi_settings = {
    0U,
    1U,
    1000000U,
    RTD_ACQUIRE_SPI_MSB_FIRST,
    8U,
    true,
};

static const rtd_acquire_max31865_config_t max31865_config = {
    430.0F,
    3U,
    60U,
    false,
    0.0F,
    false,
    0.0F,
};

static const rtd_acquire_max31865_timing_t max31865_timing = {
    RTD_ACQUIRE_MAX31865_DEFAULT_INPUT_FILTER_TIME_CONSTANT_US,
};

void setup()
{
    Serial.begin(115200);

    rtd_acquire_measurement_init(
        &measurement,
        diagnostics,
        RTD_ACQUIRE_MAX31865_MAX_DIAGNOSTICS,
        native_evidence,
        RTD_ACQUIRE_MAX31865_MAX_NATIVE_EVIDENCE
    );

    adapter_ready = rtd_acquire_arduino_avr_spi_init(
            &spi_context,
            &spi_hal,
            &SPI,
            MAX31865_CS_PIN,
            &spi_settings
        )
        && rtd_acquire_arduino_avr_delay_init(&delay_hal);

    if (!adapter_ready) {
        Serial.println("rtd-acquire adapter initialization failed");
    }
}

void loop()
{
    if (!adapter_ready) {
        delay(1000UL);
        return;
    }

    rtd_acquire_max31865_result_t result = rtd_acquire_max31865_read(
        &spi_hal,
        &delay_hal,
        &max31865_config,
        &max31865_timing,
        &measurement
    );

    if (result == RTD_ACQUIRE_MAX31865_RESULT_OK
        && measurement.has_resistance) {
        Serial.println(measurement.resistance_ohms, 6);
    } else {
        Serial.print("MAX31865 read result: ");
        Serial.println((int)result);
    }

    delay(1000UL);
}
