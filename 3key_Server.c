//standard libraries
#include <stdio.h>
#include <stdlib.h>

#include <string.h>//used for memset()

//socket libraries
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/sendfile.h>

//internet address library
#include <netinet/in.h>
#include <arpa/inet.h>

//error library
#include <errno.h>

//file manipulation library
#include <fcntl.h> //for open
#include <unistd.h> //for close

#include <time.h> //close()
int main(){
	
	//variables used for timing latency
	clock_t start, end;
	double cpu_time_used;
	
	//create a message for the server to send
	char server_message[BUFSIZ];
	
	char *ip = "169.0.0.1";
	
	//create a socket
	int server_socket;
	server_socket = socket(AF_INET, SOCK_STREAM, 0);
	
	//check if server socket was created successfully
	if(server_socket < 0){
		perror("[Server]Socket error");
		exit(1);
	}
	printf("[Server]TCP server socket created.\n");
	
	//create and clear an address for the socket	
	struct sockaddr_in server_address;
	memset(&server_address, '\0', sizeof(server_address));
	
	//specify values for the address
	server_address.sin_family = AF_INET;
	server_address.sin_port = htons(9001);
	server_address.sin_addr.s_addr = inet_addr(ip);
	setsockopt(server_socket, SOL_SOCKET, SO_BINDTODEVICE, "enp0s3", strlen("enp0s3"));
	
	//bind the socket
	int connection_status = bind(server_socket, (struct sockaddr *) &server_address, sizeof(server_address));
	
	//check is bind was successfull
	if(connection_status < 0){
		perror("[Server]Failed to establish connection");
		exit(2);
	}
	printf("[Server]Connection successfully established\n");
	
	//open and verifying file
	FILE *key_file;
	key_file = fopen("key_file.txt", "r");
	if(key_file < 0){
		fprintf(stdout, "Error opening file");
		exit(-1);
	}
	
	//finding file size
	int key_size;
	fseek(key_file, 0L, SEEK_END);
	key_size = ftell(key_file);
	fseek(key_file, 0L, SEEK_SET); //move pointer back to start of file
	fprintf(stdout, "Buffer size: %d bytes\n", BUFSIZ);
	fprintf(stdout, "File size: %d bytes\n", key_size);
	
	//start timing for latency
	start = clock();
	
	//listen for connection
	listen(server_socket, 5);
	
	//after finding a client connect it to the server
	int client1_socket;
	struct sockaddr client1_address;
	socklen_t client1_len = sizeof(client1_address);
	memset(&client1_address, '\0', sizeof(client1_address));
	client1_socket = accept(server_socket, (struct sockaddr *) &client1_address, &client1_len);
	struct sockaddr_in* client1_ipv4 = (struct sockaddr_in*)&client1_address;
	struct in_addr client1_ip = client1_ipv4->sin_addr;
	fprintf(stdout, "Client 1 ip: %s\n", inet_ntoa(client1_ip));
	
	int client2_socket;
	struct sockaddr client2_address;
	socklen_t client2_len = sizeof(client2_address);
	memset(&client2_address, '\0', sizeof(client2_address));
	client2_socket = accept(server_socket, (struct sockaddr *) &client2_address, &client2_len);
	struct sockaddr_in* client2_ipv4 = (struct sockaddr_in*)&client2_address;
	struct in_addr client2_ip = client2_ipv4->sin_addr;
	fprintf(stdout, "Client 2 ip: %s\n", inet_ntoa(client2_ip));
	
	//send message with file size to client
	sprintf(server_message, "%d", key_size);
	send(client1_socket, server_message, sizeof(server_message), 0);
	
	//file information for sending
	int sent_bytes = 0;
	off_t offset = 0;
	int remain_data = key_size;
	
	//clear server message
	memset(server_message, 0, sizeof(server_message));
	
	//send data in chunks over the buffer of the port 
	//while(((sent_bytes = sendfile(client1_socket, fileno(key_file), &offset, BUFSIZ)) > 0) && (remain_data > 0)){
	while(((sent_bytes = fread(server_message, 1, BUFSIZ, key_file)) > 0) && (remain_data > 0)){
		//fprintf(stdout, "Server sent %d bytes\n", sent_bytes);
		//remain_data -= sent_bytes;
		//fprintf(stdout, "Remaining bytes to send: %d\n", remain_data);
		fprintf(stdout, "Server sent %d bytes\n", sent_bytes);
		send(client2_socket, server_message, sent_bytes, 0);
		remain_data -= sent_bytes;
		fprintf(stdout, "Remaining bytes to send %d\n", remain_data);
	}
	
	//recieve file size from client
	char client_response[BUFSIZ];
	int message_size;
	recv(client2_socket, &client_response, sizeof(client_response), 0);
	message_size = atoi(client_response);
	
	//print out file size
	fprintf(stdout, "File size: %d bytes\n", message_size);
	
	//specify recieving file
	FILE *message_file = fopen("message_file.txt", "w");
	
	//recieve file from client in chunks
	remain_data = message_size;
	int recv_message;
	
	while((remain_data > 0) && (recv_message = recv(client2_socket, &client_response, BUFSIZ, 0))){
		fwrite(client_response, sizeof(char), recv_message, message_file);
		remain_data -= recv_message; 
		fprintf(stdout, "Recieved %d bytes\n", recv_message);
	}
	
	//close the socket
	//shutdown(client1_socket, SHUT_RDWR);
	//shutdown(client2_socket, SHUT_RDWR);
	//shutdown(server_socket, SHUT_RDWR);
	close(client1_socket);
	close(client2_socket);
	close(server_socket);
	
	//stop timing
	end = clock();
	
	//show latency
	cpu_time_used = ((double)(end - start) / CLOCKS_PER_SEC);
	fprintf(stdout, "Latency Time: %f\n", cpu_time_used);
	
	//close files
	fclose(key_file);
	fclose(message_file);
	
	return(0);
}
