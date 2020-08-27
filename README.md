# Hangman Game

- **Description** : Classic Hangman Game with network capabilities. The game is designed to be played over the network by having a client instance connect to a server instance. A client and server implementation are provided to get the game up and running. The server selects a random word from a list of words and provides the length of the word to the client. The client then can send individual characters to try and guess the word selected by the server. 

The game ends after either: The client (user) guesses the correct word or the number of attempts made by the user exceeds the maximum allowed (the user loses).
___
- **Relevant Area** : Computer Networking
___
- **Tools / Platforms**: C, Unix
___
- **Installation Instructions**:  
1. Clone repo
2. Using Makefile, on a unix terminal type: `make all`
3. Run the server binary either on same computer or another computer using following format: `./hangman_server PORT_NUM` where PORT_NUM is the port you want the server to bind to.
4. Run the client binary using the following format: `./hangman_client HOST_NAME PORT_NUM` where HOST_NAME is the IP address or domain name where the server is running, and PORT_NUM is the port number the server listens on.
5. Play Hangman.
___
