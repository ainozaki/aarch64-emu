CC=clang++
CXXFLAGS=-O2 -Wall -I./include
LFLAGS=

SRC=\
	arm.cc \
	arm-op.cc \
	emulator.cc
HEADER=\
	include/arm.h \
	include/arm-op.h \
	include/emulator.h \
	include/utils.h

SRC_OBJ=$(SRC:.cc=.o)
TARGET = emu-aarch64 

all: $(SRC_OBJ)
	$(CC) $(LFLAGS) -o $(TARGET) $^

$(SRC_OBJ): %.o: %.cc $(HEADER)
	$(CC) $(CXXFLAGS) -o $(<:.cc=.o) -c $<

run: $(TARGET)
	./$(TARGET)

format: $(SRC) $(HEADER) $(MAIN)
	clang-format -i $^

clean:
	rm -f $(SRC_OBJ) $(MAIN_OBJ) $(TARGET)

.PHONY: all run format clean
