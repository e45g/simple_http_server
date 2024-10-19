CC = gcc
CFLAGS = -Wall -Wextra -Iserver -Iserver/lib/cJSON
SRC_DIR = server_src
LIB_DIR = server_src/lib/cJSON
SRCS = $(wildcard $(SRC_DIR)/*.c) $(wildcard $(LIB_DIR)/*.c)
HDRS = $(wildcard $(SRC_DIR)/*.h) $(wildcard $(LIB_DIR)/*.h)

TARGET = server

$(TARGET): $(SRCS)
	echo $^
	$(CC) $(CFLAGS) -o $@ $^

clean:
	rm -f $(TARGET)
