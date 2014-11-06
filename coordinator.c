//Manuel Perez and Alex King ECE 151 HW4 Problem 6

// Standard libraries
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

// Socket-related libraries
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

#include <pthread.h>

#define ALLOC(num,type)    (type *)calloc(num,sizeof(*type))
#define MAX_LINE_SIZE 50
#define LISTEN_QUEUE 5
#define NUM_THREADS  10

#define VOTE_REQUEST 0
#define VOTE_COMMIT 1
#define VOTE_ABORT 2
#define GLOBAL_COMMIT 3
#define GLOBAL_ABORT 4
#define DECISION_REQUEST 5

void *ListenThread(void *qPos);
void SendThread(int, int);
int rankPorts(int numPorts);

char* port;
char* ports[200];
int ackMessages[100];
int messageTimes[100];
int queuePos, queueLen;
int pendingMessages = 0;
int socketFD;
int socketOrder[100];
int portOrder[100];
int sockets[100];
int outgoing[100];
int outPos, outLen;
int go=1;
int counter=0;
int okNeeded, voteAbort;
int rank;
int broadcasted = -1;
int myQueue[NUM_THREADS];
struct addrinfo *serverInfo;
struct addrinfo hints;
pthread_t threads[NUM_THREADS];
pthread_mutex_t mutexsum;

void print_usage(){
  fprintf(stderr, "Usage: ./gateway [port] [hostname] [port] [hostname] [port]\n");
  fprintf(stderr, "Example: ./gateway 4001 192.168.0.2 4845 92.167.0.3 4000\n\n");
}
 
void check_args(int argc, char *argv[]){
  if( argc < 4 || argc%2 != 0) {
    print_usage();
	exit(1);
  }
}


/////////////////////
// MAIN /////////////
int main(int argc, char *argv[])
{
	int rv, i, j;
	long q;

	//Used for threading
	int rc;
	long t;
  
	// Check arguments and exit if they are not well constructed
	check_args(argc, argv);

	for(i=0; i<100; i++)
	{
		ackMessages[i] = -1;
		messageTimes[i] = -1;
		portOrder[i] = -1;
		socketOrder[i] = -1;
		outgoing[i] = -1;
	}
	queueLen = 0;
	queuePos = 0;
	outLen = 0;
	outPos = 0;
	
	port=argv[1];
	for(i=0; i<argc-2; i++) {
		ports[i]=argv[i+2];
	}
	pthread_mutex_init(&mutexsum, NULL);
	
	//Create sorted array of ports
	t = (long) (argc-2)/2;
	portOrder[0] = atoi(port);
	for(i=0; i<t; i++)
	{
		portOrder[i+1] = atoi(ports[i*2+1]);
		j = i;
		while(portOrder[j+1] < portOrder[j] && j >= 0)
		{
			q = portOrder[j+1];
			portOrder[j+1] = portOrder[j];
			portOrder[j] = q;
			j--;
		}
	}
  
	// Fill the 'hints' structure
	memset( &hints, 0, sizeof(hints) );
	hints.ai_flags    = AI_PASSIVE;  // Use the local IP address
	hints.ai_family   = AF_INET;     // Use IPv4 address
	hints.ai_socktype = SOCK_STREAM; // TCP sockets
  
	// Get the address information of the server ('NULL' uses the local address )
	if ((rv = getaddrinfo( NULL , argv[1], &hints, &serverInfo )) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return 1;
	}
  
	// Make a socket (identified with a socket file descriptor)
	if ((socketFD = socket( serverInfo->ai_family, serverInfo->ai_socktype, serverInfo->ai_protocol )) == -1)
		perror("gateway: socket");
	
	// Bind to the specified port (in getaddrinfo())
	if (bind( socketFD, serverInfo->ai_addr, serverInfo->ai_addrlen ) == -1) {
		close(socketFD);
		perror("prog1: bind");
	}
		
	// This is only used to be able to reuse the same port
	int yes = 1;
	setsockopt( socketFD, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int) );
  
  	// Start to listen (only for TCP)
	if((listen(socketFD, LISTEN_QUEUE))== -1)
	{
		perror("prog1: listen");
		exit(1);
	}
		
	printf("Listening in port %s...", argv[1]);
	fflush(stdout);
	
	//initialize Queue
	for(q=0; q<NUM_THREADS; q++)
	{
		myQueue[q]=-1;
	}
  
	//Creating new thread for listening
	q=0;
	for(i=0; i<t; i++)
	{
	
		//check to see if any threads available
		go = 1;
		while(go==1) {
			for(q=0; q<NUM_THREADS; q++)
			{
				if(myQueue[q]==-1)
				{
					break;
				}
			}
			if( q==NUM_THREADS && go==1){
				sleep(1);
				continue;
			}
			go=0;
		}
	
		rc = pthread_create(&threads[q], NULL, ListenThread, (void *)q);
		if (rc){
			printf("ERROR; return code from pthread_create() is %d\n", rc);
			exit(-1);
		}
	}

	//Wait for user to hit ENTER
	printf("\nHit ENTER when all participants are up.");
	fflush(stdout);
	while(getchar() != '\n')
	{
		//wait for it...
	}


	for(i=0; i<t; i++)
	{
		// Get the address information of the server.
		if ((rv = getaddrinfo(ports[i*2], ports[i*2+1], &hints, &serverInfo)) != 0) {
			fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
			return;
		}
		// Create a socket to communicate with the server
		if ((sockets[i] = socket( serverInfo->ai_family, serverInfo->ai_socktype, serverInfo->ai_protocol )) == -1)
		perror("clientTCP: socket");
		 // Connect to the server
		if((connect(sockets[i], serverInfo->ai_addr, serverInfo->ai_addrlen)) == -1)
		{
			perror("Socket connect: connect");
		}
		
		//Put the socket in its sorted position
		for(j=0; j<100; j++)
		{
			if(portOrder[j] == atoi(ports[i*2+1]))
			{
				socketOrder[j] = sockets[i];
				break;
			}
		}
	}
	
	//Rank port numbers for time scaling
	rank = rankPorts(t);

	int caseCount = 1;
	
	while (1)
	{
		printf("\n\nCase %i\n",caseCount);
		printf("Sending VOTE_REQUEST message.\n");
		okNeeded = t;
		voteAbort = 0;
		for(i=0; i<t; i++)
		{
			SendThread(sockets[i], VOTE_REQUEST);
		}
		
		//Wait for a response from the participants
		printf("Waiting for a response from participants.\n");
		for(i=0; i<10; i++)
		{
			if(okNeeded == 0 || voteAbort != 0)
				break;
			sleep(1);
		}
		if(okNeeded == 0)
		{
			printf("Sending GLOBAL_COMMIT message.\n");
			for(i=0; i<t; i++)
			{
				SendThread(sockets[i], GLOBAL_COMMIT);
			}
		}
		else if(voteAbort != 0)
		{
			printf("Sending GLOBAL_ABORT message.\n");
			for(i=0; i<t; i++)
			{
				SendThread(sockets[i], GLOBAL_ABORT);
			}
		}
		else
		{
			printf("Timed out. Sending GLOBAL_ABORT message.\n");
			for(i=0; i<t; i++)
			{
				SendThread(sockets[i], GLOBAL_ABORT);
			}
		}
		sleep(60);
	}
	// Close the socket that was used to listen
	close(socketFD);
  
	/* Last thing that main() should do */
	pthread_exit(NULL);
  
	return EXIT_SUCCESS;
}


void *ListenThread(void *qPos)
{	
	long recvFD, q;
	int i;

	q = (long)qPos;
  
	// Accept a connections (only for TCP) and create a socket for each one
  
	struct sockaddr_storage clientAddr;
	socklen_t clientAddr_size = sizeof(clientAddr);
	
	if((recvFD = accept(socketFD, (struct sockaddr *)&clientAddr, &clientAddr_size))== -1){
		perror("accept");
		exit(1);
	}
	myQueue[q] = recvFD;
	
	char originalString[MAX_LINE_SIZE];
	int num_bytes_received;
	while(1){
		if ((num_bytes_received = recv(recvFD, originalString, MAX_LINE_SIZE-1, 0)) == -1) {
			perror("recvfrom");
			exit(1);
		}
		originalString[num_bytes_received] = '\0';
		
		switch(atoi(originalString))
		{
			case VOTE_COMMIT:
				printf("Received VOTE_COMMIT message.\n");
				okNeeded--;
				break;
			case VOTE_ABORT:
				printf("Received VOTE_ABORT message.\n");
				voteAbort = 1;
				break;
			default:
				break;
		}
		
	}
	
	pthread_exit(NULL);
}


void SendThread(int recvFD, int option)
{	
	char message [20];
	int ackLen;

	
	sprintf(message, "%i", option);
	
	ackLen = strlen(message);

	//Send request to other servers
	if ((send(recvFD, message, ackLen, 0)) == -1) {
		perror("srverTCP: send");
		exit(1);
	}

}

int rankPorts(int numPorts)
{
	int thisPort, result, tmp, i;

	thisPort = atoi(port);
	result=1;
	for(i=0; i<numPorts; i++)
	{
		tmp = atoi(ports[i*2+1]);
		if(tmp < thisPort)
		{
			result++;
		}
	}
	
	return result;
}


