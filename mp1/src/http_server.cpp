/*
** server.c -- a stream socket server demo
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include <iostream>
#include <fstream>

#define BACKLOG 10	 // how many pending connections queue will hold
#define MAXDATASIZE 10000 // max number of bytes we can get at once 

void sigchld_handler(int signal_number)
{
    (void)signal_number;
	while(waitpid(-1, NULL, WNOHANG) > 0);
}

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
	if (sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}

	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

using namespace std;

int main(int argc, char *argv[])
{
    FILE * fp;
    string input, file_path, s_send_data = "HTTP/1.1 200 OK\r\n\r\n";
	int sockfd, new_fd;  // listen on sock_fd, new connection on new_fd
	struct addrinfo hints, *servinfo, *p;
	struct sockaddr_storage their_addr; // connector's address information
	socklen_t sin_size;
	struct sigaction sa;
	int yes=1;
	char c, s[INET6_ADDRSTRLEN], buf[MAXDATASIZE], send_404_data[] = "HTTP/1.1 404 Not Found\r\n\r\n",
    send_400_data[] = "HTTP/1.1 400 Bad Request\r\n\r\n", send_data[MAXDATASIZE];
	int rv, numbytes;

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE; // use my IP
	if (argc != 2) {
	    fprintf(stderr,"usage: client hostname\n");
	    exit(1);
	}
	if ((rv = getaddrinfo(NULL, argv[1], &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return 1;
	}

	// loop through all the results and bind to the first we can
	for(p = servinfo; p != NULL; p = p->ai_next) {
		if ((sockfd = socket(p->ai_family, p->ai_socktype,
				p->ai_protocol)) == -1) {
			perror("server: socket");
			continue;
		}

		if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes,
				sizeof(int)) == -1) {
			perror("setsockopt");
			exit(1);
		}

		if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
			close(sockfd);
			perror("server: bind");
			continue;
		}

		break;
	}

	if (p == NULL)  {
		fprintf(stderr, "server: failed to bind\n");
		return 2;
	}

	freeaddrinfo(servinfo); // all done with this structure

	if (listen(sockfd, BACKLOG) == -1) {
		perror("listen");
		exit(1);
	}

	sa.sa_handler = sigchld_handler; // reap all dead processes
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_RESTART;
	if (sigaction(SIGCHLD, &sa, NULL) == -1) {
		perror("sigaction");
		exit(1);
	}

	printf("server: waiting for connections...\n");

	while(1) {  // main accept() loop
		sin_size = sizeof their_addr;
		new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);
		if (new_fd == -1) {
			perror("accept");
			continue;
		}

		inet_ntop(their_addr.ss_family,
			get_in_addr((struct sockaddr *)&their_addr),
			s, sizeof s);
		printf("server: got connection from %s\n", s);

		if (!fork()) { // this is the child process
			close(sockfd); // child doesn't need the listener
			if ((numbytes = recv(new_fd, buf, MAXDATASIZE-1, 0)) == -1) {
                perror("recv");
                exit(1);
	        }
            buf[numbytes] = '\0';
            input = buf;
            if (input.find("GET") == string::npos || input.find("HTTP") == string::npos){
                if (send(new_fd, send_400_data, strlen(send_400_data), 0) == -1)
		            perror("send");                
                close(new_fd);
			    exit(0);
            }
            file_path = input.substr(input.find("GET") + 5, input.find("HTTP") - 6);
            fp = fopen (file_path.c_str(), "r");
            if (!fp) {
                if (send(new_fd, send_404_data, strlen(send_404_data), 0) == -1)
		            perror("send");
                close(new_fd);
			    exit(0);
            }
            while(1) {
                c = fgetc(fp);
                if (feof(fp)) { 
                    break;
                }
                s_send_data += c;
                if (s_send_data.length() > MAXDATASIZE){
                    strcpy(send_data, s_send_data.c_str());
                    if (send(new_fd, send_data, strlen(send_data), 0) == -1)
                        perror("send");
                    s_send_data = "";
                }
            }
            if (s_send_data.length() > 0){
                strcpy(send_data, s_send_data.c_str());
                if (send(new_fd, send_data, strlen(send_data), 0) == -1)
                    perror("send");
            }
            fclose(fp);
			close(new_fd);
			exit(0);
		}
		close(new_fd);  // parent doesn't need this
	}

	return 0;
}

