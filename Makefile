CXXFLAGS=-O2 -Wall -Wextra -I./src/include -pthread -DNDEBUG -fsanitize=address
LDFLAGS= -fsanitize=address
LDFLAGS_TEST= $(LDFLAGS) -L/usr/local/lib -lgtest -lgtest_main -lpthread

SRC = \
	src/arm.cc \
	src/mem.cc \
	src/system.cc
TEST_OBJ = \
	tests/execute_unittest.o

OBJ=$(SRC:.cc=.o)
DEP=$(SRC:.cc=.d)

TARGET = emu-aarch64
TEST_TARGET = emu-test
TEST_GENDATA = emu-testgen

all: $(TARGET)
test: $(TEST_TARGET) $(TEST_GENDATA)

$(TARGET): $(OBJ) main.o
	$(CXX) -o $@ $^ $(LDFLAGS)

$(TEST_TARGET): $(OBJ) $(TEST_OBJ)
	$(CXX) -o $@ $^ $(LDFLAGS_TEST)

$(TEST_GENDATA): tests/create_testdata.o tests/as.o
	$(CXX) -o $@ $^ $(LDFLAGS)
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
	rm -f $(TARGET) $(TEST_TARGET) $(TEST_GENDATA) $(OBJ) $(TEST_OBJ) $(DEP) main.o main.d $(BIN) tests/tmp.o tmp.o

.PHONY: all test run run-test format clean

.SECONDARY: $(OBJ)
