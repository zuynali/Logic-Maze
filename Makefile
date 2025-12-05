# Makefile for Logic Maze Database - Phase 1

CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -O2 -g -Iinclude
LDFLAGS = -pthread

# Source files
SRC_DIR = src
SOURCES = $(SRC_DIR)/disk_manager.cpp $(SRC_DIR)/lru_replacer.cpp $(SRC_DIR)/buffer_pool_manager.cpp
OBJECTS = $(SOURCES:.cpp=.o)

# Test executable
TEST_TARGET = test_phase1
TEST_SOURCE = test/test_phase1.cpp

# Default target
all: $(TEST_TARGET)

# Build test executable
$(TEST_TARGET): $(OBJECTS) $(TEST_SOURCE)
	$(CXX) $(CXXFLAGS) $(OBJECTS) $(TEST_SOURCE) -o $(TEST_TARGET) $(LDFLAGS)

# Compile source files
$(SRC_DIR)/%.o: $(SRC_DIR)/%.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Clean build files
clean:
	rm -f $(OBJECTS) $(TEST_TARGET) *.db

# Run tests
test: $(TEST_TARGET)
	./$(TEST_TARGET)

# Check for memory leaks with valgrind
memcheck: $(TEST_TARGET)
	valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes ./$(TEST_TARGET)

# Show help
help:
	@echo "Logic Maze Database - Phase 1 Build System"
	@echo ""
	@echo "Targets:"
	@echo "  make          - Build the test executable"
	@echo "  make test     - Build and run tests"
	@echo "  make clean    - Remove build files and database files"
	@echo "  make memcheck - Run with valgrind memory checker"
	@echo "  make help     - Show this help message"

.PHONY: all clean test memcheck help