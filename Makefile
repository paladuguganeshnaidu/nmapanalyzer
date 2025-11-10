CC = gcc
CFLAGS = -Wall -Wextra -std=c99
LIBS = -lws2_32

# Directories
SRC_DIR = src
INCLUDE_DIR = include
BUILD_DIR = build

# Source files
SOURCES = $(SRC_DIR)/scanner.c $(SRC_DIR)/server.c $(SRC_DIR)/utils.c
OBJECTS = $(SOURCES:$(SRC_DIR)/%.c=$(BUILD_DIR)/%.o)

# Target executable
TARGET = nmap_scanner

# Default target
all: $(TARGET)

# Link the executable
$(TARGET): $(OBJECTS)
	$(CC) $(OBJECTS) -o $(TARGET) $(LIBS)

# Compile source files
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -I$(INCLUDE_DIR) -c $< -o $@

# Clean build files
clean:
	del /q $(BUILD_DIR)\* *.exe

# Run the server
run: $(TARGET)
	./$(TARGET)

.PHONY: all clean run