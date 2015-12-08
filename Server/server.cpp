#include <iostream>
#include <fstream>
#include <sstream>
#include<stdio.h>
#include<string>    //strlen
#include<stdlib.h>    //strlen
#include<sys/socket.h>
#include<arpa/inet.h> //inet_addr
#include<unistd.h>    //write
#include<pthread.h> //for threading , link with lpthread
#include<vector>
#include <strings.h>
#include <dirent.h>
#include <cstring>

using namespace std;

struct sockaddr_in address;
struct sockaddr_in connector_addr;
int current_socket;
int connecting_socket;
int port;
socklen_t addr_size;
int size_of_file;

/**
	Create a socket and assign current_socket to the descriptor
**/
void createSocket()
{
	current_socket = socket(AF_INET, SOCK_STREAM, 0);

	if ( current_socket == -1 )
	{
		perror("Create socket failed");
		exit(-1);
	}
}

/**
	Bind to the current_socket descriptor and listen to the port in PORT
**/
void bindSocket()
{
	address.sin_family = AF_INET; // for internet protocols like tcp and udp
	address.sin_addr.s_addr = INADDR_ANY; // bind the local ip address
	address.sin_port = htons(port); // bind the port number entered from the terminal

	if ( bind(current_socket, (struct sockaddr *)&address, sizeof(address)) < 0 )
	{
		perror("Bind to port failed");
		exit(-1);
	}
}

void startListener()
{
	// let the number of connections that can be made equal 5
	if ( listen(current_socket, 5) < 0 )
	{
		perror("Listen on port failed");
		exit(-1);
	}
	cout << "Listening on 0.0.0.0:" << port << endl;
}

string getRequestLine(string client_message){
    istringstream f(client_message);
    string req_line;
    if(getline(f, req_line)){
        return req_line;
    }
    return 0;
}

string getResponse(string file_name){
	// Form the found response
	ifstream file (file_name.c_str(), ios::in);
	ifstream error_file ("/server/not_found", ios::in);
	puts(file_name.c_str());
	// File found return 200 OK response
	if(file.good()){
		string response = "HTTP/1.0 200 OK\r\n";
		send(connecting_socket, response.c_str(), 1024, 0);
		puts(file_name.c_str());
		// get length of file
		file.seekg(0, file.end);
		size_of_file = file.tellg();
		file.seekg (0, file.beg);
		cout << "file size : " << size_of_file <<endl;
		// read file into buffer
		char *buffer = new char[size_of_file];
		file.read(buffer, size_of_file);
		return buffer;
	}

	// File not found return 404 NotFoundError response
	string response = "HTTP/1.0 404 Not Found\r\n";
	send(connecting_socket, response.c_str(), 1024, 0);
	response = "";
	string temp;
	while(getline(file, temp)){
		response += temp + "\n";
	}
	// get length of error file
	error_file.seekg(0, error_file.end);
	size_of_file = error_file.tellg();
	error_file.seekg (0, error_file.beg);
	return response;
}

string handleGET(string request){
	// request is like "GET file_name.PNG host_name port_number"
    istringstream f(request);
    vector<string> tokens;
    string line;
    while(getline(f, line, ' ')) {
        tokens.push_back(line);
    }
    string file_name = tokens[1];
    return getResponse(file_name);
}

void handlePOST(string request){
	// request is like "POST file_name.PNG host_name port_number"

	// Send approval for the client to send file first
	send(connecting_socket, "OK", 100, 0);
	istringstream f(request);
	vector<string> tokens;
	string line;
	while(getline(f, line, ' ')) {
		tokens.push_back(line);
	}
	string file_name = tokens[1];

	char client_file[1024];

	ofstream file;
	string filepath = "/server/" + file_name;
	// Write/Append data sent with the file name received
	file.open(filepath.c_str(), ios::out | ios::app);

	// Receive file from client
	int file_size;
	int received_bytes = 0;
	while ( (file_size = recv(connecting_socket, client_file, 1024, 0)) > 0){
		file.write(client_file, sizeof(client_file));
		received_bytes += file_size;
	}
	cout << "received bytes : " << received_bytes <<endl;
	if(file_size < 0){
		perror("Error receiving file");
		exit(-1);
	}

	if(file)
		puts("File is OK");
	else
		puts("File is corrupted");
}

void respond(string client_message){
	// Get the first line of the client message which is the request
    string request = getRequestLine(client_message);
    if(request.find("GET") != string::npos){
        string response = handleGET(request);
        int length = size_of_file;
        stringstream ss;
        ss << length;
        string size = ss.str();
        puts(size.c_str());
        int n = send(connecting_socket, size.c_str(), 100, 0);
        char *buffer = new char[length];
        strcpy(buffer, response.c_str());
		// Send response to client
		cout<<"length : "<<length<<endl;
		int sent_bytes = send(connecting_socket, buffer, length, 0);
		cout<<"sent bytes : "<<sent_bytes<<endl;
		int remain_data = length - sent_bytes;
		cout<<"remain data : "<<remain_data<<endl;
		while (remain_data > 0){
			sent_bytes = send(connecting_socket, buffer, length, 0);
			cout<<"sent bytes : "<<sent_bytes<<endl;
			remain_data -= sent_bytes;
			cout<<"remain data : "<<remain_data<<endl;
		}
		if (sent_bytes < 0) {
			perror("Send response failed");
			exit(-1);
		}
    }
    else if(request.find("POST") != string::npos){ // POST
        handlePOST(request);
    }
}

void *handleConnection(void *socket)
{
    // Get socket descriptor
    connecting_socket = *(int *)socket;
    char client_message[2000];

    // Receive message from client
    int message_size ;
    while( (message_size = recv(connecting_socket, client_message, 2000, 0)) > 0 ){
        client_message[message_size] = '\0';
        string str(client_message);
        respond(str);
    }
    return 0;
}

void acceptConnection()
{
	// connector_addr is a buffer that receives address of connecting entity
	addr_size = sizeof(connector_addr);
    pthread_t thread_id;

    // accept returns a newly created socket with same attributes as original socket
	connecting_socket = accept(current_socket, (struct sockaddr *)&connector_addr, &addr_size);
    if( pthread_create( &thread_id, NULL, handleConnection, (void*) &connecting_socket)){
        perror("could not create thread");
        exit(-1);
    }

	if ( connecting_socket < 0 )
	{
		perror("Accepting sockets failed");
		exit(-1);
	}

	// Print out the address and port the client connected to
	int addr = (connector_addr.sin_addr.s_addr & 0xff) == 127;
	cout << "Connected to" << addr << ":" << port << endl;

}

void start()
{
	createSocket();
	bindSocket();
	startListener();

	while ( 1 )
	{
		acceptConnection();
	}
}

int main(int argc, char *argv[])
{
    if(argc != 2) return 0;
    char* port_number = argv[1];
    port = atoi(port_number);
    start();

    return 0;
}
