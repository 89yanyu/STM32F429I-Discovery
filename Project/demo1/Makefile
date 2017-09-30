# GNU ARM Embedded Toolchain
CC=arm-none-eabi-gcc
LD=arm-none-eabi-ld
AR=arm-none-eabi-ar
AS=arm-none-eabi-as
CP=arm-none-eabi-objcopy
OD=arm-none-eabi-objdump

COMLIB_HOME = ../..

# Build Parameters: MCU Flags, Definitions, Includes, 
#                   Compile Flags, Linker Script, Linker Flags
MCFLAGS=-mcpu=cortex-m4 -mthumb -mlittle-endian \
-mfpu=fpv4-sp-d16 -mfloat-abi=softfp -mthumb-interwork
DEFS=-DUSE_STDPERIPH_DRIVER -DSTM32F4XX
INCLUDES=-IInclude \
-I$(COMLIB_HOME)/STM32F429I-Discovery \
-I$(COMLIB_HOME)/CMSIS/Include \
-I$(COMLIB_HOME)/StdPeriph \
-I$(COMLIB_HOME)/StdPeriph/inc \
-I$(COMLIB_HOME)/FreeRTOS/Source/portable/GCC/ARM_CM4F \
-I$(COMLIB_HOME)/FreeRTOS/Source/include \
-I$(COMLIB_HOME)/TraceRecorder/include \
-I$(COMLIB_HOME)/TraceRecorder/streamports/Jlink_RTT/include \
-I$(COMLIB_HOME)/Common
CFLAGS=-c $(MCFLAGS) $(DEFS) $(INCLUDES)
LDSCRIPT = ./stm32_flash.ld
LDFLAGS=-T $(LDSCRIPT) --specs=nosys.specs $(MCFLAGS)

# Inputs: C Sources, Assembler Sources
SOURCES= \
Source/main.c \
$(COMLIB_HOME)/STM32F429I-Discovery/stm32f429i_discovery.c \
$(COMLIB_HOME)/STM32F429I-Discovery/stm32f429i_discovery_lcd.c \
$(COMLIB_HOME)/STM32F429I-Discovery/stm32f429i_discovery_sdram.c \
$(COMLIB_HOME)/StdPeriph/stm32f4xx_it.c \
$(COMLIB_HOME)/StdPeriph/system_stm32f4xx.c \
$(COMLIB_HOME)/StdPeriph/src/stm32f4xx_syscfg.c \
$(COMLIB_HOME)/StdPeriph/src/misc.c \
$(COMLIB_HOME)/StdPeriph/src/stm32f4xx_gpio.c \
$(COMLIB_HOME)/StdPeriph/src/stm32f4xx_rcc.c \
$(COMLIB_HOME)/StdPeriph/src/stm32f4xx_exti.c \
$(COMLIB_HOME)/StdPeriph/src/stm32f4xx_i2c.c \
$(COMLIB_HOME)/StdPeriph/src/stm32f4xx_dma.c \
$(COMLIB_HOME)/StdPeriph/src/stm32f4xx_dma2d.c \
$(COMLIB_HOME)/StdPeriph/src/stm32f4xx_ltdc.c \
$(COMLIB_HOME)/StdPeriph/src/stm32f4xx_spi.c \
$(COMLIB_HOME)/StdPeriph/src/stm32f4xx_fmc.c \
$(COMLIB_HOME)/StdPeriph/src/stm32f4xx_tim.c \
$(COMLIB_HOME)/FreeRTOS/Source/portable/GCC/ARM_CM4_Soft/port.c \
$(COMLIB_HOME)/FreeRTOS/Source/portable/MemMang/heap_2.c \
$(COMLIB_HOME)/FreeRTOS/Source/croutine.c \
$(COMLIB_HOME)/FreeRTOS/Source/event_groups.c \
$(COMLIB_HOME)/FreeRTOS/Source/list.c \
$(COMLIB_HOME)/FreeRTOS/Source/queue.c \
$(COMLIB_HOME)/FreeRTOS/Source/tasks.c \
$(COMLIB_HOME)/FreeRTOS/Source/timers.c \
$(COMLIB_HOME)/TraceRecorder/trcKernelPort.c \
$(COMLIB_HOME)/TraceRecorder/trcSnapshotRecorder.c \
$(COMLIB_HOME)/TraceRecorder/trcStreamingRecorder.c \
$(COMLIB_HOME)/TraceRecorder/streamports/Jlink_RTT/SEGGER_RTT.c \
$(COMLIB_HOME)/TraceRecorder/streamports/Jlink_RTT/SEGGER_RTT_Printf.c

ASMSOURCES=$(COMLIB_HOME)/CMSIS/Source/startup_stm32f429_439xx.s

# Outputs: Object Files, ELF Executable & 
#          Converted ELF Executable to Intel HEX format
OBJECTS  = $(patsubst $(COMLIB_HOME)/%, Object/%, $(patsubst Source/%, $(COMLIB_HOME)/USER/%, $(SOURCES:%.c=%.o)))
OBJECTS += $(patsubst $(COMLIB_HOME)/%, Object/%, $(patsubst Source/%, $(COMLIB_HOME)/USER/%, $(ASMSOURCES:%.s=%.o)))
DIR = $(sort $(dir $(OBJECTS)))
EXECUTABLE=hello_rtos.elf
TARGET=hello_rtos.hex

# Build Rules
.PHONY: release
release: $(TARGET)

.PHONY: all
all: $(TARGET)

.PHONY: debug
debug: CFLAGS+=-g # Add debug flag
debug: LDFLAGS+=-g
debug: $(TARGET)

$(DIR):
	-mkdir -p "$@"

$(TARGET): $(DIR) $(EXECUTABLE)
	$(CP) -O ihex $(EXECUTABLE) $@
	@echo "Objcopy from ELF to IHEX complete!\n"

$(EXECUTABLE): $(OBJECTS) $(COMLIB_OBJECTS)
	$(CC) $(LDFLAGS) $(OBJECTS) $(COMLIB_OBJECTS) -o $@
	@echo "Linking complete!\n"

# Build the C source in common library
Object/%.o:$(COMLIB_HOME)/%.c
	$(CC) $(CFLAGS) $< -o $@
	@echo "Compiled "$<"!\n"

# Build the custom C source
Object/USER/%.o:Source/%.c
	$(CC) $(CFLAGS) $< -o $@
	@echo "Compiled "$<"!\n"

# Build the assemble source in common library
Object/%.o:$(COMLIB_HOME)/%.s
	$(CC) $(CFLAGS) $< -o $@
	@echo "Assambled "$<"!\n"

# Build the customassemble source in common library
Object/USER/%.o:Source/%.s
	$(CC) $(CFLAGS) $< -o $@
	@echo "Assambled "$<"!\n"

.PHONY: clean
clean:
	rm -f $(OBJECTS) $(EXECUTABLE) $(TARGET)
