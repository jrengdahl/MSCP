#include <stdint.h>
#include <stdio.h>
#include "main.h"
#include "stm32h5xx_hal.h"
#include "adc.h"

#define TS_CAL1 0x02fa
#define TS_CAL2 0x03f6
#define TS_CAL1_TEMP 30
#define TS_CAL2_TEMP 130

// This function reads the temperature sensor and returns the temperature in degrees Celsius.
int read_temperature()
    {
    // Start a conversion.
    HAL_ADC_Start(&hadc1);

    HAL_ADC_PollForConversion(&hadc1, 1000);

    int TS_DATA = HAL_ADC_GetValue(&hadc1);

    // Convert the raw temperature value to degrees Celsius.
    int temperature = (TS_CAL2_TEMP-TS_CAL1_TEMP) * (TS_DATA-TS_CAL1) / (TS_CAL2-TS_CAL1) + TS_CAL1_TEMP;

    return temperature;
    }


