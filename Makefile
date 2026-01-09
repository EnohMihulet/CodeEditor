SRC_DIR  := src
BUILD    := build
BIN_DIR  := bin
APP      := editor

SOURCES  := $(wildcard $(SRC_DIR)/*.cpp)
OBJECTS  := $(patsubst $(SRC_DIR)/%.cpp,$(BUILD)/%.o,$(SOURCES))
DEPS     := $(OBJECTS:.o=.d)

CC       := clang++
WARNINGS := -Wall -Wextra -Wconversion -Wno-unused-parameter -Wno-sign-conversion
DEPFLAGS := -MMD -MP
CFLAGS   := -std=c++23 -g -O0 -DDEBUG=1 $(WARNINGS) $(DEPFLAGS)

CFLAGS  += -I/opt/homebrew/include/SDL2 -D_THREAD_SAFE
LDFLAGS += -L/opt/homebrew/lib -lSDL2

.PHONY: all run clean
all: $(BIN_DIR)/$(APP)

$(BIN_DIR)/$(APP): $(OBJECTS) | $(BIN_DIR)
	$(CC) $(OBJECTS) -o $@ $(LDFLAGS)

$(BUILD)/%.o: $(SRC_DIR)/%.cpp | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@

run: all
	./$(BIN_DIR)/$(APP)

$(BUILD) $(BIN_DIR):
	@mkdir -p $@

clean:
	@rm -f $(OBJECTS) $(DEPS) $(BIN_DIR)/$(APP)

-include $(DEPS)
