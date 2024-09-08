all:
	@gcc server.c routes.c -o server
	@./server

clean:
	rm server
