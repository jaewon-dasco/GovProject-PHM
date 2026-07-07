################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (13.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../oThirdParty/Src/ADC_NAU7802.c \
../oThirdParty/Src/EEPROM_24LC.c \
../oThirdParty/Src/LoRa_RAK3172.c \
../oThirdParty/Src/MEMS_ISM330DHCXTR.c \
../oThirdParty/Src/NAND_MT29F2G01ABAGDWB_IT.c 

OBJS += \
./oThirdParty/Src/ADC_NAU7802.o \
./oThirdParty/Src/EEPROM_24LC.o \
./oThirdParty/Src/LoRa_RAK3172.o \
./oThirdParty/Src/MEMS_ISM330DHCXTR.o \
./oThirdParty/Src/NAND_MT29F2G01ABAGDWB_IT.o 

C_DEPS += \
./oThirdParty/Src/ADC_NAU7802.d \
./oThirdParty/Src/EEPROM_24LC.d \
./oThirdParty/Src/LoRa_RAK3172.d \
./oThirdParty/Src/MEMS_ISM330DHCXTR.d \
./oThirdParty/Src/NAND_MT29F2G01ABAGDWB_IT.d 


# Each subdirectory must supply rules for building sources it contributes
oThirdParty/Src/%.o oThirdParty/Src/%.su oThirdParty/Src/%.cyclo: ../oThirdParty/Src/%.c oThirdParty/Src/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m33 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32U575xx -DARM_MATH_CM33 -c -I../Core/Inc -I"D:/1_Embedded Project/0_IoT/98_PHM/WORK/OneLibrary/Inc" -I"D:/1_Embedded Project/0_IoT/98_PHM/WORK/oThirdParty/Inc" -I../Drivers/STM32U5xx_HAL_Driver/Inc -I../Drivers/STM32U5xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32U5xx/Include -I../Drivers/CMSIS/Include -I../Drivers/CMSIS/DSP/Include -I../Drivers/CMSIS/DSP/PrivateInclude -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv5-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-oThirdParty-2f-Src

clean-oThirdParty-2f-Src:
	-$(RM) ./oThirdParty/Src/ADC_NAU7802.cyclo ./oThirdParty/Src/ADC_NAU7802.d ./oThirdParty/Src/ADC_NAU7802.o ./oThirdParty/Src/ADC_NAU7802.su ./oThirdParty/Src/EEPROM_24LC.cyclo ./oThirdParty/Src/EEPROM_24LC.d ./oThirdParty/Src/EEPROM_24LC.o ./oThirdParty/Src/EEPROM_24LC.su ./oThirdParty/Src/LoRa_RAK3172.cyclo ./oThirdParty/Src/LoRa_RAK3172.d ./oThirdParty/Src/LoRa_RAK3172.o ./oThirdParty/Src/LoRa_RAK3172.su ./oThirdParty/Src/MEMS_ISM330DHCXTR.cyclo ./oThirdParty/Src/MEMS_ISM330DHCXTR.d ./oThirdParty/Src/MEMS_ISM330DHCXTR.o ./oThirdParty/Src/MEMS_ISM330DHCXTR.su ./oThirdParty/Src/NAND_MT29F2G01ABAGDWB_IT.cyclo ./oThirdParty/Src/NAND_MT29F2G01ABAGDWB_IT.d ./oThirdParty/Src/NAND_MT29F2G01ABAGDWB_IT.o ./oThirdParty/Src/NAND_MT29F2G01ABAGDWB_IT.su

.PHONY: clean-oThirdParty-2f-Src

