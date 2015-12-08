#include <iostream>
#include <fstream>
#include <sstream>
#include<stdio.h>
#include<string>
#include<stdlib.h>
#include<sys/socket.h>
#include<arpa/inet.h>
#include<unistd.h>
#include<pthread.h>
#include<vector>
#include <strings.h>
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <netinet/in.h>
#include <string.h>

using namespace std;
string PATH = "/client/";
string POST = "POST";
#define MAX 2000
/*returns file name in path  /blah/blah....../target*/
string getLast(string c )
{

    int loc = 0;

    for(int i = c.size()-1 ; i >= 0; i--)
        if(c[i] == '/'){
            loc = i+1;
            break;
        }

    string res = "";

    for(int j = loc ; j< c.size(); j++)
        res += c[j];

    return res;
}
string ignoreFirstLine(string file)
{
    string newFile;
    bool found = 0;
    for(int i = 0 ; i<file.size() ; i++)
    {
        if(found)
            newFile+=file[i];
        if(file[i] == '\n')
            found=1;

    }
    return newFile;
}

int main(int argc, char *argv[])
{
    int sockfd, portno, n;
    struct sockaddr_in serv_addr;
    struct hostent *server;

    char message[MAX];

    if (argc < 3)
    {
        fprintf(stderr, "usage %s hostname port\n", argv[0]);
        exit(0);
    }

    portno = atoi(argv[2]);

        /* Create a socket point */

        sockfd = socket(AF_INET, SOCK_STREAM, 0);

        if (sockfd < 0)
        {
            perror("ERROR opening socket");
            exit(1);
        }

        server = gethostbyname(argv[1]);

        if (server == NULL)
        {
            fprintf(stderr, "ERROR, no such host\n");
            exit(0);
        }

        bzero((char *) &serv_addr, sizeof(serv_addr));
        serv_addr.sin_family = AF_INET;
        bcopy((char *) server->h_addr, (char *) &serv_addr.sin_addr.s_addr,
              server->h_length);
        serv_addr.sin_port = htons(portno);

        /* Now connect to the server */
        if (connect(sockfd, (struct sockaddr*) &serv_addr, sizeof(serv_addr)) < 0)
        {
            perror("ERROR connecting");
            exit(1);
        }


        /* Now ask for a message from the user, this message
         * will be read by server
         */
        printf("Please enter the message: ");
        bzero(message,MAX);
        fgets(message,MAX-1,stdin);


//        for(int i = 0 ;  i < line.size() ; i++)
  //          message[i] = line[i];

        /*parse message to get filename and command */
        istringstream f((string) message);
        vector<string> tokens;
        string temp;
        while (getline(f, temp, ' '))
            tokens.push_back(temp);

        string file_name = tokens[1];

        string command = tokens[0];

        if (command == POST)
        {

            /* Send message to the server */
            n = send(sockfd, message, strlen(message), 0);

            if (n < 0)
            {
                perror("ERROR writing to socket");
                exit(1);
            }

            /* Now read server response */
            bzero(message, MAX);
            n = recv(sockfd, message, 255, 0);

            if (n < 0)
            {
                perror("ERROR reading from socket");
                exit(1);
            }

            if ((string) message == "OK")
            {

                // open file and read it
                string filepath = PATH.append(file_name).c_str();
                ifstream file(filepath.c_str(), ios::in);
                puts(filepath.c_str());

                if (file)
                {
                    // get length of file
                    file.seekg (0, file.end);
                    int length = file.tellg();
                    file.seekg (0, file.beg);
                    cout << "file size : " << length <<endl;
                    // read file into buffer
                    char *buffer = new char[length];
                    file.read(buffer, length);

                    int sent_bytes = send(sockfd, buffer, length, 0);
                    int remain_data = length - sent_bytes;
                    while (remain_data > 0)
                    {
                        sent_bytes = send(sockfd, buffer, length, 0);
                        remain_data -= sent_bytes;
                        cout << "sent bytes : " << sent_bytes <<endl;
                        cout << "remain data : " << remain_data <<endl;
                    }
                    if (sent_bytes < 0)
                    {
                        perror("ERROR writing to socket");
                        exit(1);
                    }
                }
            }
            else
            {
                perror("error : didn't recive OK message");
                exit(1);
            }
        }
        else
        {
            // GET command

            /* Send message to the server */
            n = send(sockfd, message, strlen(message), 0);

            if (n < 0)
            {
                perror("ERROR writing to socket");
                exit(1);
            }

            char req[1024];
            int k = recv(sockfd, req, 1024, 0);

            char size[100];
            int temp = recv(sockfd, size, 100, 0);
            int size_file = atoi(size);
            cout << "size of file = " << size_file << endl;
            /* Now read server response */
            char server_response[1024];
            string response = "";
            int file_size;
            int received_bytes = 0;

            file_name = getLast(file_name);
            fstream file;
            string filepath = PATH + file_name;
            file.open(filepath.c_str(), ios::out | ios::app);
            while ((file_size = recv(sockfd, server_response, 1024, 0)) > 0)
            {
            	puts(server_response);
            	cout<<"file size : "<<file_size<<endl;
            	file.write(server_response, sizeof(server_response));
                received_bytes += file_size;
				cout << "received bytes : " << received_bytes <<endl;
            	if(received_bytes == size_file)
            		break;
            }
            cout << "received bytes : " << received_bytes <<endl;
            if(file_size < 0)
            {
                perror("Error receiving file");
                exit(-1);
            }

        }


    return 0;
}
