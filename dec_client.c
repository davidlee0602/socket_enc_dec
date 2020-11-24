//David Lee
//08-03-2020

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <errno.h>
#include<sys/types.h>
#include<sys/stat.h>
#include <fcntl.h>

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
            err_msg = "CLIENT: ERROR Sending data";
            stream_done = send(FD, buffer + stream_size, stream_pending, 0);
        }else if(type == 2){ //if recv
            err_msg = "CLIENT: ERROR Receiving data";
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
// Set up the address struct
void setupAddressStruct(struct sockaddr_in* address,
                        int portNumber){

    // Clear out the address struct
    memset((char*) address, '\0', sizeof(*address));

    // The address should be network capable
    address->sin_family = AF_INET;
    // Store the port number
    address->sin_port = htons(portNumber);

    // Get the DNS entry for this host name
    struct hostent* hostInfo = gethostbyname("localhost"); //changed to localhost
    if (hostInfo == NULL) {
        fprintf(stderr, "CLIENT: ERROR, no such host\n");
        exit(0);
    }
    // Copy the first IP address from the DNS entry to sin_addr.s_addr
    memcpy((char*) &address->sin_addr.s_addr,
           hostInfo->h_addr_list[0],
           hostInfo->h_length);
}

//main function for dec_client
int main(int argc, char *argv[]) {
    int socketFD, portNumber, charsWritten, charsRead;
    struct sockaddr_in serverAddress;

    // Check usage & args
    if (argc < 4) {
        fprintf(stderr,"USAGE: %s text key port\n", argv[0]);
        exit(0);
    }

/////////////////////////////////////////////////////////////////////////////////////////////////////
/* Creating a socket and sending the data to the server */
    // Create a socket
    socketFD = socket(AF_INET, SOCK_STREAM, 0);
    if (socketFD < 0){
        fprintf(stderr, "CLIENT: ERROR cannot create socket\n");
        exit(1);
    }

    // Set up the server address struct
    setupAddressStruct(&serverAddress, atoi(argv[3]));

    // Connect to server
    if (connect(socketFD, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) < 0){
        fprintf(stderr, "CLIENT: ERROR connecting to server\n");
        exit(1);
    }
/////////////////////////////////////////////////////////////////////////////////////////////////////

//Send Program Type//
/////////////////////////////////////////////////////////////////////////////////////////////////////
    char type[4];
    memset(type, '\0', sizeof(type)); //reset type array
    type[0] = 'd';
    type[1] = 'e';
    type[2] = 'c';
    type[3] = '\0';
    int len_type = strlen(type); //set type array to string: 'dec'
    send(socketFD, &len_type, sizeof(len_type), 0); //send length of type array
    transmit(socketFD, type, len_type, 1); //send type text/data to server
    memset(type, '\0', sizeof(type)); //reset type array
/////////////////////////////////////////////////////////////////////////////////////////////////////

//Send Text to Server//
/////////////////////////////////////////////////////////////////////////////////////////////////////
    char plain_text_buffer[80000]; //encrypted text array that will be sent to the server
    int plain_text = open(argv[1], O_RDONLY);
    memset(plain_text_buffer, '\0', sizeof(plain_text_buffer));
    if (read(plain_text, plain_text_buffer, sizeof(plain_text_buffer)) == -1){ //reads file and stores in text buffer
        fprintf(stderr, "Cannot read file: %s\n", argv[1]);
        exit(1);
    }
    plain_text_buffer[strcspn(plain_text_buffer, "\n")] = '\0';
    int len_plain = strlen(plain_text_buffer);
    /* checks for invalid characters in argv[1] */
    for(int i = 0; i < strlen(plain_text_buffer); i++){
        if(!((plain_text_buffer[i] >= 65 && plain_text_buffer[i] <= 90)||(plain_text_buffer[i] == 32))){
            fprintf(stderr, "CLIENT: Invalid character found in %s\n", argv[1]);
            exit(1);
        }
    }
    send(socketFD, &len_plain, sizeof(len_plain), 0); //send length of text buffer
    transmit(socketFD, plain_text_buffer, len_plain, 1); //send text stream to server
    memset(plain_text_buffer, '\0', sizeof(plain_text_buffer)); //reset text buffer
/////////////////////////////////////////////////////////////////////////////////////////////////////

//Send Key to Server//
/////////////////////////////////////////////////////////////////////////////////////////////////////
    char key_buffer[80000];
    int key_text = open(argv[2], O_RDONLY); //key used to decipher encryption
    memset(key_buffer, '\0', sizeof(key_buffer));
    if (read(key_text, key_buffer, sizeof(key_buffer)) == -1){ //reads key and stores in key buffer
        fprintf(stderr, "Cannot read file: %s\n", argv[2]);
        exit(1);
    }
    key_buffer[strcspn(key_buffer, "\n")] = '\0';
    int len_key = strlen(key_buffer);
    /* checks for invalid characters in argv[2] */
    for(int i = 0; i < strlen(key_buffer); i++){
        if(!((key_buffer[i] >= 65 && key_buffer[i] <= 90)||(key_buffer[i] == 32))){
            fprintf(stderr, "CLIENT: Invalid character found in %s\n", argv[2]);
            exit(1);
        }
    }
    send(socketFD, &len_key, sizeof(len_key), 0); //send length of key buffer
    transmit(socketFD, key_buffer, len_key, 1); //send key stream to server
    memset(key_buffer, '\0', sizeof(key_buffer)); //reset key buffer
/////////////////////////////////////////////////////////////////////////////////////////////////////


//Receive Data from Server//
/////////////////////////////////////////////////////////////////////////////////////////////////////
    char recv_buffer[80000]; //Buffer to store data received from server
    memset(recv_buffer, '\0', sizeof(recv_buffer)); //reset and clear receive buffer
//Receive Length of Text from Server
    int recv_buffer_length;
    recv(socketFD, &recv_buffer_length, sizeof(recv_buffer_length), 0); //store length of stream being received
    transmit(socketFD, recv_buffer, recv_buffer_length, 2); //receive stream from server and store in recv_buffer
//Validate Correct Client Server Pair
    if(strcmp(recv_buffer,"") == 0){
        fprintf(stderr, "CLIENT: Could not contact dec_server on port %s; dec_client and enc_server connection is not allowed\n", argv[3]);
        exit(2);
    }
/////////////////////////////////////////////////////////////////////////////////////////////////////

//Check if argv[1] and argv[2] is suitable//
/////////////////////////////////////////////////////////////////////////////////////////////////////
    if (len_plain > len_key){ //checks if text length is greater than key length
        fprintf(stderr, "CLIENT: Length of key %s is shorter than length of %s\n", argv[2], argv[1]);
        exit(1);
    }
/////////////////////////////////////////////////////////////////////////////////////////////////////

//Result Output//
/////////////////////////////////////////////////////////////////////////////////////////////////////
    printf("%s\n", recv_buffer);
    close(socketFD);
/////////////////////////////////////////////////////////////////////////////////////////////////////

    return 0;
}