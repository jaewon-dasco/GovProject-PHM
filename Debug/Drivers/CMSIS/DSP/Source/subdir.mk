################################################################################
# Manual subdir.mk for CMSIS-DSP — Mi_Measurement.c 의 arm_rfft_fast_f32 용
################################################################################

C_SRCS += \
../Drivers/CMSIS/DSP/Source/CommonTables/arm_common_tables.c \
../Drivers/CMSIS/DSP/Source/CommonTables/arm_const_structs.c \
../Drivers/CMSIS/DSP/Source/ComplexMathFunctions/arm_cmplx_mag_f32.c \
../Drivers/CMSIS/DSP/Source/StatisticsFunctions/arm_max_f32.c \
../Drivers/CMSIS/DSP/Source/TransformFunctions/arm_bitreversal2.c \
../Drivers/CMSIS/DSP/Source/TransformFunctions/arm_cfft_f32.c \
../Drivers/CMSIS/DSP/Source/TransformFunctions/arm_cfft_init_f32.c \
../Drivers/CMSIS/DSP/Source/TransformFunctions/arm_cfft_radix8_f32.c \
../Drivers/CMSIS/DSP/Source/TransformFunctions/arm_rfft_fast_f32.c \
../Drivers/CMSIS/DSP/Source/TransformFunctions/arm_rfft_fast_init_f32.c

OBJS += \
./Drivers/CMSIS/DSP/Source/CommonTables/arm_common_tables.o \
./Drivers/CMSIS/DSP/Source/CommonTables/arm_const_structs.o \
./Drivers/CMSIS/DSP/Source/ComplexMathFunctions/arm_cmplx_mag_f32.o \
./Drivers/CMSIS/DSP/Source/StatisticsFunctions/arm_max_f32.o \
./Drivers/CMSIS/DSP/Source/TransformFunctions/arm_bitreversal2.o \
./Drivers/CMSIS/DSP/Source/TransformFunctions/arm_cfft_f32.o \
./Drivers/CMSIS/DSP/Source/TransformFunctions/arm_cfft_init_f32.o \
./Drivers/CMSIS/DSP/Source/TransformFunctions/arm_cfft_radix8_f32.o \
./Drivers/CMSIS/DSP/Source/TransformFunctions/arm_rfft_fast_f32.o \
./Drivers/CMSIS/DSP/Source/TransformFunctions/arm_rfft_fast_init_f32.o

C_DEPS += \
./Drivers/CMSIS/DSP/Source/CommonTables/arm_common_tables.d \
./Drivers/CMSIS/DSP/Source/CommonTables/arm_const_structs.d \
./Drivers/CMSIS/DSP/Source/ComplexMathFunctions/arm_cmplx_mag_f32.d \
./Drivers/CMSIS/DSP/Source/StatisticsFunctions/arm_max_f32.d \
./Drivers/CMSIS/DSP/Source/TransformFunctions/arm_bitreversal2.d \
./Drivers/CMSIS/DSP/Source/TransformFunctions/arm_cfft_f32.d \
./Drivers/CMSIS/DSP/Source/TransformFunctions/arm_cfft_init_f32.d \
./Drivers/CMSIS/DSP/Source/TransformFunctions/arm_cfft_radix8_f32.d \
./Drivers/CMSIS/DSP/Source/TransformFunctions/arm_rfft_fast_f32.d \
./Drivers/CMSIS/DSP/Source/TransformFunctions/arm_rfft_fast_init_f32.d


# Rule pattern for CMSIS-DSP sources
Drivers/CMSIS/DSP/Source/%.o Drivers/CMSIS/DSP/Source/%.su Drivers/CMSIS/DSP/Source/%.cyclo: ../Drivers/CMSIS/DSP/Source/%.c Drivers/CMSIS/DSP/Source/subdir.mk
	@mkdir -p "$(@D)"
	arm-none-eabi-gcc "$<" -mcpu=cortex-m33 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32U575xx -DARM_MATH_CM33 -c -I../Core/Inc -I../Drivers/CMSIS/DSP/Include -I../Drivers/CMSIS/DSP/PrivateInclude -I../Drivers/STM32U5xx_HAL_Driver/Inc -I../Drivers/STM32U5xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32U5xx/Include -I../Drivers/CMSIS/Include -O2 -ffunction-sections -fdata-sections -Wall -Wno-attributes -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv5-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-Drivers-2f-CMSIS-2f-DSP-2f-Source

clean-Drivers-2f-CMSIS-2f-DSP-2f-Source:
	-$(RM) ./Drivers/CMSIS/DSP/Source/CommonTables/*.o ./Drivers/CMSIS/DSP/Source/CommonTables/*.d ./Drivers/CMSIS/DSP/Source/CommonTables/*.su ./Drivers/CMSIS/DSP/Source/CommonTables/*.cyclo ./Drivers/CMSIS/DSP/Source/ComplexMathFunctions/*.o ./Drivers/CMSIS/DSP/Source/ComplexMathFunctions/*.d ./Drivers/CMSIS/DSP/Source/ComplexMathFunctions/*.su ./Drivers/CMSIS/DSP/Source/ComplexMathFunctions/*.cyclo ./Drivers/CMSIS/DSP/Source/StatisticsFunctions/*.o ./Drivers/CMSIS/DSP/Source/StatisticsFunctions/*.d ./Drivers/CMSIS/DSP/Source/StatisticsFunctions/*.su ./Drivers/CMSIS/DSP/Source/StatisticsFunctions/*.cyclo ./Drivers/CMSIS/DSP/Source/TransformFunctions/*.o ./Drivers/CMSIS/DSP/Source/TransformFunctions/*.d ./Drivers/CMSIS/DSP/Source/TransformFunctions/*.su ./Drivers/CMSIS/DSP/Source/TransformFunctions/*.cyclo

.PHONY: clean-Drivers-2f-CMSIS-2f-DSP-2f-Source
