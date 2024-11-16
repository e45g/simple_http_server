CC = gcc
CFLAGS = -Wall -Wextra -Isrc -Isrc/lib/cJSON -Isrc/cxc -lsqlite3
SRC_DIR = src
LIB_DIR = src/lib/cJSON
SRCS = $(wildcard $(SRC_DIR)/*.c) $(wildcard $(LIB_DIR)/*.c) $(wildcard $(SRC_DIR)/*/*.c)

SERVER = server
CXC = cxc

$(SERVER): $(SRCS)
	@make clean
	@make cxc
	@./$(CXC) && $(CC) $(CFLAGS) -o $@ $^

clean:
	@rm -f $(SERVER)
	@rm -f $(CXC)

cxc:
	@$(CC) $(CFLAGS) -o $(CXC) src_cxc/main.c

dev-serve:
	@docker exec -it simple-http-server sh -c "cd /home/dev && make && ./server"

dev-startup:
	@docker run -v "./:/home/dev/" -p 3000:3000 -d -t --name simple-http-server gcc:latest
