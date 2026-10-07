
#include "ntc.h"

#include <math.h>

HAL_StatusTypeDef ntc_init(ADC_HandleTypeDef *hadc)
{
    if (hadc == NULL) return HAL_ERROR;

    return HAL_ADCEx_Calibration_Start(hadc, ADC_CALIB_OFFSET, ADC_SINGLE_ENDED);
}

HAL_StatusTypeDef ntc_read_raw(ADC_HandleTypeDef *hadc, uint32_t *raw)
{
    ADC_ChannelConfTypeDef sConfig = {0};
    uint32_t sum = 0U;
    uint32_t i;

    if (hadc == NULL || raw == NULL) return HAL_ERROR;

    sConfig.Channel      = NTC_ADC_CHANNEL;
    sConfig.Rank         = ADC_REGULAR_RANK_1;
    sConfig.SamplingTime = NTC_SAMPLING_TIME;
    sConfig.SingleDiff   = ADC_SINGLE_ENDED;
    sConfig.OffsetNumber = ADC_OFFSET_NONE;
    sConfig.Offset       = 0;
    if (HAL_ADC_ConfigChannel(hadc, &sConfig) != HAL_OK) return HAL_ERROR;

    for (i = 0U; i < NTC_AVG_SAMPLES; i++)
    {
        if (HAL_ADC_Start(hadc) != HAL_OK) return HAL_ERROR;

        if (HAL_ADC_PollForConversion(hadc, NTC_ADC_TIMEOUT_MS) != HAL_OK)
        {
            (void)HAL_ADC_Stop(hadc);
            return HAL_TIMEOUT;
        }
        sum += HAL_ADC_GetValue(hadc);
        (void)HAL_ADC_Stop(hadc);
    }

    *raw = sum / NTC_AVG_SAMPLES;
    return HAL_OK;
}

HAL_StatusTypeDef ntc_read_res(ADC_HandleTypeDef *hadc, float *ohm)
{
    uint32_t raw;

    if (ohm == NULL) return HAL_ERROR;
    if (ntc_read_raw(hadc, &raw) != HAL_OK) return HAL_ERROR;

    /* raw = 0 -> NTC chap xuong mass; raw = FULL -> NTC ho mach */
    if ((raw == 0U) || (raw >= (uint32_t)NTC_ADC_FULL)) return HAL_ERROR;

    *ohm = NTC_R_SERIES * ((float)raw / (NTC_ADC_FULL - (float)raw));
    return HAL_OK;
}

HAL_StatusTypeDef ntc_read_temp(ADC_HandleTypeDef *hadc, float *temp_c)
{
    float ohm;
    float inv_t;

    if (temp_c == NULL) return HAL_ERROR;
    if (ntc_read_res(hadc, &ohm) != HAL_OK) return HAL_ERROR;

    /* Phuong trinh Beta: 1/T = 1/T25 + ln(R/R25)/B  (T tinh bang Kelvin) */
    inv_t   = (1.0f / NTC_T25_K) + (logf(ohm / NTC_R25) / NTC_BETA);
    *temp_c = (1.0f / inv_t) - 273.15f;

    return HAL_OK;
}

HAL_StatusTypeDef ntc_read_mc(ADC_HandleTypeDef *hadc, int32_t *mc)
{
    float t;

    if (mc == NULL) return HAL_ERROR;
    if (ntc_read_temp(hadc, &t) != HAL_OK) return HAL_ERROR;

    *mc = (int32_t)(t * 1000.0f);
    return HAL_OK;
}
