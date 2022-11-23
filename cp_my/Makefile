CC = g++

EXECUTABLE = cp.elf

CFLAGS = -Wall -Wextra
DBG_FLAGS = -g -fsanitize=address -D_DEBUG

all: build

run:
	 ./$(EXECUTABLE) -vfi

build: cp.cpp
	   $(CC) $^ $(CFLAGS) -o $(EXECUTABLE)

debug: cp.cpp
	   $(CC) $^ $(CFLAGS) -o $(EXECUTABLE) $(DBG_FLAGS)

clean:
	- rm *.elf 2>/dev/null
	- rm *.o   2>/dev/null
