//David Lee
//08-03-2020

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <netdb.h>
#define MAX_CLIENTS 5

//Universal function that sends or receives stream based on parameters provided
//Works both with client and server
void transmit(int FD, char *buffer, int string_len, int type){
    //type 1 = SEND     //type 2 = RECV
    int stream_size = 0; //tracks stream count processed
    int stream_pending = string_len; //tracks stream count pending
    int stream_done; //tracks stream count send/recv during transmission
    char temp[string_len]; //temp storage for recv only
    char * err_msg;
    while (stream_size < string_len){
        if(type == 1){ //if send
            err_msg = "SERVER: ERROR Sending data";
            stream_done = send(FD, buffer + stream_size, stream_pending, 0);
        }else if(type == 2){ //if recv
            err_msg = "SERVER: ERROR Receiving data";
            stream_done = recv(FD, temp + stream_size, stream_pending, 0);
        }
        if (stream_done == -1){ //tracks send/recv error
            fprintf(stderr, "%s\n", err_msg);
            exit(1);
        }
        stream_size = stream_size + stream_done; //adjust stream count processed
        stream_pending = stream_pending - stream_done; //adjust stream count pending
    }
    if (type == 2){ //if recv
        memcpy(buffer, temp, sizeof(temp)); //copy received stream to buffer
    }
}

//From CS344 Class Module: Client-Server Communication Via Sockets
// Set up the address struct for the server socket
void setupAddressStruct(struct sockaddr_in* address,
                        int portNumber){

    // Clear out the address struct
    memset((char*) address, '\0', sizeof(*address));
    // The address should be network capable
    address->sin_family = AF_INET;
    // Store the port number
    address->sin_port = htons(portNumber);
    // Allow a client at any address to connect to this server
    address->sin_addr.s_addr = INADDR_ANY;
}

//main function for enc_server
int main(int argc, char *argv[]){
//Creating Socket, Bind Socket and Listen
/////////////////////////////////////////////////////////////////////////////////////////////////////
    int connectionSocket;
    struct sockaddr_in serverAddress, clientAddress;
    socklen_t sizeOfClientInfo = sizeof(clientAddress);

    // Check usage & args
    if (argc < 2) {
        fprintf(stderr,"USAGE: %s port\n", argv[0]);
        exit(1);
    }

    // Create the socket that will listen for connections
    int listenSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (listenSocket < 0) {
        fprintf(stderr,"ERROR opening socket");
        exit(1);
    }

    // Set up the address struct for the server socket
    setupAddressStruct(&serverAddress, atoi(argv[1]));

    // Associate the socket to the port
    if (bind(listenSocket, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) < 0){
        fprintf(stderr,"ERROR binding socket");
        exit(1);
    }

    // Start listening for connections. Allow up to 5 connections to queue up
    listen(listenSocket, 5);

    //Concurrency of upto 5 processes created
    //support up to five concurrent socket connections running at the same time
    //set up a pool of five processes at the beginning of the program before the server allows connections
    for(int i = 0; i < MAX_CLIENTS; i++){
        int process_id = fork();
        if (process_id == -1){
            fprintf(stderr, "ERROR creating fork\n");
            exit(1);
        }else if(process_id == 0){
            while(1){
                // Accept the connection request which creates a connection socket
                connectionSocket = accept(listenSocket, (struct sockaddr *)&clientAddress, &sizeOfClientInfo);
                if (connectionSocket < 0){
                    fprintf(stderr,"ERROR accepting socket");
                    exit(1);
                }

//Receive Program Type//
/////////////////////////////////////////////////////////////////////////////////////////////////////
                char type_storage[4]; //storage for type
                memset(type_storage, '\0', 4); //reset type_storage array
                int type_length; //stores length of type
                recv(connectionSocket, &type_length, sizeof(type_length), 0); //store length of type received from client
                transmit(connectionSocket, type_storage, type_length, 2); //store type stream received from client
/////////////////////////////////////////////////////////////////////////////////////////////////////

//Receive Text from Client//
/////////////////////////////////////////////////////////////////////////////////////////////////////
                char plain_text_storage[80000]; //storage for plain text to be received
                memset(plain_text_storage, '\0', 80000); //reset storage
                int plaintext_length; //stores length of text
                recv(connectionSocket, &plaintext_length, sizeof(plaintext_length), 0); //store length of stream received from client
                transmit(connectionSocket, plain_text_storage, plaintext_length, 2); //store stream received from client
/////////////////////////////////////////////////////////////////////////////////////////////////////

//Receive Key from Client//
/////////////////////////////////////////////////////////////////////////////////////////////////////
                char key_text_storage[80000]; //storage for key to be received
                memset(key_text_storage, '\0', 80000); //reset storage
                int key_length; //stores length of key
                recv(connectionSocket, &key_length, sizeof(key_length), 0); //store length of stream received from client
                transmit(connectionSocket, key_text_storage, key_length, 2); //store stream received from client
/////////////////////////////////////////////////////////////////////////////////////////////////////

//Encrypt//
/////////////////////////////////////////////////////////////////////////////////////////////////////
                char enc_text[80000]; //store encrypted text in buffer to send to client
                memset(enc_text, '\0', sizeof(enc_text)); //reset buffer
                int len_sender = strlen(plain_text_storage); //length of string in received stream
                for(int i = 0; i < len_sender; i++){ //iterate through length of text stream
                    char enc;
                    char pt = plain_text_storage[i]; //char in plain text
                    char k = key_text_storage[i]; //char in key text
                    if(pt == 32){ //check if pt is space
                        pt = 91;
                    }
                    if(k == 32){ //check if k is space
                        k = 91;
                    }
                    if(((pt - 64) + (k - 64)) > 27){ //requires mod calc
                        enc = (((pt - 64) + (k - 64)) - 27) + 64;
                    }else{
                        enc = ((pt - 64) + (k - 64)) + 64;
                    }
                    if(enc == 91){ //check if encryption should be space
                        enc = 32;
                    }enc_text[i] = enc; //store new char in buffer
                }
                int len_enc = strlen(enc_text); //length of encrypted stream
/////////////////////////////////////////////////////////////////////////////////////////////////////

//Send New Text to Client//
/////////////////////////////////////////////////////////////////////////////////////////////////////
                if(strcmp(type_storage, "enc") == 0){ //send to client only if connected to correct enc client
                    send(connectionSocket, &len_enc, sizeof(len_enc), 0); //send length of encrypted stream
                    transmit(connectionSocket, enc_text, len_enc, 1); //send encrypted stream to client
                }
                close(connectionSocket);
/////////////////////////////////////////////////////////////////////////////////////////////////////
            }
        }
    }
    close(listenSocket);
    wait(NULL); //parent will only proceed with its execution when its child has terminated

    return 0;
}
