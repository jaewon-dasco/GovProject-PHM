################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (13.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../OneLibrary/Src/ONE_ATCommend.c \
../OneLibrary/Src/ONE_Common.c \
../OneLibrary/Src/ONE_FFT.c \
../OneLibrary/Src/ONE_Filter.c \
../OneLibrary/Src/ONE_Math.c \
../OneLibrary/Src/ONE_Memory.c \
../OneLibrary/Src/ONE_Serial.c \
../OneLibrary/Src/ONE_Signal.c \
../OneLibrary/Src/ONE_Time.c 

OBJS += \
./OneLibrary/Src/ONE_ATCommend.o \
./OneLibrary/Src/ONE_Common.o \
./OneLibrary/Src/ONE_FFT.o \
./OneLibrary/Src/ONE_Filter.o \
./OneLibrary/Src/ONE_Math.o \
./OneLibrary/Src/ONE_Memory.o \
./OneLibrary/Src/ONE_Serial.o \
./OneLibrary/Src/ONE_Signal.o \
./OneLibrary/Src/ONE_Time.o 

C_DEPS += \
./OneLibrary/Src/ONE_ATCommend.d \
./OneLibrary/Src/ONE_Common.d \
./OneLibrary/Src/ONE_FFT.d \
./OneLibrary/Src/ONE_Filter.d \
./OneLibrary/Src/ONE_Math.d \
./OneLibrary/Src/ONE_Memory.d \
./OneLibrary/Src/ONE_Serial.d \
./OneLibrary/Src/ONE_Signal.d \
./OneLibrary/Src/ONE_Time.d 


# Each subdirectory must supply rules for building sources it contributes
OneLibrary/Src/%.o OneLibrary/Src/%.su OneLibrary/Src/%.cyclo: ../OneLibrary/Src/%.c OneLibrary/Src/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m33 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32U575xx -DARM_MATH_CM33 -c -I../Core/Inc -I"D:/1_Embedded Project/0_IoT/98_PHM/WORK/OneLibrary/Inc" -I"D:/1_Embedded Project/0_IoT/98_PHM/WORK/oThirdParty/Inc" -I../Drivers/STM32U5xx_HAL_Driver/Inc -I../Drivers/STM32U5xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32U5xx/Include -I../Drivers/CMSIS/Include -I../Drivers/CMSIS/DSP/Include -I../Drivers/CMSIS/DSP/PrivateInclude -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv5-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-OneLibrary-2f-Src

clean-OneLibrary-2f-Src:
	-$(RM) ./OneLibrary/Src/ONE_ATCommend.cyclo ./OneLibrary/Src/ONE_ATCommend.d ./OneLibrary/Src/ONE_ATCommend.o ./OneLibrary/Src/ONE_ATCommend.su ./OneLibrary/Src/ONE_Common.cyclo ./OneLibrary/Src/ONE_Common.d ./OneLibrary/Src/ONE_Common.o ./OneLibrary/Src/ONE_Common.su ./OneLibrary/Src/ONE_FFT.cyclo ./OneLibrary/Src/ONE_FFT.d ./OneLibrary/Src/ONE_FFT.o ./OneLibrary/Src/ONE_FFT.su ./OneLibrary/Src/ONE_Filter.cyclo ./OneLibrary/Src/ONE_Filter.d ./OneLibrary/Src/ONE_Filter.o ./OneLibrary/Src/ONE_Filter.su ./OneLibrary/Src/ONE_Math.cyclo ./OneLibrary/Src/ONE_Math.d ./OneLibrary/Src/ONE_Math.o ./OneLibrary/Src/ONE_Math.su ./OneLibrary/Src/ONE_Memory.cyclo ./OneLibrary/Src/ONE_Memory.d ./OneLibrary/Src/ONE_Memory.o ./OneLibrary/Src/ONE_Memory.su ./OneLibrary/Src/ONE_Serial.cyclo ./OneLibrary/Src/ONE_Serial.d ./OneLibrary/Src/ONE_Serial.o ./OneLibrary/Src/ONE_Serial.su ./OneLibrary/Src/ONE_Signal.cyclo ./OneLibrary/Src/ONE_Signal.d ./OneLibrary/Src/ONE_Signal.o ./OneLibrary/Src/ONE_Signal.su ./OneLibrary/Src/ONE_Time.cyclo ./OneLibrary/Src/ONE_Time.d ./OneLibrary/Src/ONE_Time.o ./OneLibrary/Src/ONE_Time.su

.PHONY: clean-OneLibrary-2f-Src

