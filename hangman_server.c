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
#define MAX_MSG_SIZE 300


typedef struct worker_thread {
	int done;
	int socket_fd;
	pthread_t tid;
	char word[256];
	char actual_word[256];
	char incorrect[6];
} WorkerThread;

void error(const char *msg)
{
	perror(msg);
	exit(1);
}

// Run by each worker thread to handle playing game with a single client
void* handle_client(){
	printf("STUB\n");
	return NULL;
}

void send_string_msg(int msg_len, char *msg){
	printf("STUB\n");
}

void send_control_msg(int word_len, int num_correct, char *word, char *incorrect){
	printf("STUB\n");
}

// Data structures for worker management and synchronization
WorkerThread workers[MAX_CLIENTS];
pthread_mutex_t lock;
#define LOCK_MUTEX pthread_mutex_lock(&lock)
#define UNLOCK_MUTEX pthread_mutex_unlock(&lock)

int main(int argc, char *argv[]){

	// Set up vars for server socket and connection socket
	int sockfd, newsockfd, portno;
	socklen_t clilen;
	struct sockaddr_in serv_addr, cli_addr;

	int n;

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

	while(1){  // Loop to continuously accept clients after one disconnects

		// Accept a new client to receive strings from
		newsockfd = accept(sockfd, 
					(struct sockaddr *) &cli_addr, 
					&clilen);
		if (newsockfd < 0) 
			error("ERROR on accept");

		int found_thread = 0;
		for(int i = 0; i < MAX_CLIENTS; ++i){  // Find an available thread
			LOCK_MUTEX;
			if(workers[i].done){  // This thread can accept a client
				pthread_join(workers[i].tid, NULL);  // Clean up old thread
				pthread_create(&workers[i].tid, NULL, handle_client, NULL);  // Start new thread
				workers[i].done = 0;  // Not done
				workers[i].socket_fd = newsockfd;  // For talking to client
				memset(workers[i].word, 0, 256);  // Clear old word
				memset(workers[i].actual_word, 0, 256);  // Clear old word
				memset(workers[i].incorrect, 0, 6);
			}
			UNLOCK_MUTEX;
		}
		if(!found_thread){  // Currently all threads are busy
			char *msg = "server-overloaded";
			send_string_msg(strlen(msg), msg);
			close(newsockfd);
		}

		//n = read(newsockfd,buffer,BUF_SIZE);
		//write(newsockfd, msg, BUF_SIZE);
	}
	return 0;
}