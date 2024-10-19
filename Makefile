CC = gcc
CFLAGS = -Wall -Wextra -Isrc -Isrc/lib/cJSON -Isrc/tpl
SRC_DIR = src
LIB_DIR = src/lib/cJSON
SRCS = $(wildcard $(SRC_DIR)/*.c) $(wildcard $(LIB_DIR)/*.c) $(wildcard src/tpl/*.c)
HDRS = $(wildcard $(SRC_DIR)/*.h) $(wildcard $(LIB_DIR)/*.h)

TARGET = server

$(TARGET): $(SRCS)
	make clean
	bun run packages/cxc/src/index.ts
	$(CC) $(CFLAGS) -o $@ $^

clean:
	rm -f $(TARGET)

cxc:
	bun run packages/cxc/src/index.ts

dev-serve:
	@docker exec -it simple-http-server sh -c "cd /home/dev && make && ./server"

dev-startup:
	@docker run -v "./:/home/dev/" -p 3000:3000 -d -t --name simple-http-server gcc:latest

dev-install:
	@docker exec -it simple-http-server sh -c "curl -fsSL https://bun.sh/install | bash"

