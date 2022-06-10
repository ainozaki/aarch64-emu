CXXFLAGS=-O2 -Wall -Wextra -I./include -pthread -DNDEBUG -fsanitize=address
LDFLAGS= -fsanitize=address
LDFLAGS_TEST= $(LDFLAGS) -L/usr/local/lib -lgtest -lgtest_main -lpthread

SRC = \
	arm.cc \
	mem.cc \
	system.cc
TEST_SRC = \
	tests/execute_unittest.cc

OBJ=$(SRC:.cc=.o)
TEST_OBJ=$(TEST_SRC:.cc=.o)
DEP=$(SRC:.cc=.d) $(TEST_SRC:.cc=.d)

TARGET = emu-aarch64
TEST_TARGET = emu-test

all: $(TARGET)
test: $(TEST_TARGET)

$(TARGET): $(OBJ) main.o
	$(CXX) -o $@ $^ $(LDFLAGS)

$(TEST_TARGET): $(OBJ) $(TEST_OBJ)
	$(CXX) -o $@ $^ $(LDFLAGS_TEST)

%.o: %.cc
	$(CXX) $(CXXFLAGS) -c $< -o $@ -MMD -MP -MF $(@:.o=.d)

-include $(DEP)

run:
	./$(TARGET)

run-test:
	./$(TEST_TARGET)

format: $(SRC) $(HEADER) $(MAIN)
	clang-format -i $^

clean:
	rm -f $(TARGET) $(TEST_TARGET) $(OBJ) $(TEST_OBJ) $(DEP) main.o main.d

.PHONY: all test run run-test format clean

.SECONDARY: $(OBJ)
