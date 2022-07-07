CXXFLAGS=-O2 -Wall -Wextra -I./include -pthread -DNDEBUG -fsanitize=address
LDFLAGS= -fsanitize=address
LDFLAGS_TEST= $(LDFLAGS) -L/usr/local/lib -lgtest -lgtest_main -lpthread

SRC = \
	arm.cc \
	mem.cc \
	system.cc
TEST_OBJ = \
	tests/execute_unittest.o

OBJ=$(SRC:.cc=.o)
DEP=$(SRC:.cc=.d)

TARGET = emu-aarch64
TEST_TARGET = emu-test
TEST_DATA = emu-testgen

all: $(TARGET)
test: $(TEST_TARGET) $(TEST_DATA)

$(TARGET): $(OBJ) main.o
	$(CXX) -o $@ $^ $(LDFLAGS)

$(TEST_TARGET): $(OBJ) $(TEST_OBJ)
	$(CXX) -o $@ $^ $(LDFLAGS_TEST)

$(TEST_DATA): tests/create_testdata.o tests/as.o
	$(CXX) -o $@ $^ $(LDFLAGS)
	./emu-testgen
	./tests/gen-testdata.sh

%.o: %.cc
	$(CXX) $(CXXFLAGS) -c $< -o $@ -MMD -MP -MF $(@:.o=.d)

%.o: %.s
	$(CXX) -c $< -o $@

%.bin: %.s
	bash extract_text.sh $< $@

-include $(DEP)

run:
	./$(TARGET) $(BIN)

run-test:
	./$(TEST_TARGET)

format: $(SRC) $(HEADER) $(MAIN)
	clang-format -i $^

clean:
	find ./ -type f -name "*.o" -or -name "*.d" -or -name "*.out" | xargs rm -rf
	rm -f $(TARGET) $(TEST_TARGET) $(TEST_DATA) $(OBJ) $(TEST_OBJ) $(DEP) main.o main.d $(BIN) tests/tmp.o tmp.o
	rm -f ./tests/data/*.s ./tests/data/*.bin

.PHONY: all test run run-test format clean

.SECONDARY: $(OBJ)
