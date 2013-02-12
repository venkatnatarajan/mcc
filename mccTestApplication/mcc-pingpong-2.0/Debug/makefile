################################################################################
# Automatically-generated file. Do not edit!
################################################################################

### Toolchain: twr_vf600-armv7l-timesys-linux-gnueabi-gcc-4.6.3 ###
RANLIB=/media/Data/Timesys/Faraday/FreescaleRelease3/factory/build_armv7l-timesys-linux-gnueabi/toolchain/bin/armv7l-timesys-linux-gnueabi-ranlib
CPP=/media/Data/Timesys/Faraday/FreescaleRelease3/factory/build_armv7l-timesys-linux-gnueabi/toolchain/bin/armv7l-timesys-linux-gnueabi-cpp
AS=/media/Data/Timesys/Faraday/FreescaleRelease3/factory/build_armv7l-timesys-linux-gnueabi/toolchain/bin/armv7l-timesys-linux-gnueabi-as
AR=/media/Data/Timesys/Faraday/FreescaleRelease3/factory/build_armv7l-timesys-linux-gnueabi/toolchain/bin/armv7l-timesys-linux-gnueabi-ar
OBJCOPY=/media/Data/Timesys/Faraday/FreescaleRelease3/factory/build_armv7l-timesys-linux-gnueabi/toolchain/bin/armv7l-timesys-linux-gnueabi-objcopy
DEBUGGER=/media/Data/Timesys/Faraday/FreescaleRelease3/factory/build_armv7l-timesys-linux-gnueabi/toolchain/bin/armv7l-timesys-linux-gnueabi-gdb
STRIP=/media/Data/Timesys/Faraday/FreescaleRelease3/factory/build_armv7l-timesys-linux-gnueabi/toolchain/bin/armv7l-timesys-linux-gnueabi-strip
OBJDUMP=/media/Data/Timesys/Faraday/FreescaleRelease3/factory/build_armv7l-timesys-linux-gnueabi/toolchain/bin/armv7l-timesys-linux-gnueabi-objdump
CC=/media/Data/Timesys/Faraday/FreescaleRelease3/factory/build_armv7l-timesys-linux-gnueabi/toolchain/bin/armv7l-timesys-linux-gnueabi-gcc
NM=/media/Data/Timesys/Faraday/FreescaleRelease3/factory/build_armv7l-timesys-linux-gnueabi/toolchain/bin/armv7l-timesys-linux-gnueabi-nm
CXX=/media/Data/Timesys/Faraday/FreescaleRelease3/factory/build_armv7l-timesys-linux-gnueabi/toolchain/bin/armv7l-timesys-linux-gnueabi-g++
LD=/media/Data/Timesys/Faraday/FreescaleRelease3/factory/build_armv7l-timesys-linux-gnueabi/toolchain/bin/armv7l-timesys-linux-gnueabi-ld

-include ../makefile.init

RM := rm -rf

# All of the sources participating in the build are defined here
-include sources.mk
-include subdir.mk
-include src/subdir.mk
-include objects.mk

ifneq ($(MAKECMDGOALS),clean)
ifneq ($(strip $(C_DEPS)),)
-include $(C_DEPS)
endif
endif

-include ../makefile.defs

# Add inputs and outputs from these tool invocations to the build variables 

# All Target
all: pingpong

# Tool invocations
pingpong: $(OBJS) $(USER_OBJS)
	@echo 'Building target: $@'
	@echo 'Invoking: GCC C Linker'
	$(CC) -L"/media/Data/Timesys/Faraday/MCC/mccReleaseFinal/build_armv7l-timesys-linux-gnueabi/libmcc-1.0/libmcc-1.0/Debug" -o "pingpong" $(OBJS) $(USER_OBJS) $(LIBS)
	@echo 'Finished building target: $@'
	@echo ' '

# Other Targets
clean:
	-$(RM) $(OBJS)$(C_DEPS)$(EXECUTABLES) pingpong
	-@echo ' '

.PHONY: all clean dependents
.SECONDARY:

-include ../makefile.targets
