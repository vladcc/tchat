/* tcp socket abstraction module 
 * you need to only declare a socket_type variable to use it
 * see types.h */
#include "os_def.h"
#include "types.h"
#include "network.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_CONNECTIONS 1
#define BUFF_SIZE 256
#define DELIM_CHAR '\0'

static saddr_in server, client;

static void die_with_error(const char * msg);

#ifdef WINDOWS
void socket_init(void)
{
	/* calls WSAStartup() in Windows */
	static WSADATA wsaData;
	if (WSAStartup(MAKEWORD(2,2), &wsaData) != 0)
	{
		fprintf(stderr, "Err: socket initialization failed with code: %d", WSAGetLastError());
		die_with_error("");
	}
	return;
}
#endif

void socket_create(socket_type * s)
{
	/* creates the socket */
#ifdef WINDOWS
	socket_init();
#endif
	*s = socket(PF_INET, SOCK_STREAM , IPPROTO_TCP);
	
#ifdef WINDOWS
	if(*s == INVALID_SOCKET)
#else 
	if(*s < 0)
#endif
		die_with_error("could not create socket");
	
	return;
}

void socket_connect(socket_type * s, const char * ip, unsigned short port)
{
	/* initiates the sockaddr_in for the desired server and connects to it */
	memset(&server, 0, sizeof(server)); // zero out memory
	server.sin_family = AF_INET;
	server.sin_addr.s_addr = socket_resolve_name(ip).s_addr;
    server.sin_port = htons(port);
	
	if (connect(*s, (struct sockaddr *)&server , sizeof(server)) < 0)
		die_with_error("could not connect");
	
	return;
}

void socket_send(socket_type * s, const byte * message, int len)
{
	/* sends len bytes from the message buffer */
	if (send(*s, message, len, 0) != len)
		die_with_error("sending failed");
	
	return;
}

int socket_recv(socket_type * s, void(*do_with_data)(byte * data, int len))
{
	/* receives bytes one by one looking for a delimiter
	 * when it finds one it sends the received data to the do_with_data function */
	static byte recv_buff[BUFF_SIZE]; // allocate once
	int recv_size;
	int received = 0;
	
	while ( received < BUFF_SIZE &&
	(recv_size = recv(*s, recv_buff + received, 1, 0)) != 0 ) // get a single byte
	{	
#ifdef WINDOWS
		if (recv_size == SOCKET_ERROR)
#else
		if (recv_size < 0)
#endif
			die_with_error("receiving failed");
		
		if (*(recv_buff + received) == DELIM_CHAR) // end of message?
		{
			do_with_data(recv_buff, ++received); // send to the worker function
			break; 
		}	
		
		++received;
	}
	
	return recv_size;
}

void socket_shutdown(socket_type * s, int how)
{
	/* shut the socket down */
	if (shutdown(*s, how) != 0)
		die_with_error("shutdown failed");
	
	return;
}

void socket_close(socket_type * s)
{
	/* properly close the socket */
#ifdef WINDOWS
	closesocket(*s);
	WSACleanup();
#else
	close(*s);
#endif
	return;
}

// server only functions
void socket_bind(socket_type * s, unsigned short port)
{
	/* sets the sockaddr_in when creating a server and binds to a port */
	int bind_r;
	
	memset(&server, 0, sizeof(server)); // zero out memory
	server.sin_family = AF_INET;
	server.sin_addr.s_addr = INADDR_ANY;
	server.sin_port = htons(port);

	bind_r = bind(*s, (struct sockaddr *)&server, sizeof(server));

#ifdef WINDOWS
	if (bind_r == SOCKET_ERROR)
#else 
	if (bind_r < 0)
#endif
	die_with_error("binding failed");

	return;
}

void socket_listen(socket_type * s)
{
	/* listens for incoming connections */
	int listen_r;
	
	listen_r = listen(*s, MAX_CONNECTIONS);
#ifdef WINDOWS
	if (listen_r == SOCKET_ERROR)
#else 
	if (listen_r < 0)
#endif
	die_with_error("listen failed");
	
	return;
}

socket_type socket_accept(socket_type * serv_s)
{
	/* accept a connection and return new socket */
	socket_type ret;
	int clnt_len = sizeof(client);
	ret = accept(*serv_s, (struct sockaddr *)&client, &clnt_len);
	
#ifdef WINDOWS
	if (ret == INVALID_SOCKET)
#else
	if (ret < 0)
#endif
		die_with_error("connection not accepted");
	
	return ret;
}

// misc
struct in_addr socket_resolve_name(const char * name)
{
	/* resolves a hostname */
	struct hostent * host;
	
	if ( (host = gethostbyname(name)) == NULL )
		die_with_error("couldn't resolve hostname");
		
	return *((struct in_addr *)host->h_addr_list[0]);
}

char * socket_get_client_ip(void)
{
	/* returns the ip of the connected client */
	return inet_ntoa(client.sin_addr);
}

char * socket_get_server_ip(void)
{
	/* returns the ip of the connected client */
	return inet_ntoa(server.sin_addr);
}

static void die_with_error(const char * msg)
{
	/* go home when bad things happen */
	if (msg[0])
		fprintf(stderr, "Err: %s\n", msg);
	else
		fputc('\n', stderr);
		
#ifdef WINDOWS
	WSACleanup();
#endif
	exit(EXIT_FAILURE);
}
