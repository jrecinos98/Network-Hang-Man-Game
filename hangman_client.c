/* Note: The boilerplate socket code for this program 
 * (setting up the socket/sockaddr_in as a UDP client socket and connecting)
 * was taken from https://www.cs.cmu.edu/afs/cs/academic/class/15213-f99/www/class26/udpclient.c
 * as was suggested in section/from the section slides
 * Original author unknown
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <strings.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 
#include <ctype.h>

#define IPv4 AF_INET
#define SERVER_TYPE SOCK_STREAM
#define MAX 17
#define MAX_INCORRECT 6
#define CLIENT_MSG_SIZE 2

void error(const char *msg)
{
    perror(msg);
    exit(1);
}

int is_string(char * buf){
     char *end;
     strtol(buf, &end,10);
     return end == buf;
}
int set_addr(struct sockaddr_in *address, int portNum, char ip_addr[]){
    struct hostent *hp = gethostbyname(ip_addr);
    if(hp == 0){
        return -1;
    }
    address->sin_family= IPv4;
    //htons() used to make sure little endian machines will pass in the correct port number
    address->sin_port= htons(portNum);
    //Will get assigned to the server adress
    bcopy((char*) hp->h_addr, (char*) &address->sin_addr.s_addr, hp->h_length) ;
    return 0;
}
void get_data(char *buffer,char* data, int word_len){
	bzero(data, word_len);
	if(word_len > 0){
		for(int i=0; i < word_len;i++){
			data[i]= buffer[i+3]; 
		}
	}
}
void get_incorrect(char *buffer, char* incorrect){
	bzero(incorrect, MAX_INCORRECT);
	for(int i=0; i < buffer[1];i++){
		incorrect[i]= buffer[i+buffer[2]+3]; 
	}
}

int main(int argc, char *argv[]){
	int sockfd;
    struct sockaddr_in serv_addr;
    char buffer[MAX];
    bzero(buffer,MAX);
    if (argc < 3) {
       fprintf(stderr,"USAGE: %s hostname port\n", argv[0]);
       exit(0);
    }
    sockfd = socket(IPv4, SERVER_TYPE, 0);
    if (sockfd < 0) 
        error("ERROR opening socket");
    bzero((char *) &serv_addr, sizeof(serv_addr));
    //Set up serv_addr
    if (set_addr(&serv_addr, atoi(argv[2]), argv[1]) < 0) {
        fprintf(stderr,"ERROR, no such host\n");
        exit(0);
    }
    //Bind socket to the given port and address
    if (connect(sockfd,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0) 
        error("ERROR");

    char start[MAX];

    //Ask user to initiate game
    printf("Ready to start game? (y/n)\n");
    fgets(start,MAX,stdin);
    //End the client if the user doesnt input y
    if (start[0] != 'y'){
    	return 0;
    }
    char init_msg[CLIENT_MSG_SIZE]={0};
    //Write empty message to the server to signal start of game.
    if (write(sockfd,init_msg,CLIENT_MSG_SIZE) < 0) 
        error("ERROR writing to socket");
    while(1){
        bzero(buffer,MAX);
        //Read the reply
        if (read(sockfd,buffer,MAX-1) < 0){
             error("ERROR reading from socket");
       
        }
        int msg_flag= buffer[0];
        //If msg flag is set then server sent a message to the client
        if(msg_flag){
        	printf("%s\n", buffer+1);
        	//User either won or lost. Terminate loop and kill client
        	break;
        }
        //else it is a normal game control packet
        int word_len= buffer[1];
        char data[word_len+1];
        char incorrect[MAX_INCORRECT];
        //Extract the word data from buffer
        get_data(buffer, data, word_len);
        //Extract the incorrect characters
        get_incorrect(buffer, incorrect);

        //Print info
        printf("%s\n", data);
        printf("Incorrect Guesses: %s\n\n", incorrect);

        char guess[MAX]={0};
        //Loop until 1 char input is given
        while(strlen(guess) != 1 || !is_string(guess)){
        	printf("Letter to guess: ");
       		fgets(guess,MAX, stdin);
            guess[strlen(guess)-1]='\0';
       		if(strlen(guess) !=1 || !is_string(guess)){
        		printf("Error! Please guess one letter.\n");
       			//Clear wrong values
       			bzero(guess, MAX);
       		}
        }
        //Format the message
        char message[CLIENT_MSG_SIZE]={1,tolower(guess[0])};
        //Send the message to the server
        if(write(sockfd,message, CLIENT_MSG_SIZE) < 0){
        	error("ERROR writing to socket");
        }
        
    }
    close(sockfd);
	return 0;
}