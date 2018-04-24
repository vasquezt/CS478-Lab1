#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 

void error(const char *msg) { perror(msg); exit(0); } // Error function used for reporting issues

int main(int argc, char *argv[])
{
	int socketFD, portNumber, charsWritten, charsRead;
	struct sockaddr_in serverAddress;
	struct hostent* serverHostInfo;
	//char buffer[256];

	//******************************************************************
	//Here we will check what is being passed to the the program
	if (argc < 4) { fprintf(stderr,"USAGE: %s port\n", argv[0]); exit(1); } // Check usage & args

	FILE *textp, *keyp;
	//argv[1] needs to be the plain text
	textp = fopen(argv[1], "rb");
	if(textp == NULL){
		error("Plain text file not found");
		exit(1);
	}
	//argv[2] needs to be the key
	keyp = fopen(argv[2], "rb");
	if(keyp == NULL){
		error("Key file not found");
		exit(1);
	}	
	//key > plain text
	fseek(textp, 0, SEEK_END);
	int lengthOfText = ftell(textp);
	fseek(keyp, 0, SEEK_END);
	int lengthOfKey = ftell(keyp);
	if(lengthOfText > lengthOfKey){
		fflush(stdout);
		write(1, "ERROR: text is larger then key\n", 31);
		exit(1);
	}

	//From here will will copy the file contents down, to check whether or not to connect
	int i;
	char *textFileContents;
	char *keyFileContents;
	fseek(textp, 0, SEEK_SET);
	fseek(keyp, 0, SEEK_SET);

	textFileContents = malloc(lengthOfText);
	fread(textFileContents, 1, lengthOfText, textp);

	keyFileContents = malloc(lengthOfText);
	fread(keyFileContents, 1, lengthOfText, keyp);

	//Here we will check if there is any bad char's in the file
	for(i = 0; i < lengthOfText - 2; i++){
		if((textFileContents[i] > 90 || textFileContents[i] < 65) && (textFileContents[i] != 32)){
			i = lengthOfText;
			exit(1);
		}
	}
	if(textFileContents[lengthOfText - 1] != '\n'){
		exit(1);
	}

	//note pointers are set to end of file after this
	//******************************************************************

	// Set up the server address struct
	char* hostLocation = "localhost";
	memset((char*)&serverAddress, '\0', sizeof(serverAddress)); // Clear out the address struct
	portNumber = atoi(argv[3]); // Get the port number, convert to an integer from a string
	serverAddress.sin_family = AF_INET; // Create a network-capable socket
	serverAddress.sin_port = htons(portNumber); // Store the port number
	serverHostInfo = gethostbyname(hostLocation); // Convert the machine name into a special form of address
	if (serverHostInfo == NULL) { fprintf(stderr, "CLIENT: ERROR, no such host\n"); exit(0); }
	memcpy((char*)&serverAddress.sin_addr.s_addr, (char*)serverHostInfo->h_addr, serverHostInfo->h_length); // Copy in the address

	// Set up the socket
	socketFD = socket(AF_INET, SOCK_STREAM, 0); // Create the socket
	if (socketFD < 0) error("CLIENT: ERROR opening socket");
	
	// Connect to server
	if (connect(socketFD, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) < 0) // Connect socket to address
		error("CLIENT: ERROR connecting");













	//******************************************************************************
	//Here is where our client will send files to the server
	int bytesSent, bytesRec, sending, fileLocation, totBytesSent, bytesSending;
	int serverType, clientType;

	clientType = 1;
	//Send and recive type of server, handshake if you will
	//bytesSent = send(socketFD, &serverType, sizeof(int), 0);
	//bytesRec = recv(socketFD, &clientType, sizeof(int), 0);

	//printf("server type: %d\nclient type: %d\n", serverType, clientType);

	//if(clientType != serverType){
	//	exit(1);
	//}
	//fflush(stdout);

	//Send size of the plaintext file
	bytesSent = send(socketFD, &lengthOfText, sizeof(lengthOfText), 0);

	//Start sending 1000 byte packets of plaintext
	sending = 1;
	fileLocation = 0;
	totBytesSent = 0;
	bytesSending = 1000;
	while(sending == 1){
		if(bytesSending > lengthOfText - totBytesSent){
			bytesSending = (lengthOfText - totBytesSent); // This is to ensure the last packets size
		}

		bytesSent = send(socketFD, (textFileContents + fileLocation), bytesSending, 0);
		if(bytesSent == bytesSending){
			totBytesSent = totBytesSent + bytesSent;
			fileLocation = fileLocation + bytesSent; 
			if(totBytesSent >= lengthOfText){    
				sending = 0;
			}
		}
	}
	free(textFileContents);

	//Start sending 1000 byte packets of key file (note, not sending whole file)
	sending = 1;
	fileLocation = 0;
	totBytesSent = 0;
	bytesSending = 1000;
	while(sending == 1){
		if(bytesSending > lengthOfText - totBytesSent){
			bytesSending = (lengthOfText - totBytesSent); // This is to ensure the last packets size
		}
		bytesSent = send(socketFD, (keyFileContents + fileLocation), bytesSending, 0);
		if(bytesSent == bytesSending){
			totBytesSent = totBytesSent + bytesSent;
			fileLocation = fileLocation + bytesSent; 
			if(totBytesSent >= lengthOfText){    
				sending = 0;
			}
		}	
	}
	free(keyFileContents);

	//Start recieving 1000 byte packets of encrypted file
	int gettingData, totBytesRecived, bytesReciving;	
	char *encryptedContent;
	char *buffer;
	encryptedContent = malloc(lengthOfText + 1);
	encryptedContent[0] = '\0';
	buffer = malloc(1000);

	totBytesRecived = 0;
	bytesReciving = 1000;
	gettingData = 1;			
	while(gettingData == 1){
		memset(buffer,0,strlen(buffer));					
		if(bytesReciving > lengthOfText - totBytesRecived){
			bytesReciving = (lengthOfText - totBytesRecived);
		}
		bytesRec = recv(socketFD, buffer, bytesReciving, 0);
		if(bytesRec == bytesReciving){
			totBytesRecived = totBytesRecived + bytesRec;
			strcat(encryptedContent, buffer);
			if(totBytesRecived >= lengthOfText){
				gettingData = 0;
			}
		}
	}
	//printf("\nEncryption: %s\n", encryptedContent);

	//Write the encryption to stdout
	write(1, encryptedContent, strlen(encryptedContent));
	exit(0);

	/*
	// Get input message from user
	printf("CLIENT: Enter text to send to the server, and then hit enter: ");
	memset(buffer, '\0', sizeof(buffer)); // Clear out the buffer array
	fgets(buffer, sizeof(buffer) - 1, stdin); // Get input from the user, trunc to buffer - 1 chars, leaving \0
	buffer[strcspn(buffer, "\n")] = '\0'; // Remove the trailing \n that fgets adds

	// Send message to server
	charsWritten = send(socketFD, buffer, strlen(buffer), 0); // Write to the server
	if (charsWritten < 0) error("CLIENT: ERROR writing to socket");
	if (charsWritten < strlen(buffer)) printf("CLIENT: WARNING: Not all data written to socket!\n");

	// Get return message from server
	memset(buffer, '\0', sizeof(buffer)); // Clear out the buffer again for reuse
	charsRead = recv(socketFD, buffer, sizeof(buffer) - 1, 0); // Read data from the socket, leaving \0 at end
	if (charsRead < 0) error("CLIENT: ERROR reading from socket");
	printf("CLIENT: I received this from the server: \"%s\"\n", buffer);
	*/
	//******************************************************************************

	close(socketFD); // Close the socket
	return 0;	
}
