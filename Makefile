all:
	gcc -Wall -g hangman_client.c -o hangman_client 
	gcc -Wall -g hangman_server.c -lpthread  -o hangman_server 

clean:
	rm hangman_server
	rm hangman_client