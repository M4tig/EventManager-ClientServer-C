# Compiler and flags
CC = gcc
CFLAGS = -Wall -Wextra -Werror -std=c11 -I./src
LDFLAGS = 

# Directories
SRC_DIR = src
OBJ_DIR = obj
BIN_DIR = .

# Source files
SRCS = $(SRC_DIR)/client.c $(SRC_DIR)/commands.c $(SRC_DIR)/protocol.c
OBJS = $(SRCS:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o)

# Target executable
TARGET = $(BIN_DIR)/user

# Default target
all: $(TARGET)

# Create object directory if it doesn't exist
$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

# Link object files to create executable
$(TARGET): $(OBJS)
	$(CC) $(OBJS) $(LDFLAGS) -o $(TARGET)
	@echo "Build successful: $(TARGET)"

# Compile source files to object files
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Clean build artifacts
clean:
	rm -rf $(OBJ_DIR)
	rm -f $(TARGET)
	@echo "Clean complete"

# Rebuild everything
rebuild: clean all

# Run the client
run: $(TARGET)
	./$(TARGET)

# Debug build with symbols
debug: CFLAGS += -g -O0 -DDEBUG
debug: clean $(TARGET)

# Release build with optimizations
release: CFLAGS += -O2 -DNDEBUG
release: clean $(TARGET)

.PHONY: all clean rebuild run debug release
