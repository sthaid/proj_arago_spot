CC       = gcc
CFLAGS   = -g -O2 -Wall -Iutil
LDFLAGS  = -lm -lfftw3 -lSDL2 -lSDL2_ttf -lpthread

TARGET   = sim
SOURCES  = main.c aperture.c angular_spectrum_method.c display.c util/util_sdl.c util/util_misc.c 

util/util_sdl.o: CFLAGS += $(shell sdl2-config --cflags)

OBJ := $(SOURCES:.c=.o)

$(TARGET): $(OBJ)
	$(CC) -o $@ $(OBJ) $(LDFLAGS)

clean:
	rm -f $(TARGET) $(OBJ)
