all:
	gcc community_server.c -o p2 -lm -lpthread
clean:
	rm -f p2
