# Compiler settings
CC = gcc
CFLAGS = -Wall -Wextra -std=gnu99 -g -O3
LDFLAGS = -pthread

# Directories
SRC_DIR = src
BUILD_DIR = build
INCLUDE_DIR = include

# Libraries
LIBS = -lsqlite3 -ljson-c -lmicrohttpd -luuid -lssl -lcrypto

# Source files
SRCS = $(wildcard $(SRC_DIR)/*.c)
OBJS = $(SRCS:$(SRC_DIR)/%.c=$(BUILD_DIR)/%.o)

# Target executable
TARGET = openimagemirror

# Include directories
INCLUDES = -I$(INCLUDE_DIR) -I/usr/include/json-c

# Default target
all: $(BUILD_DIR)/$(TARGET)

# Linking
$(BUILD_DIR)/$(TARGET): $(OBJS)
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^ $(LIBS)

# Compiling
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) $(INCLUDES) -c -o $@ $<

# Clean up
clean:
	rm -rf $(BUILD_DIR)

# Install
install: $(BUILD_DIR)/$(TARGET)
	@echo "Installing OpenImageMirror server..."
	@sudo mkdir -p /etc/openimagemirror-server
	@sudo mkdir -p /etc/openimagemirror-server/config
	@sudo mkdir -p /var/cache/openimagemirror-server
	@sudo cp $(BUILD_DIR)/$(TARGET) /etc/openimagemirror-server/
	@sudo chmod +x /etc/openimagemirror-server/$(TARGET)
	@sudo cp config/config.json /etc/openimagemirror-server/config/ -R
	@echo "Installation complete."
	@echo "Service file provided here, please install it manually if needed."

# Uninstall
uninstall:
	@echo "Uninstalling ISO Server..."
	@sudo rm -f /etc/openimagemirror-server/$(TARGET)
	@sudo rm -rf /var/log/openimagemirror-server.log
	@sudo rm -rf /var/cache/openimagemirror-server
	@echo "Uninstallation complete."

# Phony targets
.PHONY: all clean install uninstall service

# Dependency tracking
$(BUILD_DIR)/main.o: $(SRC_DIR)/main.c $(INCLUDE_DIR)/config.h $(INCLUDE_DIR)/api.h
$(BUILD_DIR)/api.o: $(SRC_DIR)/api.c $(INCLUDE_DIR)/api.h
$(BUILD_DIR)/config.o: $(SRC_DIR)/config.c $(INCLUDE_DIR)/config.h
$(BUILD_DIR)/cache.o: $(SRC_DIR)/cache.c $(INCLUDE_DIR)/cache.h
$(BUILD_DIR)/iso_manager.o: $(SRC_DIR)/iso_manager.c $(INCLUDE_DIR)/iso_manager.h
$(BUILD_DIR)/utils.o: $(SRC_DIR)/utils.c $(INCLUDE_DIR)/utils.h