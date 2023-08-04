################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../src/comunicacion.c \
../src/interrupciones.c \
../src/kernel.c \
../src/planificacion.c 

C_DEPS += \
./src/comunicacion.d \
./src/interrupciones.d \
./src/kernel.d \
./src/planificacion.d 

OBJS += \
./src/comunicacion.o \
./src/interrupciones.o \
./src/kernel.o \
./src/planificacion.o 


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.c src/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -I"/home/utnso/tp-2023-1c-Los-Magios/shared_tuki/src" -O0 -g3 -Wall -c -fmessage-length=0 -fPIC -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


clean: clean-src

clean-src:
	-$(RM) ./src/comunicacion.d ./src/comunicacion.o ./src/interrupciones.d ./src/interrupciones.o ./src/kernel.d ./src/kernel.o ./src/planificacion.d ./src/planificacion.o

.PHONY: clean-src

