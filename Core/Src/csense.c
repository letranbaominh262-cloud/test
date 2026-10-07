
#include "csense.h"

/* Chuyen rank 1 sang kenh can doc. Phai lam moi lan vi ADC1 dung chung voi NTC. */
static HAL_StatusTypeDef csense_select(ADC_HandleTypeDef *hadc, uint32_t channel)
{
    ADC_ChannelConfTypeDef sConfig = {0};

    sConfig.Channel      = channel;
    sConfig.Rank         = ADC_REGULAR_RANK_1;
    sConfig.SamplingTime = ADC_SAMPLETIME_64CYCLES_5;   /* ngo ra INA la op-amp, tro khang thap */
    sConfig.SingleDiff   = ADC_SINGLE_ENDED;
    sConfig.OffsetNumber = ADC_OFFSET_NONE;
    sConfig.Offset       = 0;

    return HAL_ADC_ConfigChannel(hadc, &sConfig);
}

HAL_StatusTypeDef csense_read_raw(ADC_HandleTypeDef *hadc, uint32_t channel, uint32_t *raw)
{
    uint32_t sum = 0U;
    uint32_t i;

    if (hadc == NULL || raw == NULL) return HAL_ERROR;
    if (csense_select(hadc, channel) != HAL_OK) return HAL_ERROR;

    for (i = 0U; i < CSENSE_AVG_SAMPLES; i++)
    {
        if (HAL_ADC_Start(hadc) != HAL_OK) return HAL_ERROR;

        if (HAL_ADC_PollForConversion(hadc, CSENSE_ADC_TIMEOUT_MS) != HAL_OK)
        {
            (void)HAL_ADC_Stop(hadc);
            return HAL_TIMEOUT;
        }
        sum += HAL_ADC_GetValue(hadc);
        (void)HAL_ADC_Stop(hadc);
    }

    *raw = sum / CSENSE_AVG_SAMPLES;
    return HAL_OK;
}

HAL_StatusTypeDef csense_calibrate(ADC_HandleTypeDef *hadc, uint32_t channel, uint32_t *zero_raw)
{
    return csense_read_raw(hadc, channel, zero_raw);
}

HAL_StatusTypeDef csense_read_volt(ADC_HandleTypeDef *hadc, uint32_t channel, float *volt)
{
    uint32_t raw;

    if (volt == NULL) return HAL_ERROR;
    if (csense_read_raw(hadc, channel, &raw) != HAL_OK) return HAL_ERROR;

    *volt = ((float)raw * CSENSE_VREF_ADC) / CSENSE_ADC_FULL;
    return HAL_OK;
}

HAL_StatusTypeDef csense_read_ma(ADC_HandleTypeDef *hadc, uint32_t channel,
                                 uint32_t zero_raw, int32_t *ma)
{
    uint32_t raw;
    float    v_zero;
    float    volt;

    if (ma == NULL) return HAL_ERROR;
    if (csense_read_raw(hadc, channel, &raw) != HAL_OK) return HAL_ERROR;

    volt   = ((float)raw * CSENSE_VREF_ADC) / CSENSE_ADC_FULL;
    v_zero = (zero_raw == 0U) ? CSENSE_VREF_MID
                              : (((float)zero_raw * CSENSE_VREF_ADC) / CSENSE_ADC_FULL);

    /* I = (Vout - Vzero) / (Rshunt x Gain).  Voi 0.02 x 50 = 1.0 -> 1 V ung voi 1 A */
    *ma = (int32_t)(((volt - v_zero) / (CSENSE_SHUNT_OHM * CSENSE_GAIN)) * 1000.0f);

    return HAL_OK;
}
