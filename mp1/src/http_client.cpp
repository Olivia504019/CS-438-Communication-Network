#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <iostream>
#include <arpa/inet.h>
#include <fstream>

#define PORT "80" // the port client will be connecting to 

#define MAXSENDDATASIZE 1000
#define MAXDATASIZE 1000 // max number of bytes we can get at once 
using namespace std;
// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
	if (sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}

	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int main(int argc, char *argv[])
{
    FILE * fp;
	int sockfd, numbytes;  
	char buf[MAXDATASIZE], send_data[MAXSENDDATASIZE];
    string s_input, s_address, s_port = PORT, s_file_path;
    char address[20], port[10], file_path[100];
	struct addrinfo hints, *servinfo, *p;
	int rv;
	char s[INET6_ADDRSTRLEN];

	if (argc != 2) {
	    fprintf(stderr,"usage: client hostname\n");
	    exit(1);
	}

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

    s_input = argv[1];
    s_input = s_input.substr(s_input.find("//") + 2, s_input.length());
    s_address = s_input.substr(0, s_input.find("/"));
    s_file_path = s_input.substr(s_input.find("/"), s_input.length());
    if (s_address.find(":") != string::npos){
        s_port = s_address.substr(s_address.find(":") + 1, s_address.length());
        s_address = s_address.substr(0, s_address.find(":"));
    }
    strcpy(address, s_address.c_str());
    strcpy(port, s_port.c_str());
    strcpy(file_path, s_file_path.c_str());

	if ((rv = getaddrinfo(address, port, &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return 1;
	}
	// loop through all the results and connect to the first we can
	for(p = servinfo; p != NULL; p = p->ai_next) {
		if ((sockfd = socket(p->ai_family, p->ai_socktype,
				p->ai_protocol)) == -1) {
			perror("client: socket");
			continue;
		}

		if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
			close(sockfd);
			perror("client: connect");
			continue;
		}

		break;
	}

	if (p == NULL) {
		fprintf(stderr, "client: failed to connect\n");
		return 2;
	}

	inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr),
			s, sizeof s);

	freeaddrinfo(servinfo); // all done with this structure
    snprintf(send_data, MAXSENDDATASIZE - 1, "GET %s HTTP/1.1\r\nHost: %s\r\nConnection: Keep-Alive\r\n\r\n", file_path, address);

    if (send(sockfd, send_data, strlen(send_data), 0) == -1)
		perror("send");

    s_input = "";
	if ((numbytes = recv(sockfd, buf, sizeof(buf), 0)) > 0) {
	    s_input.append(buf, numbytes);
	}
	cout << s_input << "\n";
    s_input = s_input.substr(s_input.find("\r\n\r\n") + 4, s_input.length());
    fp = fopen ("output", "w+");
    fputs(s_input.c_str(), fp);
	cout << s_input << "\n";
    s_input = "";
    while ((numbytes = recv(sockfd, buf, sizeof(buf), 0)) > 0) {
	    s_input.append(buf, numbytes);
	    cout << s_input << "\n";
        fputs(s_input.c_str(), fp);
        s_input = "";
	}
    fclose(fp);
    close(sockfd);

	return 0;
}

