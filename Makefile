
BIN_NAME = jauler-lsdup

#Compiler flags
CFLAGS = -Werror -pedantic -Wall -std=c99
LFLAGS = $(CFLAGS)

#List files for dependencies specification
BUILD_DIR = build
C_FILES = $(wildcard *.c)
O_FILES = $(addprefix build/,$(notdir $(C_FILES:.c=.o)))

.PHONY: all clean
.SECONDARY: build

all: pre-build build post-build


#Building
pre-build:
	mkdir -p $(BUILD_DIR)

build: $(O_FILES)
	gcc $(LFLAGS) -o $(BIN_NAME) $^

build/%.o: %.c
	gcc $(CFLAGS) -c -o $@ $<

post-build:
	@echo "====== Build Succeded ======"
	size -A $(BIN_NAME)


clean:
	rm -rf $(BUILD_DIR)
	rm -f $(BIN_NAME)

