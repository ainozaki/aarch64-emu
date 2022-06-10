CC=clang++
CXXFLAGS=-O2 -Wall -I./include
LFLAGS=
LFLAGS_TEST=-L/usr/local/lib -lgtest -lgtest_main -pthread

TARGET = \
	arm.o \
	arm_decoder.o \
	arm_op.o \
	mem.o \
	system.o
TEST_TARGET = \
	tests/execute_unittest.o

all: emu-aarch64
test: emu-test

emu-aarch64: $(TARGET) main.o
	$(CXX) -o $@ $^

emu-test: $(TARGET) $(TEST_TARGET)
	$(CXX) -o $@ $^

%.o: %.cc
	$(CXX) -c $< -o $@ $(CXXFLAGS)

run:
	./$(TARGET)

run-test:
	./$(TEST_TARGET)

format: $(SRC) $(HEADER) $(MAIN)
	clang-format -i $^

clean:
	rm -f $(TARGET) $(TEST_TARGET) *.o emu-aarch64

.PHONY: all test run run-test format clean
