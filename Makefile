#
# Basic KallistiOS skeleton / test program
# Copyright (C) 2001-2004 Megan Potter
# Copyright (C) 2024 Falco Girgis
#   

# Put the filename of the output binary here
TARGET = 3dpinball-dc.elf

# List all of your C or C++ files here, but change the extension to ".o"
SOURCE_DIRS  := src
CPP_FILES    := $(foreach dir,$(SOURCE_DIRS),$(wildcard $(dir)/*.cpp))
C_FILES      := $(foreach dir,$(SOURCE_DIRS),$(wildcard $(dir)/*.c))
OBJS         := $(C_FILES:%.c=%.o) $(CPP_FILES:%.cpp=%.o)

CXXFLAGS     := -IlibADXplay/LibADX

# LibADX is built together with the game, no separate install into KOS addons needed
ADX_DIR      := libADXplay/LibADX
OBJS         += $(ADX_DIR)/libadx.o $(ADX_DIR)/snddrv.o
LIBS         :=

# Optional path to a directory of resources to bundle within your ELF binary.
# Its contents are accessible via the "/rd/" virtual directory at runtime.
KOS_ROMDISK_DIR = romdisk
OBJS           += romdisk.o

# Main rule which forces our ELF binary to be built
all: rm-elf $(TARGET)

# Include the common KOS Makefile rules and configuration
include $(KOS_BASE)/Makefile.rules

# Cleans the binary ELF file plus the intermediate .o files
clean: rm-elf
	-rm -f $(OBJS)

# Removes the binary ELF file
rm-elf:
	-rm -f $(TARGET)

# Invokes the compiler to build the target from our object files
$(TARGET): $(OBJS)
	kos-c++ -o $(TARGET) $(OBJS) $(LIBS)

# Attempts to run the target using the configured loader application
run: $(TARGET)
	$(KOS_LOADER) $(TARGET)

# Creates a distributable/release ELF which strips away its debug symbols
dist: $(TARGET)
	$(KOS_STRIP) $(TARGET)

# Disc image. Works with DreamSDK (makedisc) and with a plain Linux KOS install
# (KOS utils + mkisofs/genisoimage + cdi4dc). Tool paths can be overridden.
KOS_UTILS ?= $(KOS_BASE)/utils
SCRAMBLE  ?= $(shell command -v scramble 2>/dev/null || echo $(KOS_UTILS)/scramble/scramble)
MAKEIP    ?= $(shell command -v makeip 2>/dev/null || echo $(KOS_UTILS)/makeip/makeip)
MKISOFS   ?= $(shell command -v mkisofs 2>/dev/null || command -v genisoimage 2>/dev/null)
CDI4DC    ?= cdi4dc
DISC_NAME := $(basename $(TARGET))

cdi: $(TARGET) cd_root
	@test -x "$(SCRAMBLE)" || $(MAKE) -C $(KOS_UTILS)/scramble
	@test -x "$(MAKEIP)" || $(MAKE) -C $(KOS_UTILS)/makeip
	$(KOS_OBJCOPY) -R .stack -O binary $(TARGET) $(DISC_NAME).bin
	$(SCRAMBLE) $(DISC_NAME).bin cd_root/1ST_READ.BIN
	$(MAKEIP) -l logo.png -g "3D PINBALL" -f IP.BIN
	@if command -v makedisc >/dev/null 2>&1; then \
		echo makedisc $(DISC_NAME).cdi cd_root IP.BIN; \
		makedisc $(DISC_NAME).cdi cd_root IP.BIN; \
	else \
		test -n "$(MKISOFS)" || { echo "mkisofs/genisoimage not found (apt install genisoimage)"; exit 1; }; \
		command -v $(CDI4DC) >/dev/null 2>&1 || { echo "cdi4dc not found (build img4dc: https://github.com/Kazade/img4dc)"; exit 1; }; \
		$(MKISOFS) -C 0,11702 -V 3DPINBALL -G IP.BIN -r -J -l -o $(DISC_NAME).iso cd_root && \
		$(CDI4DC) $(DISC_NAME).iso $(DISC_NAME).cdi && rm -f $(DISC_NAME).iso; \
	fi

cd_root:
	mkdir cd_root