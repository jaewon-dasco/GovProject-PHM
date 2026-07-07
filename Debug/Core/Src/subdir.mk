################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (13.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Core/Src/Mi_IoT.c \
../Core/Src/Mi_IoT_PHM.c \
../Core/Src/Mi_LoRa.c \
../Core/Src/Mi_LoRa_PHM.c \
../Core/Src/Mi_Main_PHM.c \
../Core/Src/Mi_Measurement.c \
../Core/Src/Mi_Native_U575CGT6.c \
../Core/Src/Mi_Serial.c \
../Core/Src/Mi_Serial_PHM.c \
../Core/Src/Mi_Storage_PHM.c \
../Core/Src/main.c \
../Core/Src/stm32u5xx_hal_msp.c \
../Core/Src/stm32u5xx_it.c \
../Core/Src/syscalls.c \
../Core/Src/sysmem.c \
../Core/Src/system_stm32u5xx.c 

OBJS += \
./Core/Src/Mi_IoT.o \
./Core/Src/Mi_IoT_PHM.o \
./Core/Src/Mi_LoRa.o \
./Core/Src/Mi_LoRa_PHM.o \
./Core/Src/Mi_Main_PHM.o \
./Core/Src/Mi_Measurement.o \
./Core/Src/Mi_Native_U575CGT6.o \
./Core/Src/Mi_Serial.o \
./Core/Src/Mi_Serial_PHM.o \
./Core/Src/Mi_Storage_PHM.o \
./Core/Src/main.o \
./Core/Src/stm32u5xx_hal_msp.o \
./Core/Src/stm32u5xx_it.o \
./Core/Src/syscalls.o \
./Core/Src/sysmem.o \
./Core/Src/system_stm32u5xx.o 

C_DEPS += \
./Core/Src/Mi_IoT.d \
./Core/Src/Mi_IoT_PHM.d \
./Core/Src/Mi_LoRa.d \
./Core/Src/Mi_LoRa_PHM.d \
./Core/Src/Mi_Main_PHM.d \
./Core/Src/Mi_Measurement.d \
./Core/Src/Mi_Native_U575CGT6.d \
./Core/Src/Mi_Serial.d \
./Core/Src/Mi_Serial_PHM.d \
./Core/Src/Mi_Storage_PHM.d \
./Core/Src/main.d \
./Core/Src/stm32u5xx_hal_msp.d \
./Core/Src/stm32u5xx_it.d \
./Core/Src/syscalls.d \
./Core/Src/sysmem.d \
./Core/Src/system_stm32u5xx.d 


# Each subdirectory must supply rules for building sources it contributes
Core/Src/%.o Core/Src/%.su Core/Src/%.cyclo: ../Core/Src/%.c Core/Src/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m33 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32U575xx -DARM_MATH_CM33 -c -I../Core/Inc -I"D:/1_Embedded Project/0_IoT/98_PHM/WORK/OneLibrary/Inc" -I"D:/1_Embedded Project/0_IoT/98_PHM/WORK/oThirdParty/Inc" -I../Drivers/STM32U5xx_HAL_Driver/Inc -I../Drivers/STM32U5xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32U5xx/Include -I../Drivers/CMSIS/Include -I../Drivers/CMSIS/DSP/Include -I../Drivers/CMSIS/DSP/PrivateInclude -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv5-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-Core-2f-Src

clean-Core-2f-Src:
	-$(RM) ./Core/Src/Mi_IoT.cyclo ./Core/Src/Mi_IoT.d ./Core/Src/Mi_IoT.o ./Core/Src/Mi_IoT.su ./Core/Src/Mi_IoT_PHM.cyclo ./Core/Src/Mi_IoT_PHM.d ./Core/Src/Mi_IoT_PHM.o ./Core/Src/Mi_IoT_PHM.su ./Core/Src/Mi_LoRa.cyclo ./Core/Src/Mi_LoRa.d ./Core/Src/Mi_LoRa.o ./Core/Src/Mi_LoRa.su ./Core/Src/Mi_LoRa_PHM.cyclo ./Core/Src/Mi_LoRa_PHM.d ./Core/Src/Mi_LoRa_PHM.o ./Core/Src/Mi_LoRa_PHM.su ./Core/Src/Mi_Main_PHM.cyclo ./Core/Src/Mi_Main_PHM.d ./Core/Src/Mi_Main_PHM.o ./Core/Src/Mi_Main_PHM.su ./Core/Src/Mi_Measurement.cyclo ./Core/Src/Mi_Measurement.d ./Core/Src/Mi_Measurement.o ./Core/Src/Mi_Measurement.su ./Core/Src/Mi_Native_U575CGT6.cyclo ./Core/Src/Mi_Native_U575CGT6.d ./Core/Src/Mi_Native_U575CGT6.o ./Core/Src/Mi_Native_U575CGT6.su ./Core/Src/Mi_Serial.cyclo ./Core/Src/Mi_Serial.d ./Core/Src/Mi_Serial.o ./Core/Src/Mi_Serial.su ./Core/Src/Mi_Serial_PHM.cyclo ./Core/Src/Mi_Serial_PHM.d ./Core/Src/Mi_Serial_PHM.o ./Core/Src/Mi_Serial_PHM.su ./Core/Src/Mi_Storage_PHM.cyclo ./Core/Src/Mi_Storage_PHM.d ./Core/Src/Mi_Storage_PHM.o ./Core/Src/Mi_Storage_PHM.su ./Core/Src/main.cyclo ./Core/Src/main.d ./Core/Src/main.o ./Core/Src/main.su ./Core/Src/stm32u5xx_hal_msp.cyclo ./Core/Src/stm32u5xx_hal_msp.d ./Core/Src/stm32u5xx_hal_msp.o ./Core/Src/stm32u5xx_hal_msp.su ./Core/Src/stm32u5xx_it.cyclo ./Core/Src/stm32u5xx_it.d ./Core/Src/stm32u5xx_it.o ./Core/Src/stm32u5xx_it.su ./Core/Src/syscalls.cyclo ./Core/Src/syscalls.d ./Core/Src/syscalls.o ./Core/Src/syscalls.su ./Core/Src/sysmem.cyclo ./Core/Src/sysmem.d ./Core/Src/sysmem.o ./Core/Src/sysmem.su ./Core/Src/system_stm32u5xx.cyclo ./Core/Src/system_stm32u5xx.d ./Core/Src/system_stm32u5xx.o ./Core/Src/system_stm32u5xx.su

.PHONY: clean-Core-2f-Src

