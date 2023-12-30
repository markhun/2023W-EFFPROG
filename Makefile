# Makefile settings
CC      = cc
RM      = rm -f
CFLAGS	=-Wall -O3 -DNDEBUG -march=native -fwhole-program
# CFLAGS =-Wall -ggdb -O0 -fno-omit-frame-pointer 
LD		= cc
LDFLAGS	=-g

# Project specific settings
SRC_DIR		= ./magichex/src
TESTS_DIR 	= ./magichex/tests
BUILD_DIR   = ./build
BIN_DIR     = ./bin
SRC_LIST = $(wildcard $(SRC_DIR)/*.c)
OBJ_LIST = $(BUILD_DIR)/$(notdir $(SRC_LIST:.c=.o))

.PHONY: all clean directories test test1 test2 test3 profile profile-nocheck

default: all

all: magichex

magichex: magichex.o directories
	$(LD) $(LDFLAGS) $(OBJ_LIST) -o $(BIN_DIR)/$@

magichex.o: $(SRC_DIR)/magichex.c directories
	$(CC) $(CFLAGS) -c $(SRC_DIR)/magichex.c -o $(BUILD_DIR)/$@


directories:
	@mkdir -p $(BUILD_DIR)
	@mkdir -p $(BIN_DIR)

test: test1 test2 test3

test1: magichex
	@echo "Running test1: $(BIN_DIR)/magichex 4 3 14 33 30 34 39 6 24 20"
	@$(BIN_DIR)/magichex 4 3 14 33 30 34 39 6 24 20 |\
	grep -v solution| \
	awk '/^leafs visited:/ {printf("\0")} /^leafs visited:/,/^$$/ {next} 1'|\
	sort -z|\
	tr '\0' '\n\n' |\
	diff --color -u $(TESTS_DIR)/reference-output - \
	&& echo "-------- TEST1 SUCCESSFUL! --------" \
	|| echo "-------- TEST1 FAILED! --------"

test2: magichex
	@echo "Running test2: $(BIN_DIR)/magichex 3 2"
	@$(BIN_DIR)/magichex 3 2 |\
	grep -v solution| \
	awk '/^leafs visited:/ {printf("\0")} /^leafs visited:/,/^$$/ {next} 1'|\
	sort -z|\
	tr '\0' '\n\n' |\
	diff --color -u $(TESTS_DIR)/reference-output-3-2 - \
	&& echo "-------- TEST2 SUCCESSFUL! --------" \
	|| echo "-------- TEST2 FAILED! --------"

test3: magichex
	@echo "Running test3: $(BIN_DIR)/magichex 3 0"
	@$(BIN_DIR)/magichex 3 0 |\
	grep -v solution| \
	awk '/^leafs visited:/ {printf("\0")} /^leafs visited:/,/^$$/ {next} 1'|\
	sort -z|\
	tr '\0' '\n\n' |\
	diff --color -u $(TESTS_DIR)/reference-output-3-0 - \
	&& echo "-------- TEST3 SUCCESSFUL! --------" \
	|| echo "-------- TEST3 FAILED! --------"

profile-with-test: magichex
	perf stat -e cycles:u -e instructions:u -e branches:u -e branch-misses:u -e L1-dcache-load-misses:u $(BIN_DIR)/magichex 4 3 14 33 30 34 39 6 24 20 |\
	grep -v solution| \
	awk '/^leafs visited:/ {printf("\0")} /^leafs visited:/,/^$$/ {next} 1'|\
	sort -z|\
	tr '\0' '\n\n' |\
	diff --color -u $(TESTS_DIR)/reference-output - \
	&& echo "-------- TEST SUCCESSFUL! --------" \
	|| echo "-------- TEST FAILED! --------"

benchmark: magichex
	perf stat -r 5 -d $(BIN_DIR)/magichex 4 3 14 33 30 34 39 6 24 20

profile: magichex
	perf stat -e cycles:u -e instructions:u -e branches:u -e branch-misses:u -e L1-dcache-load-misses:u $(BIN_DIR)/magichex 4 3 14 33 30 34 39 6 24 20

clean:
	rm -f $(BIN_DIR)/magichex $(BUILD_DIR)/*.o
