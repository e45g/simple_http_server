CC = gcc
CFLAGS = -Wall -Wextra -Isrc -Isrc/lib/cJSON
SRC_DIR = src
LIB_DIR = src/lib/cJSON
SRCS = $(wildcard $(SRC_DIR)/**/*.c) $(wildcard $(LIB_DIR)/**/*.c)
HDRS = $(wildcard $(SRC_DIR)/**/*.h) $(wildcard $(LIB_DIR)/**/*.h)

TARGET = server

$(TARGET): $(SRCS)
	echo $^
	$(CC) $(CFLAGS) -o $@ $^
#bun run packages/cxc/src/index.ts

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

