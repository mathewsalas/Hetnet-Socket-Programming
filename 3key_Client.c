//standard libraries
#include <stdio.h>
#include <stdlib.h>

#include <string.h>//used for memset()

//socket libraries
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/sendfile.h>

//internet address library
#include <netinet/in.h>
#include <arpa/inet.h>

//file manipulation library
#include <fcntl.h> //for open
#include <unistd.h> //for close

int main(){
	
	//create a socket
	int client1_socket, client2_socket;
	client1_socket = socket(AF_INET, SOCK_STREAM, 0);
	client2_socket = socket(AF_INET, SOCK_STREAM, 0);
	
	//NIC setup
	char *ip_serv = "169.0.0.1";
	char *ip_client1 = "169.0.0.2";
	char *ip_client2 = "169.0.0.3";
	
	//check if network socket was created successfully
	if((client1_socket < 0 )||(client2_socket < 0)){
		perror("[Client]Socket error");
		exit(1);
	}
	printf("[Client]TCP server client created.\n");
	
	//create and clear an address for the socket
	struct sockaddr_in server_address, client1_address, client2_address;
	memset(&server_address, '\0', sizeof(server_address));
	memset(&client1_address, '\0', sizeof(client1_address));
	memset(&client2_address, '\0', sizeof(client2_address));
	
	//specify values for the server address
	server_address.sin_family = AF_INET;
	server_address.sin_port = htons(9001);
	server_address.sin_addr.s_addr = inet_addr(ip_serv);
	
	//specify values for the client 1 address
	client1_address.sin_family = AF_INET;
	client1_address.sin_port = htons(9001);
	client1_address.sin_addr.s_addr = inet_addr(ip_client1);
	setsockopt(client1_socket, SOL_SOCKET, SO_BINDTODEVICE, "enp0s3", strlen("enp0s3"));
	
	//specify values for the client 2 address
	client2_address.sin_family = AF_INET;
	client2_address.sin_port = htons(9001);
	client2_address.sin_addr.s_addr = inet_addr(ip_client2);
	setsockopt(client2_socket, SOL_SOCKET, SO_BINDTODEVICE, "enp0s4", strlen("enp0s4"));
	
	//bind socket to specific local nic and check for succsess
	int client1_status, client2_status;
	client1_status = bind(client1_socket, (struct sockaddr *) &client1_address, sizeof(client1_address));
	client1_status = bind(client2_socket, (struct sockaddr *) &client2_address, sizeof(client2_address));
	
	if((client1_status < 0)||(client2_status < 0)){
		perror("[Client]Failed to bind");
		exit(2);
	}
	
	//connect socket to server
	int connection1_status = connect(client1_socket, (struct sockaddr *) &server_address, sizeof(server_address));
	int connection2_status = connect(client2_socket, (struct sockaddr *) &server_address, sizeof(server_address));
	
	//check for connection error
	if((connection1_status < 0)||(connection2_status < 0)){
		perror("[Client]Failed to establish connection");
		exit(2);
	}
	printf("[Client]Connection successfully established.\n");
	
	//request key from server over socket 1
	char key_request[] = "Requesting key from server";
	send(client1_socket, key_request, sizeof(key_request), 0);
	
	//receive file size from the server on socket 1
	char server_response[BUFSIZ];
	int key_size;
	recv(client1_socket, &server_response, sizeof(server_response), 0);
	key_size = atoi(server_response);
	
	//print out file size
	fprintf(stdout, "[Server]File size: %d bytes\n", key_size);
	
	//specify recieving file and message file
	FILE *key_file = fopen("key_file.txt", "w");
	FILE *message_file = fopen("message_file.txt", "a");
	
	//receive file from server and writing to new file + message
	int remain_data = key_size;
	int recv_key;
	
	while((remain_data > 0) && (recv_key = recv(client1_socket, &server_response, BUFSIZ, 0))){
		fwrite(server_response, sizeof(char), recv_key, key_file);
		fwrite(server_response, sizeof(char), recv_key, message_file);
		remain_data -= recv_key;
		fprintf(stdout, "Recieved %d bytes\n", recv_key);
	}
	
	//reseting message file to be sent back to server
	fseek(message_file, 0, 0);
	int message_size = ftell(message_file);
	char client_message[BUFSIZ];
	
	//send message with file size over client 2
	sprintf(client_message, "%d", message_size);
	send(client2_socket, client_message, sizeof(client_message), 0);
	
	//file information for sending
	int sent_bytes = 0;
	off_t offset = 0;
	remain_data = message_size;
	
	//send data in chunks over the buffer of the port
	while(((sent_bytes = sendfile(client2_socket, fileno(message_file), &offset, BUFSIZ)) > 0) && (remain_data > 0)){	
		fprintf(stdout, "Client 2 send %d bytes\n", sent_bytes);
		remain_data -= sent_bytes;
		fprintf(stdout, "Remain bytes to send %d\n", remain_data);
	}
	
	//close the socket
	shutdown(client1_socket, SHUT_RDWR);
	shutdown(client2_socket, SHUT_RDWR);
	close(client1_socket);
	close(client2_socket);
	
	return(0);
}
