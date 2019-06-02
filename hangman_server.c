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

#define MAX_CLIENTS 3  // Max number of clients that can be simultaneously connected
#define MAX_MSG_SIZE 17
#define MAX_WORD_SIZE 8
#define CLI_MSG_SIZE 2
#define MAX_INCORRECT 6


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
	exit(1);
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
	printf("pick_from_file(): STUB\n");
	strcpy(word, "STUBWORD\0");
}

// Send a string only message
void send_string_msg(int msg_len, char *msg){
	printf("send_string_msg(): STUB\n");
}

// Send a game control message with proper fields
void send_control_msg(int client_fd, int word_len, int num_incorrect, char *word, char *incorrect){
	printf("send_control_msg(): STUB\n");

}

// Run by each worker thread to handle playing game with a single client
void* handle_client(void *arg){
	char word[MAX_WORD_SIZE + 1];
	char actual_word[MAX_WORD_SIZE + 1];
	char incorrect[MAX_INCORRECT + 1];
	memset(incorrect, 0, MAX_INCORRECT + 1);
	char cli_msg[CLI_MSG_SIZE];
	int num_correct = 0;
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

	printf("handle_client(): %s w/ len = %d \n", word, strlen(word));

	// Wait for client start message:
	int n = read(client_fd,cli_msg,CLI_MSG_SIZE);
	if(n <= 0 || cli_msg[0] != 0){
		fprintf(stderr, "ERROR: Improper client start message\n");
		close(client_fd);
		return NULL;
	}

	printf("handle_client(): Received start msg -- starting game\n");
	
	while(!done){
		send_control_msg(client_fd, strlen(word), strlen(incorrect), word, incorrect);
		n = read(client_fd,cli_msg,CLI_MSG_SIZE);

		if(n <= 0){  // Client disconnected
			break;
		}

		if(cli_msg[0] == 1){  // Client sent a guess
			int num_changed = 0;
			for(int i = 0; i < strlen(actual_word); i++){
				if(tolower(actual_word[i]) == tolower(cli_msg[1])){
					word[i] = tolower(cli_msg[1]);
					num_changed++;
				}
			}
			num_correct += num_changed;
			if(num_correct == strlen(actual_word)){  // Client won
				printf("handle_client(): Client wins\n");

				done = 1;
			}
			if(num_changed == 0){
				if(strlen(incorrect) == MAX_INCORRECT - 1){  // Client lost
					printf("handle_client(): Client loses\n");

					done = 1;
				}
				printf("handle_client(): Incorrect guess\n");
				incorrect[strlen(incorrect)] = toupper(cli_msg[1]);
			}
		}
	}
	close(client_fd);
	printf("handle_client(): Exiting\n");

	return NULL;
}



int main(int argc, char *argv[]){

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
			 sizeof(serv_addr)) < 0) 
			 error("ERROR on binding");

	// Listen for any incoming connections
	listen(sockfd,5);
	clilen = sizeof(cli_addr);

	init_workers(workers);

	while(1){  // Loop to continuously accept clients after one disconnects
		// Accept a new client to receive strings from
		// newsockfd = accept(sockfd, 
		// 			(struct sockaddr *) &cli_addr, 
		// 			&clilen);
		sleep(1);
		newsockfd = 1;
		if (newsockfd < 0) 
			error("ERROR on accept");

		int found_thread = 0;
		for(int i = 0; i < MAX_CLIENTS; ++i){  // Find an available thread
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
			}
			UNLOCK_MUTEX;
		}
		if(!found_thread){  // Currently all threads are busy
			printf("Could not find open thread\n");
			char *msg = "server-overloaded";
			send_string_msg(strlen(msg), msg);
			close(newsockfd);
		}

		//n = read(newsockfd,buffer,BUF_SIZE);
		//write(newsockfd, msg, BUF_SIZE);
	}
	return 0;
}