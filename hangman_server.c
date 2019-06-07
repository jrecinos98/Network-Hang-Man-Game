/* Note: The boilerplate socket code for this program 
 * (setting up the socket/sockaddr_in as a UDP client socket and connecting)
 * was taken from https://www.cs.cmu.edu/afs/cs/academic/class/15213-f99/www/class26/udpclient.c
 * as was suggested in section/from the section slides
 * Original author unknown
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <ctype.h>
#include <pthread.h>
#include <time.h>

#define MAX_CLIENTS 3  // Max number of clients that can be simultaneously connected
#define MAX_MSG_SIZE 17
#define MAX_STR_MSG_SIZE 50
#define MAX_WORD_SIZE 8
#define CLI_MSG_SIZE 2
#define MAX_INCORRECT 6
#define MAX_WORDS_FILE 15
#define TEXT_FILE "hangman_words.txt"


typedef struct worker_thread {
	int done;
	int socket_fd;
	int initialized;
	pthread_t tid;

} WorkerThread;

// Data structures for worker management and synchronization
WorkerThread workers[MAX_CLIENTS];
pthread_mutex_t lock;
#define LOCK_MUTEX pthread_mutex_lock(&lock)
#define UNLOCK_MUTEX pthread_mutex_unlock(&lock)

void error(const char *msg)
{
	perror(msg);

}

// Set up initial worker states
void init_workers(WorkerThread workers[]){
	for(int i = 0; i < MAX_CLIENTS; i++){
		workers[i].done = 1;
		workers[i].socket_fd = -1;
		workers[i].tid = -1;
		workers[i].initialized = 0;
	}
}

// Read input from hangman text file and place selected word in word
void pick_from_file(char *word){
	int num_words = 0;
	char **file_words = malloc(MAX_WORDS_FILE * sizeof(char*));
	FILE *fp;
	fp = fopen(TEXT_FILE, "r");
	if(fp == NULL){
	}
	file_words[num_words] = malloc(MAX_WORD_SIZE + 2);
	while ( num_words < MAX_WORDS_FILE && fgets(file_words[num_words], MAX_WORD_SIZE + 2, fp)) {
		if(file_words[num_words][strlen(file_words[num_words])-1] == '\n'){
			file_words[num_words][strlen(file_words[num_words])-1] = '\0';
		}
		if(strlen(file_words[num_words]) != 0){  // Skip empty lines
			if(num_words < MAX_WORDS_FILE - 1){
	    	    num_words++;
		        file_words[num_words] = malloc(MAX_WORD_SIZE + 2);
			}else{
				num_words++;
			}	
		}
		
    }
    fclose(fp);
    memset(word, 0, MAX_WORD_SIZE);
    while(strlen(word) == 0){
	    int pick_word = rand() % num_words;

		strcpy(word, file_words[pick_word]);
    }
    

	for(int i = 0; i < num_words; i++){
		free(file_words[i]);
	}
	free(file_words);
}

// Send a string only message
void send_string_msg(int client_fd, int msg_len, char *msg){
	char str_msg[MAX_STR_MSG_SIZE];
	str_msg[0] = msg_len;
	for(int i = 0; i < msg_len; i++){
		str_msg[i+1] = msg[i];
	}
	write(client_fd, str_msg, MAX_STR_MSG_SIZE);
}

// Send a game control message with proper fields
void send_control_msg(int client_fd, int word_len, int num_incorrect, char *word, char *incorrect){
	char cntl_msg[MAX_MSG_SIZE];
	cntl_msg[0] = 0;
	cntl_msg[1] = word_len;
	cntl_msg[2] = num_incorrect;
	for(int i = 0; i < word_len; i++){
		cntl_msg[i+3] = word[i];
	}
	for(int i = 0; i < num_incorrect; i++){
		cntl_msg[i + 3 + word_len] = incorrect[i];
	}

	write(client_fd, cntl_msg, MAX_MSG_SIZE);

}
//Forats the message to reveal to the user when game is over
void word_message(char *message, char* actual_word){
	memset(message, 0, MAX_STR_MSG_SIZE+1);
	const char* msg= "The word was";
	char spc_word[2*strlen(actual_word)];
	memset(spc_word,0, 2*strlen(actual_word));
	int odd=1;
	int even=0;
	for(int i=0; i < strlen(actual_word);i++){
		spc_word[even]=' ';
		spc_word[odd]= actual_word[i];
		even+=2;
		odd+=2;
	}
	spc_word[2*strlen(actual_word)]='\0';
	//copy the const string
	strcpy(message, msg);
	//copy the spaced word
	strcat(message,spc_word);



}
// Run by each worker thread to handle playing game with a single client
void* handle_client(void *arg){
	char word[MAX_WORD_SIZE + 1];
	char actual_word[MAX_WORD_SIZE + 1];
	char incorrect[MAX_INCORRECT + 1];
	memset(incorrect, 0, MAX_INCORRECT + 1);
	char cli_msg[CLI_MSG_SIZE];
	int num_correct = 0;
	int num_incorrect = 0;
	int done = 0;

	int worker_num = *((int*)arg);
	LOCK_MUTEX;
	int client_fd = workers[worker_num].socket_fd;
	UNLOCK_MUTEX;
	free(arg);

	pick_from_file(actual_word);

	for(int i = 0; i < strlen(actual_word); ++i){
		word[i] = '_';  // Set up empty word
	}
	word[strlen(actual_word)] = '\0';


	// Wait for client start message:
	int n = read(client_fd,cli_msg,CLI_MSG_SIZE);
	if(n <= 0 || cli_msg[0] != 0){
		close(client_fd);
		done = 1;
	}

	
	while(!done){

		send_control_msg(client_fd, strlen(word), strlen(incorrect), word, incorrect);

		n = read(client_fd,cli_msg,CLI_MSG_SIZE);

		if(n <= 0){  // Client disconnected
			break;
		}
		if(cli_msg[0] == 1){  // Client sent a guess
			int num_changed = 0;
			for(int i = 0; i < strlen(actual_word); i++){
				if(tolower(actual_word[i]) == tolower(cli_msg[1])
					&& tolower(cli_msg[1] != word[i])){  // Make sure not guessing same word again
					word[i] = tolower(cli_msg[1]);
					num_changed++;
				}
			}
			num_correct += num_changed;
			if(num_correct == strlen(actual_word)){  // Client won
				
				char word_msg[MAX_STR_MSG_SIZE+1];
				//Format string that reveals word
				word_message(word_msg, actual_word);
				send_string_msg(client_fd, strlen(word_msg),word_msg);
				send_string_msg(client_fd, 8, "You Win!");
				send_string_msg(client_fd, 10, "Game Over!");

				done = 1;
			}
			if(num_changed == 0){
				int already_guessed = 0;
				for(int i = 0; i < num_incorrect; i++){
					if(tolower(incorrect[i]) == tolower(cli_msg[1])){
						already_guessed = 1;
					}
				}
				for(int i = 0; i < strlen(actual_word); i++){
					if(word[i] == cli_msg[1]){
						already_guessed = 1;
					}
				}
				if(!already_guessed){  // Check if client has guessed this before
					num_incorrect++;
					incorrect[num_incorrect-1] = toupper(cli_msg[1]);
				}
				if(num_incorrect == MAX_INCORRECT){  // Client lost

					char word_msg[MAX_STR_MSG_SIZE+1];
					//Format string that reveals word
					word_message(word_msg, actual_word);
					send_string_msg(client_fd, strlen(word_msg),word_msg);
					send_string_msg(client_fd, 9, "You Lose.\0");
					send_string_msg(client_fd, 10, "Game Over!\0");
					
					done = 1;
				}
			}
		}
	}

	fflush(stdout);
	close(client_fd);
	workers[worker_num].done = 1;

	pthread_exit(NULL);
}



int main(int argc, char *argv[]){
	srand(time(0));
	// Set up vars for server socket and connection socket
	int sockfd, newsockfd, portno;
	socklen_t clilen;
	struct sockaddr_in serv_addr, cli_addr;

	// Handle bad usage
	if (argc < 2) {
		fprintf(stderr,"ERROR, no port provided\n");
		exit(1);
	}

	// Create the TCP socket
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0) 
		error("ERROR opening socket");

	// Set up the servers socket
	memset((char *) &serv_addr, 0, sizeof(serv_addr));
	portno = atoi(argv[1]);
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = INADDR_ANY;  // Bind to all ip's
	serv_addr.sin_port = htons(portno);

	// Bind to port so same port is always used
	if (bind(sockfd, (struct sockaddr *) &serv_addr,
			 sizeof(serv_addr)) < 0) {
			error("ERROR on binding");
			exit(1);
		}

	// Listen for any incoming connections
	listen(sockfd,5);
	clilen = sizeof(cli_addr);

	init_workers(workers);

	while(1){  // Loop to continuously accept clients after one disconnects
		// Accept a new client to receive strings from
		newsockfd = accept(sockfd, 
					(struct sockaddr *) &cli_addr, 
					&clilen);
		if (newsockfd < 0) 
			error("ERROR on accept");

		int found_thread = 0;
		for(int i = 0; i < MAX_CLIENTS && !found_thread; ++i){  // Find an available thread
			LOCK_MUTEX;
			if(workers[i].done){  // This thread can accept a client
				if(workers[i].initialized){
					pthread_join(workers[i].tid, NULL);  // Clean up old thread
				}
				int *worker_num = malloc(sizeof(int));
				*worker_num = found_thread = i;
				pthread_create(&workers[i].tid, NULL, handle_client, worker_num);  // Start new thread
				workers[i].done = 0;  // Not done
				workers[i].socket_fd = newsockfd;  // For talking to client
				workers[i].initialized = 1;  // Needs to be cleaned on exit
				found_thread = 1;
			}
			UNLOCK_MUTEX;
		}
		if(!found_thread){  // Currently all threads are busy
			char *msg = "server-overloaded";
			send_string_msg(newsockfd, strlen(msg), msg);
			close(newsockfd);
		}
	}
	return 0;
}