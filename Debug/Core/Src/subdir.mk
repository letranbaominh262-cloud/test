################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (13.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Core/Src/app_tasks.c \
../Core/Src/csense.c \
../Core/Src/drv8908.c \
../Core/Src/flash.c \
../Core/Src/fram.c \
../Core/Src/freertos.c \
../Core/Src/main.c \
../Core/Src/mlx90393.c \
../Core/Src/mlx90614.c \
../Core/Src/ntc.c \
../Core/Src/stm32h7xx_hal_msp.c \
../Core/Src/stm32h7xx_hal_timebase_tim.c \
../Core/Src/stm32h7xx_it.c \
../Core/Src/syscalls.c \
../Core/Src/sysmem.c \
../Core/Src/system_stm32h7xx.c \
../Core/Src/tca9548a.c \
../Core/Src/veml6031.c 

OBJS += \
./Core/Src/app_tasks.o \
./Core/Src/csense.o \
./Core/Src/drv8908.o \
./Core/Src/flash.o \
./Core/Src/fram.o \
./Core/Src/freertos.o \
./Core/Src/main.o \
./Core/Src/mlx90393.o \
./Core/Src/mlx90614.o \
./Core/Src/ntc.o \
./Core/Src/stm32h7xx_hal_msp.o \
./Core/Src/stm32h7xx_hal_timebase_tim.o \
./Core/Src/stm32h7xx_it.o \
./Core/Src/syscalls.o \
./Core/Src/sysmem.o \
./Core/Src/system_stm32h7xx.o \
./Core/Src/tca9548a.o \
./Core/Src/veml6031.o 

C_DEPS += \
./Core/Src/app_tasks.d \
./Core/Src/csense.d \
./Core/Src/drv8908.d \
./Core/Src/flash.d \
./Core/Src/fram.d \
./Core/Src/freertos.d \
./Core/Src/main.d \
./Core/Src/mlx90393.d \
./Core/Src/mlx90614.d \
./Core/Src/ntc.d \
./Core/Src/stm32h7xx_hal_msp.d \
./Core/Src/stm32h7xx_hal_timebase_tim.d \
./Core/Src/stm32h7xx_it.d \
./Core/Src/syscalls.d \
./Core/Src/sysmem.d \
./Core/Src/system_stm32h7xx.d \
./Core/Src/tca9548a.d \
./Core/Src/veml6031.d 


# Each subdirectory must supply rules for building sources it contributes
Core/Src/%.o Core/Src/%.su Core/Src/%.cyclo: ../Core/Src/%.c Core/Src/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m7 -std=gnu11 -g3 -DDEBUG -DUSE_PWR_LDO_SUPPLY -DUSE_HAL_DRIVER -DSTM32H743xx -c -I../Core/Inc -I../Drivers/STM32H7xx_HAL_Driver/Inc -I../Drivers/STM32H7xx_HAL_Driver/Inc/Legacy -I../Middlewares/Third_Party/FreeRTOS/Source/include -I../Middlewares/Third_Party/FreeRTOS/Source/CMSIS_RTOS_V2 -I../Middlewares/Third_Party/FreeRTOS/Source/portable/GCC/ARM_CM4F -I../Drivers/CMSIS/Device/ST/STM32H7xx/Include -I../Drivers/CMSIS/Include -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv5-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-Core-2f-Src

clean-Core-2f-Src:
	-$(RM) ./Core/Src/app_tasks.cyclo ./Core/Src/app_tasks.d ./Core/Src/app_tasks.o ./Core/Src/app_tasks.su ./Core/Src/csense.cyclo ./Core/Src/csense.d ./Core/Src/csense.o ./Core/Src/csense.su ./Core/Src/drv8908.cyclo ./Core/Src/drv8908.d ./Core/Src/drv8908.o ./Core/Src/drv8908.su ./Core/Src/flash.cyclo ./Core/Src/flash.d ./Core/Src/flash.o ./Core/Src/flash.su ./Core/Src/fram.cyclo ./Core/Src/fram.d ./Core/Src/fram.o ./Core/Src/fram.su ./Core/Src/freertos.cyclo ./Core/Src/freertos.d ./Core/Src/freertos.o ./Core/Src/freertos.su ./Core/Src/main.cyclo ./Core/Src/main.d ./Core/Src/main.o ./Core/Src/main.su ./Core/Src/mlx90393.cyclo ./Core/Src/mlx90393.d ./Core/Src/mlx90393.o ./Core/Src/mlx90393.su ./Core/Src/mlx90614.cyclo ./Core/Src/mlx90614.d ./Core/Src/mlx90614.o ./Core/Src/mlx90614.su ./Core/Src/ntc.cyclo ./Core/Src/ntc.d ./Core/Src/ntc.o ./Core/Src/ntc.su ./Core/Src/stm32h7xx_hal_msp.cyclo ./Core/Src/stm32h7xx_hal_msp.d ./Core/Src/stm32h7xx_hal_msp.o ./Core/Src/stm32h7xx_hal_msp.su ./Core/Src/stm32h7xx_hal_timebase_tim.cyclo ./Core/Src/stm32h7xx_hal_timebase_tim.d ./Core/Src/stm32h7xx_hal_timebase_tim.o ./Core/Src/stm32h7xx_hal_timebase_tim.su ./Core/Src/stm32h7xx_it.cyclo ./Core/Src/stm32h7xx_it.d ./Core/Src/stm32h7xx_it.o ./Core/Src/stm32h7xx_it.su ./Core/Src/syscalls.cyclo ./Core/Src/syscalls.d ./Core/Src/syscalls.o ./Core/Src/syscalls.su ./Core/Src/sysmem.cyclo ./Core/Src/sysmem.d ./Core/Src/sysmem.o ./Core/Src/sysmem.su ./Core/Src/system_stm32h7xx.cyclo ./Core/Src/system_stm32h7xx.d ./Core/Src/system_stm32h7xx.o ./Core/Src/system_stm32h7xx.su ./Core/Src/tca9548a.cyclo ./Core/Src/tca9548a.d ./Core/Src/tca9548a.o ./Core/Src/tca9548a.su ./Core/Src/veml6031.cyclo ./Core/Src/veml6031.d ./Core/Src/veml6031.o ./Core/Src/veml6031.su

.PHONY: clean-Core-2f-Src

