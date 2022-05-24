CC=clang++
CXXFLAGS=-std=C++17 -O2 -Wall -I./
LFLAGS=

SRC=\
	cpu.cc \
	emulator.cc
HEADER=\
	cpu.h \
	emulator.h

SRC_OBJ=$(SRC:.cc=.o)
TARGET = emu-aarch64 

all: $(SRC_OBJ)
	$(CC) $(LFLAGS) -o $(TARGET) $^

$(SRC_OBJ): %.o: %.cc $(HEADER)
	$(CC) $(CPPFLAGS) -o $(<:.cc=.o) -c $<

run: $(TARGET)
	./$(TARGET)

format: $(SRC) $(HEADER) $(MAIN)
	clang-format -i $^

clean:
	rm -f $(SRC_OBJ) $(MAIN_OBJ) $(TARGET)

.PHONY: all run format clean
