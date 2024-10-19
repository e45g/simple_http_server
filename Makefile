CC = gcc
CFLAGS = -Wall -Wextra -Isrc -Isrc/lib/cJSON
SRC_DIR = src
LIB_DIR = src/lib/cJSON
SRCS = $(wildcard $(SRC_DIR)/*.c) $(wildcard $(LIB_DIR)/*.c)
HDRS = $(wildcard $(SRC_DIR)/*.h) $(wildcard $(LIB_DIR)/*.h)

TARGET = server

$(TARGET): $(SRCS)
	echo $^
	$(CC) $(CFLAGS) -o $@ $^

clean:
	rm -f $(TARGET)

cxc: 
	bun run packages/cxc/src/index.ts