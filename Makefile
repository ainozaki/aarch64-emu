CC=clang++
CXXFLAGS=-O2 -Wall -I./include
LFLAGS=
LFLAGS_TEST=-L/usr/local/lib -lgtest -lgtest_main -pthread

MAIN=main.cc
SRC=\
	arm.cc \
	arm-op.cc \
	decoder.cc \
	emulator.cc
SRC_TEST=\
	tests/execute_unittest.cc
HEADER=\
	include/arm.h \
	include/arm-op.h \
	include/decoder.h \
	include/emulator.h \
	include/utils.h

TARGET = emu-aarch64
TEST_TARGET = test

all: $(SRC) $(HEADER)
	$(CC) $(CXXFLAGS) $(SRC) $(MAIN) -o $(TARGET)  $(LFLAGS)

run:
	./$(TARGET)

test: $(SRC) $(SRC_TEST) $(HEADER)
	$(CC) $(CXXFLAGS) $(SRC) $(SRC_TEST) -o $(TEST_TARGET) $(LFLAGS) $(LFLAGS_TEST)

run-test:
	./$(TEST_TARGET)

format: $(SRC) $(HEADER) $(MAIN)
	clang-format -i $^

clean:
	rm -f $(TARGET) $(TEST_TARGET) *.o

.PHONY: all test run format clean
