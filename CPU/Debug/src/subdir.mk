################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../src/ciclo_de_instruccion.c \
../src/comunicacion.c \
../src/cpu.c \
../src/operaciones_cpu.c 

C_DEPS += \
./src/ciclo_de_instruccion.d \
./src/comunicacion.d \
./src/cpu.d \
./src/operaciones_cpu.d 

OBJS += \
./src/ciclo_de_instruccion.o \
./src/comunicacion.o \
./src/cpu.o \
./src/operaciones_cpu.o 


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.c src/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -I"/home/utnso/tp-2023-1c-Los-Magios/shared_tuki/src" -O0 -g3 -Wall -c -fmessage-length=0 -fPIC -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


clean: clean-src

clean-src:
	-$(RM) ./src/ciclo_de_instruccion.d ./src/ciclo_de_instruccion.o ./src/comunicacion.d ./src/comunicacion.o ./src/cpu.d ./src/cpu.o ./src/operaciones_cpu.d ./src/operaciones_cpu.o

.PHONY: clean-src

