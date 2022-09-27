CXXFLAGS=-g -Wall -Wextra -I./src/include -pthread -DNDEBUG -fsanitize=address
LDFLAGS= -fsanitize=address -T ./linker_script.x
LDFLAGS_TEST= $(LDFLAGS) -L/usr/local/lib -lgtest -lgtest_main -lpthread

SRC = \
	src/bus.cc \
	src/cpu.cc \
	src/emulator.cc \
	src/loader.cc \
	src/mem.cc
TEST_OBJ = \
	tests/execute_unittest.o
TEST_GENOBJ =\
	tests/create_testdata.o \
	tests/data/adds.o \
	tests/data/fun_fibonacci.o \
	tests/data/fun_sum.o \
	tests/data/subs.o \
	tests/data/b.o \
	tests/data/ret.o

OBJ=$(SRC:.cc=.o)
DEP=$(SRC:.cc=.d)

TARGET = emu-aarch64
TEST_TARGET = emu-test
TEST_GENDATA = emu-testgen

all: $(TARGET)
test: $(TEST_TARGET) $(TEST_GENDATA)

$(TARGET): $(OBJ) src/main.o
	$(CXX) -o $@ $^ $(LDFLAGS)

$(TEST_TARGET): $(OBJ) $(TEST_OBJ)
	$(CXX) -o $@ $^ $(LDFLAGS_TEST)

$(TEST_GENDATA): $(TEST_GENOBJ)
	$(CXX) -o $@ $^ $(LDFLAGS)
	./tests/gen-testdata.sh

%.o: %.cc
	$(CXX) $(CXXFLAGS) -c $< -o $@ -MMD -MP -MF $(@:.o=.d)

%.o: %.s
	$(CXX) -c $< -o $@

%.bin: %.s
	bash extract_text.sh $< $@

-include $(DEP)

BIN=./misc/static

run:
	./$(TARGET) $(BIN)

run-test:
	./$(TEST_TARGET)

format: $(SRC) $(HEADER) $(MAIN)
	find ./ -type f -name "*.cc" -or -name "*.h" | xargs clang-format -i

clean:
	find ./ -type f -name "*.o" -or -name "*.d" -or -name "*.out" -or -name "*.bin" | xargs rm -rf
	rm -f $(TARGET) $(TEST_TARGET) $(TEST_GENDATA) $(OBJ) $(TEST_OBJ) $(DEP) main.o main.d tests/tmp.o tmp.o

.PHONY: all test run run-test format clean

.SECONDARY: $(OBJ)
