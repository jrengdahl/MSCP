#include <stdint.h>
#include <stdio.h>
#include "main.h"
#include "stm32h7xx_hal.h"
#include "adc.h"



// This function reads the temperature sensor and returns the temperature in degrees Celsius.
int read_temperature()
    {
    // Start a conversion.
    HAL_ADC_Start(&hadc3);

    HAL_ADC_PollForConversion(&hadc3, HAL_MAX_DELAY);
    unsigned TS_DATA = HAL_ADC_GetValue(&hadc3);
    HAL_ADC_Stop(&hadc3);

    // Convert the raw temperature value to degrees Celsius.
    int temperature = __LL_ADC_CALC_TEMPERATURE(2500, TS_DATA, LL_ADC_RESOLUTION_12B);

    return temperature;
    }


