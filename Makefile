CXX := g++
CXXFLAGS := -std=c++17 -O2 -Wall -Wextra
LDFLAGS :=
EXECUTABLE := ./testing/bslc

OBJ_DIR := ./testing/objfiles
SRC_DIR := .


SRCS := $(shell find $(SRC_DIR) -type f -name '*.cpp')

OBJS := $(patsubst %.cpp,$(OBJ_DIR)/%.o,$(SRCS))

all: $(EXECUTABLE)

$(EXECUTABLE): $(OBJS)
	$(CXX) $(LDFLAGS) $^ -o $@

$(OBJ_DIR)/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

.PHONY: clean
clean:
	$(RM) -r $(OBJ_DIR) $(EXECUTABLE)
