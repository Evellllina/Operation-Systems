CXX      = g++
CXXFLAGS = -std=c++17 -O2 -Wall -Wextra -pthread -Iinclude

TARGET = lab3
SRC    = src/main.cpp src/timsort.cpp src/parallel_timsort.cpp
OBJ    = $(SRC:src/%.cpp=build/%.o)

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CXX) $(CXXFLAGS) -o $@ $^

build/%.o: src/%.cpp $(wildcard include/*.hpp) | build
	$(CXX) $(CXXFLAGS) -c $< -o $@

build:
	mkdir -p build

clean:
	rm -rf build $(TARGET)

.PHONY: all clean
