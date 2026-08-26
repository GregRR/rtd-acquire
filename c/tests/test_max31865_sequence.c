#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "rtd_acquire/max31865.h"

#define MAX_TRANSFER_BYTES 5U

typedef struct {
    size_t length;
    uint8_t tx[MAX_TRANSFER_BYTES];
    uint8_t rx[MAX_TRANSFER_BYTES];
    rtd_acquire_spi_result_t result;
} transfer_event_t;

typedef struct {
    const transfer_event_t *events;
    size_t event_count;
    size_t next_event;
} fake_spi_context_t;

typedef struct {
    uint32_t duration_us;
    rtd_acquire_delay_result_t result;
} delay_event_t;

typedef struct {
    const delay_event_t *events;
    size_t event_count;
    size_t next_event;
} fake_delay_context_t;

static rtd_acquire_spi_result_t fake_spi_transfer(
    void *context,
    const uint8_t *tx,
    uint8_t *rx,
    size_t length
)
{
    fake_spi_context_t *fake = context;
    const transfer_event_t *event;

    assert(fake != NULL);
    assert(fake->next_event < fake->event_count);
    event = &fake->events[fake->next_event];
    ++fake->next_event;

    assert(length == event->length);
    assert(length <= MAX_TRANSFER_BYTES);
    assert(memcmp(tx, event->tx, length) == 0);
    if (event->result == RTD_ACQUIRE_SPI_OK) {
        memcpy(rx, event->rx, length);
    }
    return event->result;
}

static rtd_acquire_delay_result_t fake_delay_us(
    void *context,
    uint32_t duration_us
)
{
    fake_delay_context_t *fake = context;
    const delay_event_t *event;

    assert(fake != NULL);
    assert(fake->next_event < fake->event_count);
    event = &fake->events[fake->next_event];
    ++fake->next_event;

    assert(duration_us == event->duration_us);
    return event->result;
}

static rtd_acquire_max31865_config_t make_config(void)
{
    return (rtd_acquire_max31865_config_t){
        .reference_resistance_ohms = 430.0F,
        .wire_count = 2U,
        .filter_frequency_hz = 60U,
        .has_low_fault_threshold = false,
        .low_fault_threshold_ohms = 0.0F,
        .has_high_fault_threshold = false,
        .high_fault_threshold_ohms = 0.0F,
    };
}

static rtd_acquire_max31865_timing_t make_timing(void)
{
    return (rtd_acquire_max31865_timing_t){
        .input_filter_time_constant_us =
            RTD_ACQUIRE_MAX31865_DEFAULT_INPUT_FILTER_TIME_CONSTANT_US,
    };
}

static rtd_acquire_spi_t make_spi(fake_spi_context_t *context)
{
    return (rtd_acquire_spi_t){
        .context = context,
        .settings = {
            .clock_polarity = 0U,
            .clock_phase = 1U,
            .clock_frequency_hz = 1000000U,
            .bit_order = RTD_ACQUIRE_SPI_MSB_FIRST,
            .bits_per_word = 8U,
            .chip_select_active_low = true,
        },
        .transfer = fake_spi_transfer,
    };
}

static rtd_acquire_delay_t make_delay(fake_delay_context_t *context)
{
    return (rtd_acquire_delay_t){
        .context = context,
        .delay_us = fake_delay_us,
    };
}

static rtd_acquire_measurement_t make_measurement(
    rtd_acquire_diagnostic_t *diagnostics,
    size_t diagnostic_capacity,
    rtd_acquire_native_evidence_t *native_evidence,
    size_t native_evidence_capacity
)
{
    rtd_acquire_measurement_t measurement;

    rtd_acquire_measurement_init(
        &measurement,
        diagnostics,
        diagnostic_capacity,
        native_evidence,
        native_evidence_capacity
    );
    return measurement;
}

static void assert_fake_consumed(
    const fake_spi_context_t *spi,
    const fake_delay_context_t *delay
)
{
    assert(spi->next_event == spi->event_count);
    assert(delay->next_event == delay->event_count);
}

static void test_successful_one_shot_sequence(void)
{
    const transfer_event_t transfers[] = {
        {5U, {0x83U, 0xFFU, 0xFFU, 0x00U, 0x00U}, {0}, RTD_ACQUIRE_SPI_OK},
        {2U, {0x80U, 0x82U}, {0}, RTD_ACQUIRE_SPI_OK},
        {2U, {0x80U, 0x84U}, {0}, RTD_ACQUIRE_SPI_OK},
        {2U, {0x80U, 0xA0U}, {0}, RTD_ACQUIRE_SPI_OK},
        {3U, {0x01U, 0x00U, 0x00U}, {0x00U, 0x40U, 0x00U}, RTD_ACQUIRE_SPI_OK},
        {2U, {0x80U, 0x00U}, {0}, RTD_ACQUIRE_SPI_OK},
    };
    const delay_event_t delays[] = {
        {1011U, RTD_ACQUIRE_DELAY_OK},
        {600U, RTD_ACQUIRE_DELAY_OK},
        {1005U, RTD_ACQUIRE_DELAY_OK},
        {55000U, RTD_ACQUIRE_DELAY_OK},
    };
    fake_spi_context_t spi_context = {transfers, 6U, 0U};
    fake_delay_context_t delay_context = {delays, 4U, 0U};
    rtd_acquire_spi_t spi = make_spi(&spi_context);
    rtd_acquire_delay_t delay = make_delay(&delay_context);
    rtd_acquire_max31865_config_t config = make_config();
    rtd_acquire_max31865_timing_t timing = make_timing();
    rtd_acquire_diagnostic_t diagnostics[RTD_ACQUIRE_MAX31865_MAX_DIAGNOSTICS];
    rtd_acquire_native_evidence_t evidence[
        RTD_ACQUIRE_MAX31865_MAX_NATIVE_EVIDENCE
    ];
    rtd_acquire_measurement_t measurement = make_measurement(
        diagnostics,
        RTD_ACQUIRE_MAX31865_MAX_DIAGNOSTICS,
        evidence,
        RTD_ACQUIRE_MAX31865_MAX_NATIVE_EVIDENCE
    );

    timing.input_filter_time_constant_us = 1U;

    assert(
        rtd_acquire_max31865_read(
            &spi,
            &delay,
            &config,
            &timing,
            &measurement
        ) == RTD_ACQUIRE_MAX31865_RESULT_OK
    );
    assert(measurement.has_resistance);
    assert(measurement.resistance_ohms == 107.5F);
    assert(measurement.diagnostic_count == 0U);
    assert_fake_consumed(&spi_context, &delay_context);
}

static void test_fault_flag_reads_status_and_ignores_reserved_bits(void)
{
    const transfer_event_t transfers[] = {
        {5U, {0x83U, 0xFFU, 0xFFU, 0x00U, 0x00U}, {0}, RTD_ACQUIRE_SPI_OK},
        {2U, {0x80U, 0x82U}, {0}, RTD_ACQUIRE_SPI_OK},
        {2U, {0x80U, 0x84U}, {0}, RTD_ACQUIRE_SPI_OK},
        {2U, {0x80U, 0xA0U}, {0}, RTD_ACQUIRE_SPI_OK},
        {3U, {0x01U, 0x00U, 0x00U}, {0x00U, 0x40U, 0x01U}, RTD_ACQUIRE_SPI_OK},
        {2U, {0x07U, 0x00U}, {0x00U, 0x83U}, RTD_ACQUIRE_SPI_OK},
        {2U, {0x80U, 0x00U}, {0}, RTD_ACQUIRE_SPI_OK},
    };
    const delay_event_t delays[] = {
        {11500U, RTD_ACQUIRE_DELAY_OK},
        {600U, RTD_ACQUIRE_DELAY_OK},
        {6000U, RTD_ACQUIRE_DELAY_OK},
        {55000U, RTD_ACQUIRE_DELAY_OK},
    };
    fake_spi_context_t spi_context = {transfers, 7U, 0U};
    fake_delay_context_t delay_context = {delays, 4U, 0U};
    rtd_acquire_spi_t spi = make_spi(&spi_context);
    rtd_acquire_delay_t delay = make_delay(&delay_context);
    rtd_acquire_max31865_config_t config = make_config();
    rtd_acquire_max31865_timing_t timing = make_timing();
    rtd_acquire_diagnostic_t diagnostics[1];
    rtd_acquire_native_evidence_t evidence[1];
    rtd_acquire_measurement_t measurement = make_measurement(
        diagnostics,
        1U,
        evidence,
        1U
    );

    assert(
        rtd_acquire_max31865_read(
            &spi,
            &delay,
            &config,
            &timing,
            &measurement
        ) == RTD_ACQUIRE_MAX31865_RESULT_OK
    );
    assert(measurement.has_resistance);
    assert(measurement.resistance_ohms == 107.5F);
    assert(measurement.diagnostic_count == 1U);
    assert(
        diagnostics[0].code
        == RTD_ACQUIRE_DIAGNOSTIC_CODE_RESISTANCE_HIGH_THRESHOLD
    );
    assert_fake_consumed(&spi_context, &delay_context);
}

static void test_three_wire_50_hz_sequence_and_timing(void)
{
    const transfer_event_t transfers[] = {
        {5U, {0x83U, 0x40U, 0x00U, 0x20U, 0x00U}, {0}, RTD_ACQUIRE_SPI_OK},
        {2U, {0x80U, 0x93U}, {0}, RTD_ACQUIRE_SPI_OK},
        {2U, {0x80U, 0x95U}, {0}, RTD_ACQUIRE_SPI_OK},
        {2U, {0x80U, 0xB1U}, {0}, RTD_ACQUIRE_SPI_OK},
        {3U, {0x01U, 0x00U, 0x00U}, {0x00U, 0x40U, 0x00U}, RTD_ACQUIRE_SPI_OK},
        {2U, {0x80U, 0x11U}, {0}, RTD_ACQUIRE_SPI_OK},
    };
    const delay_event_t delays[] = {
        {11500U, RTD_ACQUIRE_DELAY_OK},
        {600U, RTD_ACQUIRE_DELAY_OK},
        {6000U, RTD_ACQUIRE_DELAY_OK},
        {66000U, RTD_ACQUIRE_DELAY_OK},
    };
    fake_spi_context_t spi_context = {transfers, 6U, 0U};
    fake_delay_context_t delay_context = {delays, 4U, 0U};
    rtd_acquire_spi_t spi = make_spi(&spi_context);
    rtd_acquire_delay_t delay = make_delay(&delay_context);
    rtd_acquire_max31865_config_t config = make_config();
    rtd_acquire_max31865_timing_t timing = make_timing();
    rtd_acquire_measurement_t measurement = make_measurement(NULL, 0U, NULL, 0U);

    spi.settings.clock_polarity = 1U;
    spi.settings.clock_frequency_hz = 5000000U;
    config.reference_resistance_ohms = 400.0F;
    config.wire_count = 3U;
    config.filter_frequency_hz = 50U;
    config.has_low_fault_threshold = true;
    config.low_fault_threshold_ohms = 50.0F;
    config.has_high_fault_threshold = true;
    config.high_fault_threshold_ohms = 100.0F;

    assert(
        rtd_acquire_max31865_read(
            &spi,
            &delay,
            &config,
            &timing,
            &measurement
        ) == RTD_ACQUIRE_MAX31865_RESULT_OK
    );
    assert_fake_consumed(&spi_context, &delay_context);
}

static void test_spi_settings_are_validated_before_io(void)
{
    fake_spi_context_t spi_context = {NULL, 0U, 0U};
    fake_delay_context_t delay_context = {NULL, 0U, 0U};
    rtd_acquire_spi_t spi = make_spi(&spi_context);
    rtd_acquire_delay_t delay = make_delay(&delay_context);
    rtd_acquire_max31865_config_t config = make_config();
    rtd_acquire_max31865_timing_t timing = make_timing();
    rtd_acquire_measurement_t measurement = make_measurement(NULL, 0U, NULL, 0U);

    spi.settings.clock_phase = 0U;
    assert(
        rtd_acquire_max31865_read(
            &spi,
            &delay,
            &config,
            &timing,
            &measurement
        ) == RTD_ACQUIRE_MAX31865_RESULT_CONFIGURATION_ERROR
    );
    assert_fake_consumed(&spi_context, &delay_context);

    spi.settings.clock_phase = 1U;
    spi.settings.clock_frequency_hz = 0U;
    assert(
        rtd_acquire_max31865_read(
            &spi,
            &delay,
            &config,
            &timing,
            &measurement
        ) == RTD_ACQUIRE_MAX31865_RESULT_CONFIGURATION_ERROR
    );
    assert_fake_consumed(&spi_context, &delay_context);

    spi = make_spi(&spi_context);
    spi.settings.clock_polarity = 2U;
    assert(
        rtd_acquire_max31865_read(
            &spi, &delay, &config, &timing, &measurement
        ) == RTD_ACQUIRE_MAX31865_RESULT_CONFIGURATION_ERROR
    );

    spi = make_spi(&spi_context);
    spi.settings.clock_frequency_hz = 5000001U;
    assert(
        rtd_acquire_max31865_read(
            &spi, &delay, &config, &timing, &measurement
        ) == RTD_ACQUIRE_MAX31865_RESULT_CONFIGURATION_ERROR
    );

    spi = make_spi(&spi_context);
    spi.settings.bit_order = RTD_ACQUIRE_SPI_LSB_FIRST;
    assert(
        rtd_acquire_max31865_read(
            &spi, &delay, &config, &timing, &measurement
        ) == RTD_ACQUIRE_MAX31865_RESULT_CONFIGURATION_ERROR
    );

    spi = make_spi(&spi_context);
    spi.settings.bits_per_word = 16U;
    assert(
        rtd_acquire_max31865_read(
            &spi, &delay, &config, &timing, &measurement
        ) == RTD_ACQUIRE_MAX31865_RESULT_CONFIGURATION_ERROR
    );

    spi = make_spi(&spi_context);
    spi.settings.chip_select_active_low = false;
    assert(
        rtd_acquire_max31865_read(
            &spi, &delay, &config, &timing, &measurement
        ) == RTD_ACQUIRE_MAX31865_RESULT_CONFIGURATION_ERROR
    );
    assert_fake_consumed(&spi_context, &delay_context);
}

static void test_delay_failure_preserves_error_and_attempts_bias_off(void)
{
    const transfer_event_t transfers[] = {
        {5U, {0x83U, 0xFFU, 0xFFU, 0x00U, 0x00U}, {0}, RTD_ACQUIRE_SPI_OK},
        {2U, {0x80U, 0x82U}, {0}, RTD_ACQUIRE_SPI_OK},
        {2U, {0x80U, 0x00U}, {0}, RTD_ACQUIRE_SPI_IO_ERROR},
    };
    const delay_event_t delays[] = {
        {11500U, RTD_ACQUIRE_DELAY_ERROR},
    };
    fake_spi_context_t spi_context = {transfers, 3U, 0U};
    fake_delay_context_t delay_context = {delays, 1U, 0U};
    rtd_acquire_spi_t spi = make_spi(&spi_context);
    rtd_acquire_delay_t delay = make_delay(&delay_context);
    rtd_acquire_max31865_config_t config = make_config();
    rtd_acquire_max31865_timing_t timing = make_timing();
    rtd_acquire_measurement_t measurement = make_measurement(NULL, 0U, NULL, 0U);

    measurement.has_resistance = true;
    measurement.resistance_ohms = 12.5F;

    assert(
        rtd_acquire_max31865_read(
            &spi,
            &delay,
            &config,
            &timing,
            &measurement
        ) == RTD_ACQUIRE_MAX31865_RESULT_DELAY_ERROR
    );
    assert(measurement.has_resistance);
    assert(measurement.resistance_ohms == 12.5F);
    assert_fake_consumed(&spi_context, &delay_context);
}

static void test_spi_failure_after_bias_attempts_bias_off(void)
{
    const transfer_event_t transfers[] = {
        {5U, {0x83U, 0xFFU, 0xFFU, 0x00U, 0x00U}, {0}, RTD_ACQUIRE_SPI_OK},
        {2U, {0x80U, 0x82U}, {0}, RTD_ACQUIRE_SPI_OK},
        {2U, {0x80U, 0x84U}, {0}, RTD_ACQUIRE_SPI_IO_ERROR},
        {2U, {0x80U, 0x00U}, {0}, RTD_ACQUIRE_SPI_OK},
    };
    const delay_event_t delays[] = {
        {11500U, RTD_ACQUIRE_DELAY_OK},
    };
    fake_spi_context_t spi_context = {transfers, 4U, 0U};
    fake_delay_context_t delay_context = {delays, 1U, 0U};
    rtd_acquire_spi_t spi = make_spi(&spi_context);
    rtd_acquire_delay_t delay = make_delay(&delay_context);
    rtd_acquire_max31865_config_t config = make_config();
    rtd_acquire_max31865_timing_t timing = make_timing();
    rtd_acquire_measurement_t measurement = make_measurement(NULL, 0U, NULL, 0U);

    assert(
        rtd_acquire_max31865_read(
            &spi,
            &delay,
            &config,
            &timing,
            &measurement
        ) == RTD_ACQUIRE_MAX31865_RESULT_SPI_IO_ERROR
    );
    assert_fake_consumed(&spi_context, &delay_context);
}

static void test_failed_initial_bias_write_still_attempts_bias_off(void)
{
    const transfer_event_t transfers[] = {
        {5U, {0x83U, 0xFFU, 0xFFU, 0x00U, 0x00U}, {0}, RTD_ACQUIRE_SPI_OK},
        {2U, {0x80U, 0x82U}, {0}, RTD_ACQUIRE_SPI_IO_ERROR},
        {2U, {0x80U, 0x00U}, {0}, RTD_ACQUIRE_SPI_OK},
    };
    fake_spi_context_t spi_context = {transfers, 3U, 0U};
    fake_delay_context_t delay_context = {NULL, 0U, 0U};
    rtd_acquire_spi_t spi = make_spi(&spi_context);
    rtd_acquire_delay_t delay = make_delay(&delay_context);
    rtd_acquire_max31865_config_t config = make_config();
    rtd_acquire_max31865_timing_t timing = make_timing();
    rtd_acquire_measurement_t measurement = make_measurement(NULL, 0U, NULL, 0U);

    assert(
        rtd_acquire_max31865_read(
            &spi,
            &delay,
            &config,
            &timing,
            &measurement
        ) == RTD_ACQUIRE_MAX31865_RESULT_SPI_IO_ERROR
    );
    assert_fake_consumed(&spi_context, &delay_context);
}

static void test_final_bias_off_failure_leaves_measurement_untouched(void)
{
    const transfer_event_t transfers[] = {
        {5U, {0x83U, 0xFFU, 0xFFU, 0x00U, 0x00U}, {0}, RTD_ACQUIRE_SPI_OK},
        {2U, {0x80U, 0x82U}, {0}, RTD_ACQUIRE_SPI_OK},
        {2U, {0x80U, 0x84U}, {0}, RTD_ACQUIRE_SPI_OK},
        {2U, {0x80U, 0xA0U}, {0}, RTD_ACQUIRE_SPI_OK},
        {3U, {0x01U, 0x00U, 0x00U}, {0x00U, 0x40U, 0x00U}, RTD_ACQUIRE_SPI_OK},
        {2U, {0x80U, 0x00U}, {0}, RTD_ACQUIRE_SPI_IO_ERROR},
    };
    const delay_event_t delays[] = {
        {11500U, RTD_ACQUIRE_DELAY_OK},
        {600U, RTD_ACQUIRE_DELAY_OK},
        {6000U, RTD_ACQUIRE_DELAY_OK},
        {55000U, RTD_ACQUIRE_DELAY_OK},
    };
    fake_spi_context_t spi_context = {transfers, 6U, 0U};
    fake_delay_context_t delay_context = {delays, 4U, 0U};
    rtd_acquire_spi_t spi = make_spi(&spi_context);
    rtd_acquire_delay_t delay = make_delay(&delay_context);
    rtd_acquire_max31865_config_t config = make_config();
    rtd_acquire_max31865_timing_t timing = make_timing();
    rtd_acquire_measurement_t measurement = make_measurement(NULL, 0U, NULL, 0U);

    measurement.has_resistance = true;
    measurement.resistance_ohms = 12.5F;

    assert(
        rtd_acquire_max31865_read(
            &spi,
            &delay,
            &config,
            &timing,
            &measurement
        ) == RTD_ACQUIRE_MAX31865_RESULT_SPI_IO_ERROR
    );
    assert(measurement.has_resistance);
    assert(measurement.resistance_ohms == 12.5F);
    assert_fake_consumed(&spi_context, &delay_context);
}


static void test_fault_cycle_delay_failure_attempts_bias_off(void)
{
    const transfer_event_t transfers[] = {
        {5U, {0x83U, 0xFFU, 0xFFU, 0x00U, 0x00U}, {0}, RTD_ACQUIRE_SPI_OK},
        {2U, {0x80U, 0x82U}, {0}, RTD_ACQUIRE_SPI_OK},
        {2U, {0x80U, 0x84U}, {0}, RTD_ACQUIRE_SPI_OK},
        {2U, {0x80U, 0x00U}, {0}, RTD_ACQUIRE_SPI_OK},
    };
    const delay_event_t delays[] = {
        {11500U, RTD_ACQUIRE_DELAY_OK},
        {600U, RTD_ACQUIRE_DELAY_ERROR},
    };
    fake_spi_context_t spi_context = {transfers, 4U, 0U};
    fake_delay_context_t delay_context = {delays, 2U, 0U};
    rtd_acquire_spi_t spi = make_spi(&spi_context);
    rtd_acquire_delay_t delay = make_delay(&delay_context);
    rtd_acquire_max31865_config_t config = make_config();
    rtd_acquire_max31865_timing_t timing = make_timing();
    rtd_acquire_measurement_t measurement = make_measurement(NULL, 0U, NULL, 0U);

    assert(
        rtd_acquire_max31865_read(
            &spi, &delay, &config, &timing, &measurement
        ) == RTD_ACQUIRE_MAX31865_RESULT_DELAY_ERROR
    );
    assert_fake_consumed(&spi_context, &delay_context);
}

static void test_post_fault_delay_failure_attempts_bias_off(void)
{
    const transfer_event_t transfers[] = {
        {5U, {0x83U, 0xFFU, 0xFFU, 0x00U, 0x00U}, {0}, RTD_ACQUIRE_SPI_OK},
        {2U, {0x80U, 0x82U}, {0}, RTD_ACQUIRE_SPI_OK},
        {2U, {0x80U, 0x84U}, {0}, RTD_ACQUIRE_SPI_OK},
        {2U, {0x80U, 0x00U}, {0}, RTD_ACQUIRE_SPI_OK},
    };
    const delay_event_t delays[] = {
        {11500U, RTD_ACQUIRE_DELAY_OK},
        {600U, RTD_ACQUIRE_DELAY_OK},
        {6000U, RTD_ACQUIRE_DELAY_ERROR},
    };
    fake_spi_context_t spi_context = {transfers, 4U, 0U};
    fake_delay_context_t delay_context = {delays, 3U, 0U};
    rtd_acquire_spi_t spi = make_spi(&spi_context);
    rtd_acquire_delay_t delay = make_delay(&delay_context);
    rtd_acquire_max31865_config_t config = make_config();
    rtd_acquire_max31865_timing_t timing = make_timing();
    rtd_acquire_measurement_t measurement = make_measurement(NULL, 0U, NULL, 0U);

    assert(
        rtd_acquire_max31865_read(
            &spi, &delay, &config, &timing, &measurement
        ) == RTD_ACQUIRE_MAX31865_RESULT_DELAY_ERROR
    );
    assert_fake_consumed(&spi_context, &delay_context);
}

static void test_conversion_delay_failure_attempts_bias_off(void)
{
    const transfer_event_t transfers[] = {
        {5U, {0x83U, 0xFFU, 0xFFU, 0x00U, 0x00U}, {0}, RTD_ACQUIRE_SPI_OK},
        {2U, {0x80U, 0x82U}, {0}, RTD_ACQUIRE_SPI_OK},
        {2U, {0x80U, 0x84U}, {0}, RTD_ACQUIRE_SPI_OK},
        {2U, {0x80U, 0xA0U}, {0}, RTD_ACQUIRE_SPI_OK},
        {2U, {0x80U, 0x00U}, {0}, RTD_ACQUIRE_SPI_OK},
    };
    const delay_event_t delays[] = {
        {11500U, RTD_ACQUIRE_DELAY_OK},
        {600U, RTD_ACQUIRE_DELAY_OK},
        {6000U, RTD_ACQUIRE_DELAY_OK},
        {55000U, RTD_ACQUIRE_DELAY_ERROR},
    };
    fake_spi_context_t spi_context = {transfers, 5U, 0U};
    fake_delay_context_t delay_context = {delays, 4U, 0U};
    rtd_acquire_spi_t spi = make_spi(&spi_context);
    rtd_acquire_delay_t delay = make_delay(&delay_context);
    rtd_acquire_max31865_config_t config = make_config();
    rtd_acquire_max31865_timing_t timing = make_timing();
    rtd_acquire_measurement_t measurement = make_measurement(NULL, 0U, NULL, 0U);

    assert(
        rtd_acquire_max31865_read(
            &spi, &delay, &config, &timing, &measurement
        ) == RTD_ACQUIRE_MAX31865_RESULT_DELAY_ERROR
    );
    assert_fake_consumed(&spi_context, &delay_context);
}

static void test_threshold_write_failure_returns_without_bias_cleanup(void)
{
    const transfer_event_t transfers[] = {
        {5U, {0x83U, 0xFFU, 0xFFU, 0x00U, 0x00U}, {0}, RTD_ACQUIRE_SPI_IO_ERROR},
    };
    fake_spi_context_t spi_context = {transfers, 1U, 0U};
    fake_delay_context_t delay_context = {NULL, 0U, 0U};
    rtd_acquire_spi_t spi = make_spi(&spi_context);
    rtd_acquire_delay_t delay = make_delay(&delay_context);
    rtd_acquire_max31865_config_t config = make_config();
    rtd_acquire_max31865_timing_t timing = make_timing();
    rtd_acquire_measurement_t measurement = make_measurement(NULL, 0U, NULL, 0U);

    assert(
        rtd_acquire_max31865_read(
            &spi, &delay, &config, &timing, &measurement
        ) == RTD_ACQUIRE_MAX31865_RESULT_SPI_IO_ERROR
    );
    assert_fake_consumed(&spi_context, &delay_context);
}

static void test_one_shot_write_failure_attempts_bias_off(void)
{
    const transfer_event_t transfers[] = {
        {5U, {0x83U, 0xFFU, 0xFFU, 0x00U, 0x00U}, {0}, RTD_ACQUIRE_SPI_OK},
        {2U, {0x80U, 0x82U}, {0}, RTD_ACQUIRE_SPI_OK},
        {2U, {0x80U, 0x84U}, {0}, RTD_ACQUIRE_SPI_OK},
        {2U, {0x80U, 0xA0U}, {0}, RTD_ACQUIRE_SPI_IO_ERROR},
        {2U, {0x80U, 0x00U}, {0}, RTD_ACQUIRE_SPI_OK},
    };
    const delay_event_t delays[] = {
        {11500U, RTD_ACQUIRE_DELAY_OK},
        {600U, RTD_ACQUIRE_DELAY_OK},
        {6000U, RTD_ACQUIRE_DELAY_OK},
    };
    fake_spi_context_t spi_context = {transfers, 5U, 0U};
    fake_delay_context_t delay_context = {delays, 3U, 0U};
    rtd_acquire_spi_t spi = make_spi(&spi_context);
    rtd_acquire_delay_t delay = make_delay(&delay_context);
    rtd_acquire_max31865_config_t config = make_config();
    rtd_acquire_max31865_timing_t timing = make_timing();
    rtd_acquire_measurement_t measurement = make_measurement(NULL, 0U, NULL, 0U);

    assert(
        rtd_acquire_max31865_read(
            &spi, &delay, &config, &timing, &measurement
        ) == RTD_ACQUIRE_MAX31865_RESULT_SPI_IO_ERROR
    );
    assert_fake_consumed(&spi_context, &delay_context);
}

static void test_rtd_read_failure_attempts_bias_off(void)
{
    const transfer_event_t transfers[] = {
        {5U, {0x83U, 0xFFU, 0xFFU, 0x00U, 0x00U}, {0}, RTD_ACQUIRE_SPI_OK},
        {2U, {0x80U, 0x82U}, {0}, RTD_ACQUIRE_SPI_OK},
        {2U, {0x80U, 0x84U}, {0}, RTD_ACQUIRE_SPI_OK},
        {2U, {0x80U, 0xA0U}, {0}, RTD_ACQUIRE_SPI_OK},
        {3U, {0x01U, 0x00U, 0x00U}, {0}, RTD_ACQUIRE_SPI_IO_ERROR},
        {2U, {0x80U, 0x00U}, {0}, RTD_ACQUIRE_SPI_OK},
    };
    const delay_event_t delays[] = {
        {11500U, RTD_ACQUIRE_DELAY_OK},
        {600U, RTD_ACQUIRE_DELAY_OK},
        {6000U, RTD_ACQUIRE_DELAY_OK},
        {55000U, RTD_ACQUIRE_DELAY_OK},
    };
    fake_spi_context_t spi_context = {transfers, 6U, 0U};
    fake_delay_context_t delay_context = {delays, 4U, 0U};
    rtd_acquire_spi_t spi = make_spi(&spi_context);
    rtd_acquire_delay_t delay = make_delay(&delay_context);
    rtd_acquire_max31865_config_t config = make_config();
    rtd_acquire_max31865_timing_t timing = make_timing();
    rtd_acquire_measurement_t measurement = make_measurement(NULL, 0U, NULL, 0U);

    assert(
        rtd_acquire_max31865_read(
            &spi, &delay, &config, &timing, &measurement
        ) == RTD_ACQUIRE_MAX31865_RESULT_SPI_IO_ERROR
    );
    assert_fake_consumed(&spi_context, &delay_context);
}

static void test_fault_status_read_failure_attempts_bias_off(void)
{
    const transfer_event_t transfers[] = {
        {5U, {0x83U, 0xFFU, 0xFFU, 0x00U, 0x00U}, {0}, RTD_ACQUIRE_SPI_OK},
        {2U, {0x80U, 0x82U}, {0}, RTD_ACQUIRE_SPI_OK},
        {2U, {0x80U, 0x84U}, {0}, RTD_ACQUIRE_SPI_OK},
        {2U, {0x80U, 0xA0U}, {0}, RTD_ACQUIRE_SPI_OK},
        {3U, {0x01U, 0x00U, 0x00U}, {0x00U, 0x40U, 0x01U}, RTD_ACQUIRE_SPI_OK},
        {2U, {0x07U, 0x00U}, {0}, RTD_ACQUIRE_SPI_IO_ERROR},
        {2U, {0x80U, 0x00U}, {0}, RTD_ACQUIRE_SPI_OK},
    };
    const delay_event_t delays[] = {
        {11500U, RTD_ACQUIRE_DELAY_OK},
        {600U, RTD_ACQUIRE_DELAY_OK},
        {6000U, RTD_ACQUIRE_DELAY_OK},
        {55000U, RTD_ACQUIRE_DELAY_OK},
    };
    fake_spi_context_t spi_context = {transfers, 7U, 0U};
    fake_delay_context_t delay_context = {delays, 4U, 0U};
    rtd_acquire_spi_t spi = make_spi(&spi_context);
    rtd_acquire_delay_t delay = make_delay(&delay_context);
    rtd_acquire_max31865_config_t config = make_config();
    rtd_acquire_max31865_timing_t timing = make_timing();
    rtd_acquire_measurement_t measurement = make_measurement(NULL, 0U, NULL, 0U);

    assert(
        rtd_acquire_max31865_read(
            &spi, &delay, &config, &timing, &measurement
        ) == RTD_ACQUIRE_MAX31865_RESULT_SPI_IO_ERROR
    );
    assert_fake_consumed(&spi_context, &delay_context);
}

static void test_repeated_reads_replace_measurement_state(void)
{
    const transfer_event_t transfers[] = {
        {5U, {0x83U, 0xFFU, 0xFFU, 0x00U, 0x00U}, {0}, RTD_ACQUIRE_SPI_OK},
        {2U, {0x80U, 0x82U}, {0}, RTD_ACQUIRE_SPI_OK},
        {2U, {0x80U, 0x84U}, {0}, RTD_ACQUIRE_SPI_OK},
        {2U, {0x80U, 0xA0U}, {0}, RTD_ACQUIRE_SPI_OK},
        {3U, {0x01U, 0x00U, 0x00U}, {0x00U, 0x40U, 0x01U}, RTD_ACQUIRE_SPI_OK},
        {2U, {0x07U, 0x00U}, {0x00U, 0x20U}, RTD_ACQUIRE_SPI_OK},
        {2U, {0x80U, 0x00U}, {0}, RTD_ACQUIRE_SPI_OK},
        {5U, {0x83U, 0xFFU, 0xFFU, 0x00U, 0x00U}, {0}, RTD_ACQUIRE_SPI_OK},
        {2U, {0x80U, 0x82U}, {0}, RTD_ACQUIRE_SPI_OK},
        {2U, {0x80U, 0x84U}, {0}, RTD_ACQUIRE_SPI_OK},
        {2U, {0x80U, 0xA0U}, {0}, RTD_ACQUIRE_SPI_OK},
        {3U, {0x01U, 0x00U, 0x00U}, {0x00U, 0x40U, 0x00U}, RTD_ACQUIRE_SPI_OK},
        {2U, {0x80U, 0x00U}, {0}, RTD_ACQUIRE_SPI_OK},
    };
    const delay_event_t delays[] = {
        {11500U, RTD_ACQUIRE_DELAY_OK},
        {600U, RTD_ACQUIRE_DELAY_OK},
        {6000U, RTD_ACQUIRE_DELAY_OK},
        {55000U, RTD_ACQUIRE_DELAY_OK},
        {11500U, RTD_ACQUIRE_DELAY_OK},
        {600U, RTD_ACQUIRE_DELAY_OK},
        {6000U, RTD_ACQUIRE_DELAY_OK},
        {55000U, RTD_ACQUIRE_DELAY_OK},
    };
    fake_spi_context_t spi_context = {transfers, 13U, 0U};
    fake_delay_context_t delay_context = {delays, 8U, 0U};
    rtd_acquire_spi_t spi = make_spi(&spi_context);
    rtd_acquire_delay_t delay = make_delay(&delay_context);
    rtd_acquire_max31865_config_t config = make_config();
    rtd_acquire_max31865_timing_t timing = make_timing();
    rtd_acquire_diagnostic_t diagnostics[1];
    rtd_acquire_native_evidence_t evidence[1];
    rtd_acquire_measurement_t measurement = make_measurement(
        diagnostics,
        1U,
        evidence,
        1U
    );

    assert(
        rtd_acquire_max31865_read(
            &spi, &delay, &config, &timing, &measurement
        ) == RTD_ACQUIRE_MAX31865_RESULT_OK
    );
    assert(!measurement.has_resistance);
    assert(measurement.diagnostic_count == 1U);
    assert(measurement.native_evidence_count == 1U);
    assert(
        rtd_acquire_measurement_status(&measurement)
        == RTD_ACQUIRE_MEASUREMENT_STATUS_FAULT
    );

    assert(
        rtd_acquire_max31865_read(
            &spi, &delay, &config, &timing, &measurement
        ) == RTD_ACQUIRE_MAX31865_RESULT_OK
    );
    assert(measurement.has_resistance);
    assert(measurement.resistance_ohms == 107.5F);
    assert(measurement.diagnostic_count == 0U);
    assert(measurement.native_evidence_count == 0U);
    assert(
        rtd_acquire_measurement_status(&measurement)
        == RTD_ACQUIRE_MEASUREMENT_STATUS_OK
    );
    assert_fake_consumed(&spi_context, &delay_context);
}

static void test_actual_fault_can_exceed_caller_storage_after_io(void)
{
    const transfer_event_t transfers[] = {
        {5U, {0x83U, 0xFFU, 0xFFU, 0x00U, 0x00U}, {0}, RTD_ACQUIRE_SPI_OK},
        {2U, {0x80U, 0x82U}, {0}, RTD_ACQUIRE_SPI_OK},
        {2U, {0x80U, 0x84U}, {0}, RTD_ACQUIRE_SPI_OK},
        {2U, {0x80U, 0xA0U}, {0}, RTD_ACQUIRE_SPI_OK},
        {3U, {0x01U, 0x00U, 0x00U}, {0x00U, 0x40U, 0x01U}, RTD_ACQUIRE_SPI_OK},
        {2U, {0x07U, 0x00U}, {0x00U, 0x80U}, RTD_ACQUIRE_SPI_OK},
        {2U, {0x80U, 0x00U}, {0}, RTD_ACQUIRE_SPI_OK},
    };
    const delay_event_t delays[] = {
        {11500U, RTD_ACQUIRE_DELAY_OK},
        {600U, RTD_ACQUIRE_DELAY_OK},
        {6000U, RTD_ACQUIRE_DELAY_OK},
        {55000U, RTD_ACQUIRE_DELAY_OK},
    };
    fake_spi_context_t spi_context = {transfers, 7U, 0U};
    fake_delay_context_t delay_context = {delays, 4U, 0U};
    rtd_acquire_spi_t spi = make_spi(&spi_context);
    rtd_acquire_delay_t delay = make_delay(&delay_context);
    rtd_acquire_max31865_config_t config = make_config();
    rtd_acquire_max31865_timing_t timing = make_timing();
    rtd_acquire_measurement_t measurement = make_measurement(NULL, 0U, NULL, 0U);

    measurement.has_resistance = true;
    measurement.resistance_ohms = 12.5F;

    assert(
        rtd_acquire_max31865_read(
            &spi,
            &delay,
            &config,
            &timing,
            &measurement
        ) == RTD_ACQUIRE_MAX31865_RESULT_INSUFFICIENT_STORAGE
    );
    assert(measurement.has_resistance);
    assert(measurement.resistance_ohms == 12.5F);
    assert_fake_consumed(&spi_context, &delay_context);
}

static void test_unrepresentable_timing_is_configuration_error_before_io(void)
{
    fake_spi_context_t spi_context = {NULL, 0U, 0U};
    fake_delay_context_t delay_context = {NULL, 0U, 0U};
    rtd_acquire_spi_t spi = make_spi(&spi_context);
    rtd_acquire_delay_t delay = make_delay(&delay_context);
    rtd_acquire_max31865_config_t config = make_config();
    rtd_acquire_max31865_timing_t timing = {UINT32_MAX};
    rtd_acquire_measurement_t measurement = make_measurement(NULL, 0U, NULL, 0U);

    assert(
        rtd_acquire_max31865_read(
            &spi,
            &delay,
            &config,
            &timing,
            &measurement
        ) == RTD_ACQUIRE_MAX31865_RESULT_CONFIGURATION_ERROR
    );
    assert_fake_consumed(&spi_context, &delay_context);
}

static void test_null_capabilities_are_invalid_arguments(void)
{
    fake_spi_context_t spi_context = {NULL, 0U, 0U};
    fake_delay_context_t delay_context = {NULL, 0U, 0U};
    rtd_acquire_spi_t spi = make_spi(&spi_context);
    rtd_acquire_delay_t delay = make_delay(&delay_context);
    rtd_acquire_max31865_config_t config = make_config();
    rtd_acquire_max31865_timing_t timing = make_timing();
    rtd_acquire_measurement_t measurement = make_measurement(NULL, 0U, NULL, 0U);

    assert(
        rtd_acquire_max31865_read(
            NULL,
            &delay,
            &config,
            &timing,
            &measurement
        ) == RTD_ACQUIRE_MAX31865_RESULT_INVALID_ARGUMENT
    );
    assert(
        rtd_acquire_max31865_read(
            &spi,
            NULL,
            &config,
            &timing,
            &measurement
        ) == RTD_ACQUIRE_MAX31865_RESULT_INVALID_ARGUMENT
    );

    spi.transfer = NULL;
    assert(
        rtd_acquire_max31865_read(
            &spi,
            &delay,
            &config,
            &timing,
            &measurement
        ) == RTD_ACQUIRE_MAX31865_RESULT_INVALID_ARGUMENT
    );
}

int main(void)
{
    test_successful_one_shot_sequence();
    test_fault_flag_reads_status_and_ignores_reserved_bits();
    test_three_wire_50_hz_sequence_and_timing();
    test_spi_settings_are_validated_before_io();
    test_delay_failure_preserves_error_and_attempts_bias_off();
    test_fault_cycle_delay_failure_attempts_bias_off();
    test_post_fault_delay_failure_attempts_bias_off();
    test_conversion_delay_failure_attempts_bias_off();
    test_threshold_write_failure_returns_without_bias_cleanup();
    test_spi_failure_after_bias_attempts_bias_off();
    test_failed_initial_bias_write_still_attempts_bias_off();
    test_one_shot_write_failure_attempts_bias_off();
    test_rtd_read_failure_attempts_bias_off();
    test_fault_status_read_failure_attempts_bias_off();
    test_final_bias_off_failure_leaves_measurement_untouched();
    test_repeated_reads_replace_measurement_state();
    test_actual_fault_can_exceed_caller_storage_after_io();
    test_unrepresentable_timing_is_configuration_error_before_io();
    test_null_capabilities_are_invalid_arguments();
    return 0;
}
