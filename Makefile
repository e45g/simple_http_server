all:
	@gcc server.c routes.c utils.c -o server

clean:
	rm server
