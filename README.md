# C implementation of the popular Hangman Game

The game is meant to be played over the network by having a client instance connect to a server instance. A client and server implementation are provided to get the game up and running. The server selects a random word from a list of words and provides the length of the word to the client. The client then can send individual characters to try and guess the word selected by the server. 

The game ends after either: The client (user) guesses the correct word or the number of attempts made by the user exceeds the maximum allowed (the user loses).
