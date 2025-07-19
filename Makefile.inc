###########################################################################
## GENERIC TEMPLATE - USAGE
###########################################################################
# The following variables should be set in your makefile before including
# this file.
### Library version.  Must use Semantic Versioning 'x.y.z'.  E.g. '0.1.0'
#APP_MAJOR_VER = 0
#APP_MINOR_VER = 1
#APP_PATCH_VER = 0
#
### Name of the library (do not prefix with 'lib'), e.g. myUtil
#TARGET_NAME = myUtil
#
### List of private source files (do not include file extensions)
### Do not put public header files in the list, only private ones.
#CPP_SRC_LIST =
#C_SRC_LIST =
#H_SRC_LIST =
### API_HEADER_LIST contains all public header files, *excluding* the $(TARGET_NAME).h file.
#API_HEADER_LIST =
#
### List of System Lib Folders. "-l" will be prepended to each element.
#PKG_LIST =
### example = el-lib/libSysUtil.git el-lib/libVidUtil.git
#DEP_BUILD_LIST =
#SYS_INC_DIRS =
#SYS_LIBS_DIRS =
### example GLEW glfw GL freetype X11 Xxf6vm Xrandr pthread Xi jpeg m
#SYS_DYN_LIBS_LIST =
#SYS_STAT_LIBS_LIST =
#
### To use sysroot, either define the environment variable externally, or set it manually
# SYSROOT =


###########################################################################
## GENERIC TEMPLATE - DO NOT EDIT
###########################################################################

###########################################################################
## Useful functions for parsing strings like 'repo.git', 'mybranch!repo.git'
# Returns the .git repo after optional separator [mybranch!]repo.git
repo = $(lastword $(subst !, ,$1))
# Returns the branch in 'mybranch!repo.git' if present, otherwise master
branch = $(if $(word 2,$(subst !, ,$1)), $(firstword $(subst !, ,$1)),master)

###########################################################################

STATIC_ONLY = 1

# Check for environment variables.
ifndef SHELL
SHELL = /usr/bin/sh
endif

ifndef ARCH
ARCH = $(shell uname -m)
endif

ifndef OS_SIZE
OS_SIZE = $(shell getconf LONG_BIT)
$(info INFO: OS_SIZE not specified. Using host size)
endif

# Python Related
export PYTHON ?= python3
export PYTHON_SCRIPTS_DIR ?= $(CURDIR)/scripts/avalon_app_build_engine.$(MCU_TYPE)/python/buildScripts

# Set default makefile name
MAKEFILE_NAME ?= Makefile

CC      = $(TOOL_PREFIX)gcc
CXX     = $(TOOL_PREFIX)g++
AS      = $(TOLL_PREFIX)as
AR      = $(TOOL_PREFIX)gcc-ar
LD      = $(TOOL_PREFIX)ld
OBJCOPY = $(TOOL_PREFIX)objcopy

ifndef DESTDIR
DESTDIR =$(DIST_ARCHDIR)
endif

# Setup the directory structure
DISTDIR = $(CURDIR)/dist/
API_INCDIR = $(CURDIR)/inc/
LOCAL_INCDIR = $(CURDIR)/src/
SRCDIR = $(CURDIR)/src/
TSTDIR = $(CURDIR)/tests/
# dependency dir can come from the environment
export DEPDIR ?= $(CURDIR)/deps

# Define the output directories
OBJDIR = $(CURDIR)/obj/$(ARCH)/
# DISTDIR can come from the environment
export DISTDIR = $(CURDIR)/dist
export DIST_ARCHDIR ?= $(CURDIR)/dist/$(ARCH)
PREFIX ?= /
DIST_LIBDIR ?= $(DIST_ARCHDIR)$(PREFIX)/lib/
DIST_INCDIR ?= $(DIST_ARCHDIR)$(PREFIX)/include/
DIST_BINDIR ?= $(DIST_ARCHDIR)$(PREFIX)/bin/
OUTPUT_DIRS = $(DIST_ARCHDIR) $(DIST_LIBDIR) $(DIST_INCDIR) $(DIST_INCDIR)/$(TARGET_NAME) $(DIST_BINDIR) $(OBJDIR) $(DEPDIR)
MKDIR_P = mkdir -p

# Compiler and Linker settings
ifdef NO_PRINTF
COMMON_FLAGS += -DNO_PRINTF
endif
COMMON_FLAGS += -DLIB_MAJOR_VERSION=$(LIB_MAJOR_VER) -DLIB_MINOR_VERSION=$(LIB_MINOR_VER) -DLIB_PATCH_VERSION=$(LIB_PATCH_VER)

# Preprocessor flags
CPPFLAGS += -c -Wall $(COMMON_FLAGS)
CPPFLAGS += -ffunction-sections -fdata-sections -fno-exceptions
CPPFLAGS += -Wno-error=narrowing
CPPFLAGS += -mthumb -mcpu=cortex-m7 -mfloat-abi=hard -mfpu=fpv5-d16
CPPFLAGS += -nostdlib
CPPFLAGS += -D__IMXRT1062__ -DTEENSYDUINO=154 -DARDUINO=10815 -DF_CPU=600000000 -DLAYOUT_US_ENGLISH
# for teensy, std::mutex is provided by TeensyThreads
CPPFLAGS += -DSTD_MUTEX_EXTERNAL="\"TeensyThreads.h\""
CPPFLAGS += -DAUDIO_BLOCK_SAMPLES=128 -DAUDIO_SAMPLE_RATE_EXACT=48000.0f

ifndef AVALON_REV
export AVALON_REV=1
endif

ifeq ($(AVALON_REV),1)
CPPFLAGS += -DARDUINO_TEENSY41 -DAVALON_REV1
endif

ifeq ($(AVALON_REV),2)
CPPFLAGS += -DARDUINO_TEENSY_MICROMOD -DAVALON_REV2
endif
CPPFLAGS += $(SYS_INC_DIRS) -I$(API_INCDIR) -I$(LOCAL_INCDIR) -I$(DIST_INCDIR)

# The core repo is special and it's path is manually included
CPPFLAGS  += -I$(DIST_INCDIR)/cores

# include all subdirs in the repo 'inc' folder
#CPPFLAGS += -I$(dir $(wildcard $(DIST_INCDIR)/*))
#CPPFLAGS += $(addprefix -I, $(shell find $(DIST_INCDIR) -type d))

TEENSYLIBS_INCLUDE_LIST = Adafruit_SSD1306 Adafruit_SH1106 Adafruit-GFX-Library Adafruit_BusIO globalCompat Bounce2 \
                          Encoder Wire SPI MIDI TeensyThreads WDT_T4 EEPROM Crypto Teensy-4.x-Quad-Encoder-Library LittleFS \
                          TeensyDebug
TEENSYLIBS_INCLUDE_PATHS = $(addprefix -I$(DIST_INCDIR)/, $(TEENSYLIBS_INCLUDE_LIST))
CPPFLAGS += $(TEENSYLIBS_INCLUDE_PATHS)

# Teensy library list for the linker
TEENSY_CORE_LIBS = Adafruit_SSD1306 Adafruit_SH1106 Adafruit-GFX-Library Adafruit_BusIO Bounce2 Encoder Teensy-4.x-Quad-Encoder-Library Wire \
    SPI MIDI EEPROM Crypto TeensyThreads LittleFS cores

# note ARM EABI compiler doesn't support -march option
CFLAGS   += -std=gnu99 $(COMMON_FLAGS)
CXXFLAGS += -std=gnu++17 -fpermissive -fno-rtti -fno-threadsafe-statics -felide-constructors

# Linker Flags
LDFLAGS  += -O2 -Wl,--gc-sections,--relax -Wl,--exclude-libs,ALL
LDFLAGS  += -mthumb -mcpu=cortex-m7 -mfloat-abi=hard -mfpu=fpv5-d16
LDFLAGS_BASE_LIBS += -larm_cortexM7lfsp_math -lm -lstdc++

ifeq ($(AVALON_REV),2)
LD_FILE  += -T./scripts/avalon_app_build_engine.$(MCU_TYPE)/imxrt1062.micromod.ld
endif

# Archiver flags
ARFLAGS   = -cr

# HEX Flags
#OBJCOPY_EE_FLAGS += -O ihex -j .eeprom --set-section-flags=.eeprom=alloc,load --no-change-warnings --change-section-lma .eeprom=0
OBJCOPY_HEX_FLAGS += -O ihex -R .eeprom

DEBUGFLAGS     = -g -O0 -D_DEBUG -DUSB_DUAL_SERIAL
RELEASEFLAGS   = -s -fvisibility=hidden -O2 -D NDEBUG -DUSB_MIDI_AUDIO_SERIAL
DEFAULTFLAGS   = -s -fvisibility=hidden -O2 -D NDEBUG -DUSB_MIDI_AUDIO_SERIAL

TARGET_LIST = $(TARGET_NAME).$(APP_VER)

#Prepend the path
SYS_DYN_LIBS = -L$(DIST_LIBDIR) $(SYS_LIBS_DIRS) $(addprefix -l, $(SYS_DYN_LIBS_LIST))
SYS_STAT_LIBS = -L$(DIST_LIBDIR) $(SYS_LIBS_DIRS) $(addprefix -l, $(SYS_STAT_LIBS_LIST) $(TEENSY_CORE_LIBS))
API_HEADERS = $(addsuffix .h, $(addprefix $(API_INCDIR), $(API_HEADER_LIST)))
LOCAL_HEADERS += $(addsuffix .h, $(addprefix $(LOCAL_INCDIR), $(H_SRC_LIST)))
SOURCES_CPP = $(addsuffix .cpp, $(addprefix $(SRCDIR), $(CPP_SRC_LIST)))
SOURCES_C = $(addsuffix .c, $(addprefix $(SRCDIR), $(C_SRC_LIST)))
SOURCES_S = $(addsuffix .S, $(addprefix $(SRCDIR), $(S_SRC_LIST)))

OBJECTS_CPP = $(addsuffix .o, $(addprefix $(OBJDIR), $(CPP_SRC_LIST)))
OBJECTS_C = $(addsuffix .o, $(addprefix $(OBJDIR), $(C_SRC_LIST)))
OBJECTS_S = $(addsuffix .o, $(addprefix $(OBJDIR), $(S_SRC_LIST)))

PREPROC_DEFINES = $(addprefix -D, $(PREPROC_DEFINES_LIST))

ifeq ($(NO_PRINTF),1)
PREPROC_DEFINES += -DNO_PRINTF
endif

CPPFLAGS += $(PREPROC_DEFINES)

# Create a list of the examples based on directories
EXAMPLE_DIRS = $(sort $(dir $(wildcard ./examples/*/)))

TARGET =  $(addprefix $(DIST_BINDIR), $(TARGET_LIST))
TARGET_STATIC =  $(addsuffix _static, $(addprefix $(DIST_BINDIR), $(TARGET_LIST)))

ifdef STATIC_ONLY
DYN_TARGET =
endif

all: build_engine_ver directories $(TARGET)
all-nodeps : directories $TARGET)_nodeps

build_engine_ver :
	@cd $(APP_BUILD_ENGINE_LOC) && \
	echo "APP_BUILD_ENGINE_LOC: $(APP_BUILD_ENGINE_LOC)" && \
	echo "APP_BUILD_ENGINE GIT BRANCH: `git symbolic-ref -q --short HEAD || git describe --tags --exact-match` : `git rev-parse --verify HEAD`"

directories:
	$(MKDIR_P) $(OUTPUT_DIRS)

$(TARGET): deps $(OBJECTS_C) $(OBJECTS_CPP) $(OBJECTS_S)
	$(CC) $(COMMON_FLAGS) -o $(TARGET) $(LDFLAGS) $(LD_FILE) $(OBJECTS_CPP) $(SYS_STAT_LIBS) $(LDFLAGS_BASE_LIBS)
	$(OBJCOPY) $(OBJCOPY_HEX_FLAGS) $(TARGET) $(TARGET).hex


nodeps: $(OBJECTS_C) $(OBJECTS_CPP) $(OBJECTS_S)
	$(CC) $(COMMON_FLAGS) -o $(TARGET) $(LDFLAGS) $(LD_FILE) $(OBJECTS_CPP) $(SYS_STAT_LIBS) $(LDFLAGS_BASE_LIBS)
	$(OBJCOPY) $(OBJCOPY_HEX_FLAGS) $(TARGET) $(TARGET).hex

debug: DEFAULTFLAGS   = $(DEBUGFLAGS)
debug: all

release: DEFAULTFLAGS   = $(RELEASEFLAGS)
release: all

#profile: CFLAGS += -pg
#profile: CXXFLAGS += -pg
#profile: DEFAULTFLAGS   = $(RELEASEFLAGS)
#profile: all

clean:
	-rm -f $(OBJECTS_C) $(OBJECTS_CPP) $(OBJECTS_CU)
	-rm -f $(TARGET) $(TARGET).hex

distclean: clean
	-$(foreach rep, $(DEP_BUILD_LIST), make -C $(DEPDIR)/$(call repo,$(rep)) clean;)
	-rm -rf $(DIST_ARCHDIR)

mrproper: clean
	-rm -rf $(DEPDIR) $(DISTDIR) $(OBJDIR)

veryclean: distclean

set-install-local:
	export DESTDIR=$(CURDIR)/dist/$(ARCH)
install: all
	$(MKDIR_P) $(DESTDIR)$(PREFIX)/lib
	$(MKDIR_P) $(DESTDIR)$(PREFIX)/include
	$(MKDIR_P) $(DESTDIR)$(PREFIX)/include/$(TARGET_NAME)
ifndef STATIC_ONLY
	@if ! [ $(DYN_TARGET) -ef $(DESTDIR)$(PREFIX)/lib/$(DYN_TARGET_LIST) ]; then \
	cp -f $(DYN_TARGET) $(DESTDIR)$(PREFIX)/lib/$(DYN_TARGET_LIST); \
	fi
	ln -f -s lib$(TARGET_NAME).so.$(APP_VER) $(DESTDIR)$(PREFIX)/lib/lib$(TARGET_NAME).so
	ln -f -s lib$(TARGET_NAME).so.$(APP_VER) $(DESTDIR)$(PREFIX)/lib/lib$(TARGET_NAME).so.$(APP_MAJOR_VER)
	ln -f -s lib$(TARGET_NAME).so.$(APP_VER) $(DESTDIR)$(PREFIX)/lib/lib$(TARGET_NAME).so.$(APP_MAJOR_VER).$(APP_MINOR_VER)
endif
ifndef DYNAMIC_ONLY
	@if ! [ $(STATIC_TARGET) -ef $(DESTDIR)$(PREFIX)/lib/$(STATIC_TARGET_LIST) ]; then \
	cp -f $(STATIC_TARGET) $(DESTDIR)$(PREFIX)/lib/$(STATIC_TARGET_LIST); \
	fi
endif
	-cp -f $(API_HEADERS) $(DESTDIR)$(PREFIX)/include/$(TARGET_NAME)

deps: $(DEP_BUILD_LIST)
%.git:
	@echo -e "\nAPP DEPENDENCIES: $(call branch,$@) @ $(call repo,$@) \n" \
        && echo "Calling $(PYTHON) $(PYTHON_SCRIPTS_DIR)/dependencies.py $(call repo,$@) $(call branch,$@)" \
	&& $(PYTHON) $(PYTHON_SCRIPTS_DIR)/dependencies.py $(DEPDIR) $(call repo,$@) $(call branch,$@) \
	&& cd $(DEPDIR)/$(call repo,$@) \
	&& git submodule init \
	&& git submodule update \
	&& cd $(CURDIR) \
	|| ( rm -Rf $(DEPDIR)/$(call repo,$@); exit 1 ); \
	make $(MAKECMDGOALS) -f $(MAKEFILE_NAME) -C $(DEPDIR)/$(call repo,$@) DESTDIR=$(DESTDIR) install || exit 1; \

report_deps:
	$(PYTHON) $(PYTHON_SCRIPTS_DIR)/printVersionAll.py $(DEPDIR)

printvar:
	$(foreach v, $(.VARIABLES), $(info $(v) = $($(v))))

$(OBJDIR)%.o: $(SRCDIR)%.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(DEFAULTFLAGS) -c -o $@ $<

$(OBJDIR)%.o: $(SRCDIR)%.c
	$(CC) $(CPPFLAGS) $(CFLAGS) $(DEFAULTFLAGS) -c -o $@ $<

$(OBJDIR)%.o: $(SRCDIR)%.S
	$(CC) $(CPPFLAGS) -x assembler-with-cpp $(DEFAULTFLAGS) -c -o $@ $<


# .PHONY prevent make from having issues with source files called all, profile, release, etc.
.PHONY : all profile release debug deb\
	install install-strip uninstall clean distclean veryclean directories symlinks set-install-local pkgconfig deps deb printvar examples
