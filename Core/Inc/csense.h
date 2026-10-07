
#ifndef CSENSE_H
#define CSENSE_H

#include "main.h"

/* Do dong qua 3 cuon magnetorquer (sheet Half-Bridge)
 *   Shunt 0.02 ohm noi tiep moi cuon -> INA187A2 (gain 50 V/V)
 *   Chan REF cua INA noi +1V65_REF (U12 = REF3016, 1.65 V)
 *   => Vout = 1.65 V + I x 0.02 x 50 = 1.65 V + I x 1.0
 *      Dong 0 A  -> 1.65 V  -> ADC ~32768
 *      Dong +1 A -> 2.65 V,  dong -1 A -> 0.65 V
 *      Do duoc ca 2 chieu, dai +-1.65 A, moi bit ADC = 50 uA
 *
 *   Kenh ADC (theo Top Design):
 *     I_COIL_1 -> PC0  = ADC1_INP10
 *     I_COIL_2 -> PC1  = ADC1_INP11
 *     I_COIL_3 -> PC2_C = ADC3_INP0
 *
 *   Luu y: ADC1 dung chung voi NTC (kenh 9). Thu vien nay LUON cau hinh lai
 *   kenh truoc moi lan doc, nen ntc.c cung phai lam vay (xem ghi chu trong README).
 */

#define CSENSE_CH_COIL1         ADC_CHANNEL_10   /* PC0,   tren ADC1 */
#define CSENSE_CH_COIL2         ADC_CHANNEL_11   /* PC1,   tren ADC1 */
#define CSENSE_CH_COIL3         ADC_CHANNEL_0    /* PC2_C, tren ADC3 */

#define CSENSE_SHUNT_OHM        0.02f
#define CSENSE_GAIN             50.0f            /* INA187A2 */
#define CSENSE_VREF_MID         1.65f            /* REF3016 */
#define CSENSE_VREF_ADC         3.3f
#define CSENSE_ADC_FULL         65535.0f

#define CSENSE_AVG_SAMPLES      16U
#define CSENSE_ADC_TIMEOUT_MS   10U

/* Doc gia tri ADC tho (trung binh CSENSE_AVG_SAMPLES lan) */
HAL_StatusTypeDef csense_read_raw(ADC_HandleTypeDef *hadc, uint32_t channel, uint32_t *raw);

/* Do diem 0: goi khi cuon day DANG TAT, luu lai gia tri de bu troi offset */
HAL_StatusTypeDef csense_calibrate(ADC_HandleTypeDef *hadc, uint32_t channel, uint32_t *zero_raw);

/* Dong tinh bang mA, co dau. zero_raw lay tu csense_calibrate,
 * truyen 0 thi dung diem 0 ly thuyet (1.65 V) */
HAL_StatusTypeDef csense_read_ma(ADC_HandleTypeDef *hadc, uint32_t channel,
                                 uint32_t zero_raw, int32_t *ma);

/* Dien ap tai chan ADC, de soi khi go loi */
HAL_StatusTypeDef csense_read_volt(ADC_HandleTypeDef *hadc, uint32_t channel, float *volt);

#endif /* CSENSE_H */
