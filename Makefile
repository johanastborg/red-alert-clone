CXX = clang++
CXXFLAGS = -std=c++17 -Wall -Wextra -Wno-deprecated-declarations -O2 $(shell pkg-config --cflags glfw3 glm) -Isrc
LDFLAGS = $(shell pkg-config --libs glfw3) -framework OpenGL -framework OpenAL

SRC_DIR = src
OBJ_DIR = build/obj
TARGET = redalert

SRCS = $(wildcard $(SRC_DIR)/*.cpp)
OBJS = $(patsubst $(SRC_DIR)/%.cpp, $(OBJ_DIR)/%.o, $(SRCS))

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(OBJS) -o $(TARGET) $(LDFLAGS)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp | $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

clean:
	rm -rf build $(TARGET)

run: $(TARGET)
	./$(TARGET)

.PHONY: all clean run
