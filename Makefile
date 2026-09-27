# compilation defines:
PROJ_NAME=aCgb
PROJ_DIR=$(shell pwd)
BUILD_DIR=$(PROJ_DIR)/build

# Compiler options
C_FLAGS=-std=c11
C_FLAGS+=-Werror
C_FLAGS+=-Wall

# Defines
#ACGB_DEFINES="-DGB_HOST_HAS_FILESYSTEM"



C_FILES=main.c

C_FILES+=sstr.c
C_FILES+=cbuf.c

C_FILES+=gb.c
C_FILES+=mem.c
C_FILES+=mbc.c
C_FILES+=rom.c
C_FILES+=input.c
C_FILES+=sm83/sm83.c
C_FILES+=sm83/sm83_alu.c

INCLUDES=-I$(PROJ_DIR)
INCLUDES+=-I$(PROJ_DIR)/sm83

.PHONY: regular clean

regular:
	mkdir -p $(BUILD_DIR)
	gcc -c $(C_FILES) $(INCLUDES) $(ACGB_DEFINES) $(C_FLAGS)
#probably could do this in a better way
	mv $(PROJ_DIR)/*.o $(BUILD_DIR)
	gcc -g -o $(BUILD_DIR)/$(PROJ_NAME) $(BUILD_DIR)/*.o

clean:
	rm -rf $(BUILD_DIR)