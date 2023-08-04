################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../src/comunicacion.c \
../src/estructuras.c \
../src/file_system.c 

C_DEPS += \
./src/comunicacion.d \
./src/estructuras.d \
./src/file_system.d 

OBJS += \
./src/comunicacion.o \
./src/estructuras.o \
./src/file_system.o 


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.c src/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -I"/home/utnso/tp-2023-1c-Los-Magios/shared_tuki/src" -O0 -g3 -Wall -c -fmessage-length=0 -fPIC -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


clean: clean-src

clean-src:
	-$(RM) ./src/comunicacion.d ./src/comunicacion.o ./src/estructuras.d ./src/estructuras.o ./src/file_system.d ./src/file_system.o

.PHONY: clean-src

