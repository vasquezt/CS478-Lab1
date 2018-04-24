#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>

/*Globals*/
int numChildren = 0;

void error(const char *msg) { perror(msg); exit(1); } // Error function used for reporting issues

void sigHandler(int signo){
	//Cleans up children on termination
	if(signo == SIGCHLD){
		wait();
		numChildren--;
	}
}

int main(int argc, char *argv[]){

	//Signal handeler for children
	struct sigaction SIGCHLD_action = {0};
	signal(SIGCHLD, sigHandler);

	int listenSocketFD, establishedConnectionFD, portNumber, charsRead;
	socklen_t sizeOfClientInfo;
	//char buffer[256];
	struct sockaddr_in serverAddress, clientAddress;

	if (argc < 2) { fprintf(stderr,"USAGE: %s port\n", argv[0]); exit(1); } // Check usage & args

	// Set up the address struct for this process (the server)
	memset((char *)&serverAddress, '\0', sizeof(serverAddress)); // Clear out the address struct
	portNumber = atoi(argv[1]); // Get the port number, convert to an integer from a string
	serverAddress.sin_family = AF_INET; // Create a network-capable socket
	serverAddress.sin_port = htons(portNumber); // Store the port number
	serverAddress.sin_addr.s_addr = INADDR_ANY; // Any address is allowed for connection to this process

	// Set up the socket
	listenSocketFD = socket(AF_INET, SOCK_STREAM, 0); // Create the socket
	if (listenSocketFD < 0) error("ERROR opening socket");

	// Enable the socket to begin listening
	if (bind(listenSocketFD, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) < 0) // Connect socket to port
		error("ERROR on binding"); //MIGHT NEED LOOP HERE TO GET PORT
	listen(listenSocketFD, 5); // Flip the socket on - it can now receive up to 5 connections

	while(1){
		// Accept a connection, blocking if one is not available until one connects
		sizeOfClientInfo = sizeof(clientAddress); // Get the size of the address for the client that will connect
		//This is here to keep more then 5 connections from being formed at once
		int findingConnection = 1;
		while(findingConnection == 1){
			if(numChildren < 5){
				establishedConnectionFD = accept(listenSocketFD, (struct sockaddr *)&clientAddress, &sizeOfClientInfo); // Accept
				//printf("connection accepted\n");
				findingConnection = 0;
			}
		}

		//Building Fork for Children and Parents
		int pid = fork();
		if(pid > 0){
			numChildren++;
		}else{
			//******************************************************************************
			//Here is where our server will comunicate with the client
			int bytesRec, lengthOfText, gettingData, fileLocation, totBytesRecived, bytesReciving;
			int serverType, clientType, bytesSent;
			char *textContent, *keyContent, *buffer, *encryptedContent;

			serverType = 1;
			//Send and recive type of server, handshake if you will
			//bytesRec = recv(establishedConnectionFD, &clientType, sizeof(int), 0);
			//bytesSent = send(establishedConnectionFD, &serverType, sizeof(int), 0);

			//printf("server type: %d\nclient type: %d\n", serverType, clientType);

			//if(clientType != serverType){
			//	write(1, "problem with boi\n", 17);
			//	exit(1);
			//}

			//Get size of plaintext file
			bytesRec = recv(establishedConnectionFD, &lengthOfText, sizeof(int), 0);


			//NOTE THE + 1	
			textContent = malloc(lengthOfText + 1);
			keyContent = malloc(lengthOfText + 1);
			encryptedContent = malloc(lengthOfText + 1);
			buffer = malloc(1000);	

			//start accepting packets of 1000 bytes of plaintext
			totBytesRecived = 0;
			bytesReciving = 1000;
			gettingData = 1;			
			while(gettingData == 1){
				memset(buffer,0,strlen(buffer));					
				if(bytesReciving > lengthOfText - totBytesRecived){
					bytesReciving = (lengthOfText - totBytesRecived);
				}
				bytesRec = recv(establishedConnectionFD, buffer, bytesReciving, 0);
				if(bytesRec == bytesReciving){
					totBytesRecived = totBytesRecived + bytesRec;
					strcat(textContent, buffer);
					if(totBytesRecived >= lengthOfText){
						gettingData = 0;
					}
				}
			}

			//start accepting packets of 1000 bytes of key
			totBytesRecived = 0;
			bytesReciving = 1000;
			gettingData = 1;	
			while(gettingData == 1){	
				memset(buffer,0,strlen(buffer));				
				if(bytesReciving > lengthOfText - totBytesRecived){	
					bytesReciving = (lengthOfText - totBytesRecived);
				}
				bytesRec = recv(establishedConnectionFD, buffer, bytesReciving, 0);
				if(bytesRec == bytesReciving){
					totBytesRecived = totBytesRecived + bytesRec;
					strcat(keyContent, buffer);
					if(totBytesRecived >= lengthOfText){
						gettingData = 0;
					}
				}
			}

			
			//Begin encrypting file
			int i;
			for(i = 0; i < lengthOfText - 1; i++){
				if(textContent[i] == 32){
					textContent[i] = 91;
				}
				if(keyContent[i] == 32){
					keyContent[i] = 91;
				}
				encryptedContent[i] = textContent[i] + keyContent[i] - 65;
				if(encryptedContent[i] > 91){
					encryptedContent[i] = encryptedContent[i] - 27;
				}
				if(encryptedContent[i] == 91){
					encryptedContent[i] = 32;
				}	
			}
			encryptedContent[lengthOfText - 1] = '\n';


			//Send back in 1000 byte packets encryptedContent text
			int sending, totBytesSent, bytesSending;	
			sending = 1;
			fileLocation = 0;
			totBytesSent = 0;
			bytesSending = 1000;
			while(sending == 1){
				if(bytesSending > lengthOfText - totBytesSent){
					bytesSending = (lengthOfText - totBytesSent); // This is to ensure the last packets size
				}

				bytesSent = send(establishedConnectionFD, (encryptedContent + fileLocation), bytesSending, 0);
				if(bytesSent == bytesSending){
					totBytesSent = totBytesSent + bytesSent;
					fileLocation = fileLocation + bytesSent; 
					if(totBytesSent >= lengthOfText){    
						sending = 0;
					}
				}
			}
			free(encryptedContent);


			//exit(0)

			/*
			if (establishedConnectionFD < 0) error("ERROR on accept");
			printf("SERVER: Connected Client at port 5d\n", ntohs(clientAddress.sin_port));
			// Get the message from the client and display it
			memset(buffer, '\0', 256);
			charsRead = recv(establishedConnectionFD, buffer, 255, 0); // Read the client's message from the socket
			if (charsRead < 0) error("ERROR reading from socket");
			printf("SERVER: I received this from the client: \"%s\"\n", buffer);

			// Send a Success message back to the client
			charsRead = send(establishedConnectionFD, "I am the server, and I got your message", 39, 0); // Send success back
			if (charsRead < 0) error("ERROR writing to socket");

			*/
			//******************************************************************************
			close(establishedConnectionFD); // Close the existing socket which is connected to the client
			close(listenSocketFD); // Close the listening socket
			exit(0);
		}
	} 
}
