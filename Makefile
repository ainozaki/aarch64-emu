CXXFLAGS=-O2 -Wall -Wextra -I./include -pthread -DNDEBUG -fsanitize=address
LDFLAGS= -fsanitize=address
LDFLAGS_TEST= $(LDFLAGS) -L/usr/local/lib -lgtest -lgtest_main -lpthread

SRC = \
	arm.cc \
	mem.cc \
	system.cc
TEST_SRC = \
	tests/execute_unittest.cc
TEST_AS = tests/test_branch.s

OBJ=$(SRC:.cc=.o)
TEST_OBJ=$(TEST_SRC:.cc=.o)
DEP=$(SRC:.cc=.d) $(TEST_SRC:.cc=.d)
BIN = $(TEST_AS:.s=.bin)

TARGET = emu-aarch64
TEST_TARGET = emu-test

all: $(TARGET)
test: $(TEST_TARGET) $(BIN)

$(TARGET): $(OBJ) main.o
	$(CXX) -o $@ $^ $(LDFLAGS)

$(TEST_TARGET): $(OBJ) $(TEST_OBJ)
	$(CXX) -o $@ $^ $(LDFLAGS_TEST)

%.o: %.cc
	$(CXX) $(CXXFLAGS) -c $< -o $@ -MMD -MP -MF $(@:.o=.d)

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
	rm -f $(TARGET) $(TEST_TARGET) $(OBJ) $(TEST_OBJ) $(DEP) main.o main.d $(BIN) tests/tmp.o tmp.o

.PHONY: all test run run-test format clean

.SECONDARY: $(OBJ)
