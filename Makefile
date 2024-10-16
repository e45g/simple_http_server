all:
	@gcc server.c routes.c utils.c -o server
	@./server

clean:
	rm server
