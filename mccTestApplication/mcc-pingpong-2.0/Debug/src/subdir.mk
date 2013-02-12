################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../src/pingpong.c 

OBJS += \
./src/pingpong.o 

C_DEPS += \
./src/pingpong.d 


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	$(CC) -I/media/Data/Timesys/Faraday/MCC/mccReleaseFinal/build_armv7l-timesys-linux-gnueabi/mcc-kmod-1.0/mcc-kmod-1.0 -I/media/Data/Timesys/Faraday/MCC/mccReleaseFinal/build_armv7l-timesys-linux-gnueabi/libmcc-1.0/libmcc-1.0/include -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF "$(@:%.o=%.d)" -MT "$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


