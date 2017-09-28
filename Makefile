ifeq ($(origin COMLIB_HOME), undefined)
$(error COMLIB_HOME is undefined, how can you find me?)
endif

ifeq ($(origin COMLIB_SOURCES), undefined)
ifeq ($(origin COMLIB_ASMSOURCES), undefined)
$(error Can't find any source or asm source to build.)
endif
endif

ifdef COMLIB_SOURCES
COMLIB_DIR_TMP = $(dir $(COMLIB_SOURCES))
COMLIB_OBJECT_TMP = $(COMLIB_SOURCES:%.c=%.o)
endif
ifdef COMLIB_ASMSOURCES
COMLIB_DIR_TMP += $(dir $(COMLIB_ASMSOURCES))
COMLIB_OBJECT_TMP += $(COMLIB_ASMSOURCES:%.s=%.o)
endif

COMLIB_DIR = $(COMLIB_HOME)/Object
COMLIB_DIR += $(patsubst $(COMLIB_HOME)%, $(COMLIB_HOME)/Object%, $(COMLIB_DIR_TMP))
COMLIB_OBJECTS = $(patsubst $(COMLIB_HOME)%,$(COMLIB_HOME)/Object%, $(COMLIB_OBJECT_TMP))

$(COMLIB_DIR):

# Build Rules
# General rule like %.o:%.c defined by caller

$(COMLIB_OBJECTS): $(COMLIB_SOURCES) $(COMLIB_ASMSOURCES)
	$(CC) $(CFLAGS) $(filter $(patsubst %.o, %., $(patsubst $(COMLIB_HOME)/Object%, $(COMLIB_HOME)%, $@))%, $?) -o $@
	@echo "In COMLib, Compiled "$(filter $(patsubst %.o, %., $(patsubst $(COMLIB_HOME)/Object%, $(COMLIB_HOME)%, $@))%, $?)"!\n"

.PHONY: comlib_clean
comlib_clean:
	-rm -f $(COMLIB_OBJECTS)
