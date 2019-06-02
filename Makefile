all:
	gcc -Wall hangman_client.c -o hangman_client 
	gcc -std=c99 -Wall hangman_server.c -lpthread  -o hangman_server 

clean:
	rm hangman_server
	rm hangman_client