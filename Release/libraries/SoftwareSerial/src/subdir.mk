################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
K:\arduino-1.8.9\hardware\arduino\avr\libraries\SoftwareSerial\src\SoftwareSerial.cpp 

LINK_OBJ += \
.\libraries\SoftwareSerial\src\SoftwareSerial.cpp.o 

CPP_DEPS += \
.\libraries\SoftwareSerial\src\SoftwareSerial.cpp.d 


# Each subdirectory must supply rules for building sources it contributes
libraries\SoftwareSerial\src\SoftwareSerial.cpp.o: K:\arduino-1.8.9\hardware\arduino\avr\libraries\SoftwareSerial\src\SoftwareSerial.cpp
	@echo 'Building file: $<'
	@echo 'Starting C++ compile'
	"C:\eclipse\arduinoPlugin\packages\arduino\tools\avr-gcc\5.4.0-atmel3.6.1-arduino2/bin/avr-g++" -c -g -Os -Wall -Wextra -std=gnu++11 -fpermissive -fno-exceptions -ffunction-sections -fdata-sections -fno-threadsafe-statics -Wno-error=narrowing -MMD -flto -mmcu=atmega2560 -DF_CPU=16000000L -DARDUINO=10802 -DARDUINO_AVR_MEGA2560 -DARDUINO_ARCH_AVR     -I"K:\arduino-1.8.9\hardware\arduino\avr\cores\arduino" -I"K:\arduino-1.8.9\hardware\arduino\avr\variants\mega" -I"K:\arduino\libraries\FastLED" -I"K:\arduino\libraries\LiquidCrystal" -I"K:\arduino-1.8.9\hardware\arduino\avr\libraries\SoftwareSerial\src" -I"K:\arduino-1.8.9\hardware\arduino\avr\libraries\Wire\src" -I"K:\arduino\libraries\Adafruit_NeoPixel" -I"K:\arduino\libraries\LiquidCrystal\src" -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -D__IN_ECLIPSE__=1 -x c++ "$<"  -o  "$@"
	@echo 'Finished building: $<'
	@echo ' '


