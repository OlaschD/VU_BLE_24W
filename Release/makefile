################################################################################
# Automatically-generated file. Do not edit!
################################################################################

-include ..\makefile.init

RM := del 

# All of the sources participating in the build are defined here
-include sources.mk
-include subdir.mk
-include libraries\Wire\src\utility\subdir.mk
-include libraries\Wire\src\subdir.mk
-include libraries\SoftwareSerial\src\subdir.mk
-include libraries\LiquidCrystal\src\subdir.mk
-include libraries\FastLED\subdir.mk
-include libraries\Adafruit_NeoPixel\subdir.mk
-include core\core\subdir.mk
-include objects.mk

ifneq ($(MAKECMDGOALS),clean)
ifneq ($(strip $(CC_DEPS)),)
-include $(CC_DEPS)
endif
ifneq ($(strip $(C++_DEPS)),)
-include $(C++_DEPS)
endif
ifneq ($(strip $(C_UPPER_DEPS)),)
-include $(C_UPPER_DEPS)
endif
ifneq ($(strip $(CXX_DEPS)),)
-include $(CXX_DEPS)
endif
ifneq ($(strip $(ASM_DEPS)),)
-include $(ASM_DEPS)
endif
ifneq ($(strip $(INO_DEPS)),)
-include $(INO_DEPS)
endif
ifneq ($(strip $(PDE_DEPS)),)
-include $(PDE_DEPS)
endif
ifneq ($(strip $(S_UPPER_DEPS)),)
-include $(S_UPPER_DEPS)
endif
ifneq ($(strip $(CPP_DEPS)),)
-include $(CPP_DEPS)
endif
ifneq ($(strip $(C_DEPS)),)
-include $(C_DEPS)
endif
endif

-include ..\makefile.defs

# Add inputs and outputs from these tool invocations to the build variables 
AR += \
K:\My_Eclipse_Arduino\VU_BLE_24W\Release\arduino.ar \

ELF += \
VU_BLE_24W.elf \

HEX += \
VU_BLE_24W.hex \


# All Target
all: VU_BLE_24W

# Tool invocations
VU_BLE_24W: $(HEX)
	@echo 'Building target: $@'
	@echo 'Printing size:'
	"C:\eclipse\arduinoPlugin\packages\arduino\tools\avr-gcc\5.4.0-atmel3.6.1-arduino2/bin/avr-size" -A "K:/My_Eclipse_Arduino/VU_BLE_24W/Release/VU_BLE_24W.elf"
	@echo 'Finished building target: $@'
	@echo ' '

K:\My_Eclipse_Arduino\VU_BLE_24W\Release\arduino.ar: $(AR_OBJ)
	@echo 'Starting archiver'
	"C:\eclipse\arduinoPlugin\packages\arduino\tools\avr-gcc\5.4.0-atmel3.6.1-arduino2/bin/avr-gcc-ar" rcs     K:/My_Eclipse_Arduino/VU_BLE_24W/Release/arduino.ar $(AR_OBJ)
	@echo 'Finished building: $@'
	@echo ' '

VU_BLE_24W.elf: $(LINK_OBJ) $(AR)
	@echo 'Starting combiner'
	"C:\eclipse\arduinoPlugin\packages\arduino\tools\avr-gcc\5.4.0-atmel3.6.1-arduino2/bin/avr-gcc" -Wall -Wextra -Os -Wl,--gc-sections,--relax -mmcu=atmega2560  -o "K:/My_Eclipse_Arduino/VU_BLE_24W/Release/VU_BLE_24W.elf"   $(LINK_OBJ)   K:/My_Eclipse_Arduino/VU_BLE_24W/Release/arduino.ar  "-LK:/My_Eclipse_Arduino/VU_BLE_24W/Release" -lm
	@echo 'Finished building: $@'
	@echo ' '

VU_BLE_24W.hex: $(ELF)
	@echo 'Do all objcopy commands'
	${A.RECIPE.OBJCOPY.EEP.PATTERN}
	${A.RECIPE.OBJCOPY.HEX.PATTERN}
	@echo 'Finished building: $@'
	@echo ' '

# Other Targets
clean:
	-$(RM) $(C_UPPER_DEPS)$(PDE_DEPS)$(C_DEPS)$(AR)$(CC_DEPS)$(AR_OBJ)$(C++_DEPS)$(LINK_OBJ)$(CXX_DEPS)$(ASM_DEPS)$(HEX)$(INO_DEPS)$(SIZEDUMMY)$(S_UPPER_DEPS)$(ELF)$(CPP_DEPS)
	-@echo ' '

.PHONY: all clean dependents

-include ..\makefile.targets
