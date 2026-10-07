
#ifndef NTC_H
#define NTC_H

#include "main.h"

#define NTC_R_SERIES        300000.0f   /* R95 */
#define NTC_R25             100000.0f   /* R cua NTC tai 25 do C */
#define NTC_BETA            4250.0f     /* B(25/50), datasheet Murata */
#define NTC_T25_K           298.15f
#define NTC_ADC_FULL        65535.0f    /* ADC 16 bit */

#define NTC_AVG_SAMPLES     16U         /* so lan do lay trung binh */
#define NTC_ADC_TIMEOUT_MS  10U

#define NTC_ADC_CHANNEL     ADC_CHANNEL_9
#define NTC_SAMPLING_TIME   ADC_SAMPLETIME_810CYCLES_5

/* Goi 1 lan truoc khi doc: hieu chuan offset cua ADC */
HAL_StatusTypeDef ntc_init(ADC_HandleTypeDef *hadc);

HAL_StatusTypeDef ntc_read_raw(ADC_HandleTypeDef *hadc, uint32_t *raw);        /* gia tri ADC tho */
HAL_StatusTypeDef ntc_read_res(ADC_HandleTypeDef *hadc, float *ohm);           /* dien tro NTC */
HAL_StatusTypeDef ntc_read_temp(ADC_HandleTypeDef *hadc, float *temp_c);       /* do C */
HAL_StatusTypeDef ntc_read_mc(ADC_HandleTypeDef *hadc, int32_t *mc);           /* milli do C */

#endif /* NTC_H */
