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
#define MAX 50
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
	for(int i=0; i < buffer[2];i++){
		incorrect[i]= buffer[i+buffer[1]+3]; 
	}
}
int print_message(char *buffer){
    int msg_flag=buffer[0];
    char overload[17]={"server-overloaded"};
    int n=1;
    for (int i=0;i< msg_flag; i++){
        if(msg_flag>= 17){
            if(overload[i] != buffer[i+1] ){
                n=0;
            }
        }
        printf("%c",buffer[i+1]);
    }
    printf("\n");
    return n;

}


int main(int argc, char *argv[]){
	int sockfd;
    struct sockaddr_in serv_addr;
    char buffer[MAX+1];
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
    start[MAX-1]='\0';
    int valid=0;
    //End the client if the user doesnt input y
    while(!valid){
        //Ask user to initiate game
        printf("Ready to start game? (y/n): ");
        fgets(start,MAX,stdin);
        printf("\n");
        //fgets includes \n char so size == 2
        if(tolower(start[0]) == 'y' && strlen(start)==2){
            valid=1;
        }
        //fgets includes \n char so size == 2
        if(tolower(start[0]) == 'n' && strlen(start)==2){
            close(sockfd);
            return 0;
        }
    }
    char init_msg[CLIENT_MSG_SIZE+1]={0};
    init_msg[CLIENT_MSG_SIZE]='\0';
    //Write empty message to the server to signal start of game.
    if (write(sockfd,init_msg,CLIENT_MSG_SIZE) < 0) 
        error("ERROR: can't write to socket\n");
    while(1){
        memset(buffer, 0, MAX + 1);
        buffer[MAX-1]='\0';
        //Read the reply
        if (read(sockfd,buffer,MAX) <= 0){
             printf("ERROR: can't read from socket\n");   
             exit(1);    
        }
        char msg_flag= buffer[0];
        //If msg flag is set then server sent a message to the client
        if(msg_flag){
            //Reveal the word
            if(print_message(buffer)){
                break;
            }
            char *message= buffer+1;
            if (strcmp("server-overloaded", message) == 0){
                break;
            }
            memset(buffer, 0, MAX + 1);

            //Read the next message (Win or lose message)
            if (read(sockfd,buffer,MAX) <= 0){
             	printf("ERROR: can't read from socket\n");
                exit(1);
        	}
            //print win/lose message
        	print_message(buffer);
            memset(buffer, 0, MAX + 1);

            //Read the game over message
            if (read(sockfd,buffer,MAX) <= 0){
                printf("ERROR: can't read from socket\n");
                exit(1);
            }
            //print game over
            print_message(buffer);
        	//User either won or lost. Terminate loop and kill client
        	break;
        }
        //else it is a normal game control packet
        int word_len= buffer[1];
        char data[word_len+1];
        data[word_len]='\0';
        char incorrect[MAX_INCORRECT+1];
        incorrect[MAX_INCORRECT]='\0';
        //Extract the word data from buffer
        get_data(buffer, data, word_len);
        //Extract the incorrect characters
        get_incorrect(buffer, incorrect);

        //Print info
        for(int i = 0; i < word_len - 1; i++){
        	printf("%c ", data[i]);
        }
        printf("%c\n", data[word_len-1]);
        printf("Incorrect Guesses: %s\n\n", incorrect);

        char guess[MAX]={0};
        guess[MAX-1]='\0';
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
        char message[CLIENT_MSG_SIZE+1]={1,tolower(guess[0])};
        message[CLIENT_MSG_SIZE]='\0';
        //Send the message to the server
        if(write(sockfd,message, CLIENT_MSG_SIZE) < 0){
        	error("ERROR writing to socket");
        }
        
    }
    close(sockfd);
	return 0;
}