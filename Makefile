CXX = g++
CXXFLAGS = -std=c++17 -O2 -Wall -Wextra -Wpedantic -Iinclude
TARGET = compiler.exe

SRCS = src/main.cpp \
       src/lexer.cpp \
       src/parser.cpp \
       src/ir_generator.cpp \
       src/constant_folding.cpp \
       src/constant_propagation.cpp \
       src/dead_code_elimination.cpp \
       src/interpreter.cpp \
       src/json_output.cpp

all: $(TARGET)

$(TARGET): $(SRCS)
	$(CXX) $(CXXFLAGS) $(SRCS) -o $(TARGET)

test: $(TARGET)
	python tests/run_tests.py

web: $(TARGET)
	python web/server.py

clean:
	rm -f $(TARGET)

.PHONY: all test web clean
