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



C_FILES=src/main.c

C_FILES+=src/sstr.c
C_FILES+=src/cbuf.c

C_FILES+=src/gb.c
C_FILES+=src/mem.c
C_FILES+=src/mbc.c
C_FILES+=src/rom.c
C_FILES+=src/input.c
C_FILES+=src/sm83/sm83.c
C_FILES+=src/sm83/sm83_alu.c

INCLUDES=-I$(PROJ_DIR)/src
INCLUDES+=-I$(PROJ_DIR)/src/sm83

.PHONY: regular clean

regular:
	mkdir -p $(BUILD_DIR)
	gcc -c $(C_FILES) $(INCLUDES) $(ACGB_DEFINES) $(C_FLAGS)
#probably could do this in a better way
	mv $(PROJ_DIR)/*.o $(BUILD_DIR)
	gcc -g -o $(BUILD_DIR)/$(PROJ_NAME) $(BUILD_DIR)/*.o

clean:
	rm -rf $(BUILD_DIR)