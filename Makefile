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
INCLUDES+=-I$(PROJ_DIR)/lib/SDL/include
INCLUDES+=-I$(PROJ_DIR)/lib/SDL_mixer/include
INCLUDES+=-I$(PROJ_DIR)/lib/SDL_ttf/include

LIB_DIRS=-L$(PROJ_DIR)/lib/build/SDL
LIB_DIRS+=-L$(PROJ_DIR)/lib/build/SDL_mixer
LIB_DIRS+=-L$(PROJ_DIR)/lib/build/SDL_ttf

LIBS+=-lSDL3
LIBS+=-lSDL3_mixer
LIBS+=-lSDL3_ttf
LIBS+=-lm

.PHONY: regular clean lib src libclean

regular: lib src

lib:
	cd lib && cmake -S . -B build && cmake --build build --parallel
	cd ..

src:
	mkdir -p $(BUILD_DIR)
	gcc -c $(C_FILES) $(INCLUDES) $(C_FLAGS)
#probably could do this in a better way
	mv $(PROJ_DIR)/*.o $(BUILD_DIR)
	gcc -g -o $(BUILD_DIR)/$(PROJ_NAME) $(BUILD_DIR)/*.o $(LIB_DIRS) $(LIBS)

clean:
	rm -rf $(BUILD_DIR)

libclean:
	rm -rf $(PROJ_DIR)/lib/build