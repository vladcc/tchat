/* the main module */
#include "os_def.h"				// specifies compilation for Windows or Linux
#include "types.h"				// redefines different types for abstraction and shorthand definitions
#include "network.h"			// tcp socket abstraction module interface
#include "display.h"			// console display interface
#include "non_buff_input.h"		// forces non-buffered input from the keyboard
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#ifdef LINUX
#include <pthread.h>
#else
#include <windows.h>
#endif

#define ARG '-'					// command flags from the terminal begin with this character
#define HOST 'h'				// this option from the terminal creates a session
#define CONNECT 'c'				// this option from the terminal connects to a session
#define PORT 'p'				// this option form the terminal specifies a port number
#define USERNAME 'u'			// this option from the terminal or in the chat sets your username
#define HELP '?'				// this option from the terminal prints help
#define COMMAND '/'				// commands in the chat begin with this character
#define QUIT 'q'				// this option in the chat quits the session
#define CHANGE_NAME 'u'			// same as USERNAME
#define BYEBYE 0xBB				// this magic number is sent as a communication terminator
#define FOR_HELP fprintf(stderr, "%s -%c for help", bin_name, HELP)	// shows how to print help

// when input is non-buffered EOL is different for Windows
#ifdef LINUX
#define EOL_ '\n'
#else 							
#define EOL_ '\r'
#endif

// backspace is different as well
#ifdef LINUX
#define BCKSPC 0x7F
#else 							
#define BCKSPC 0x08
#endif
#define DEFAULT_PORT 5555
// max hostname and username length
#define NAME_SIZE 256
// assumes 80x25 screen (79 avoids word wrap)
#define CUR_ROW 25
#define MSG_BUFF_SIZE 79
// encrypting and decrypting is done with the same function
#define encrypt(x) xor_crypt((x))
#define decrypt(x) xor_crypt((x))

// arguments for the thread that receives data
typedef struct thread_args
{
	socket_type * s;
	void(*fp)(byte * data, int len);
} thread_args;

void create_server(socket_type * s, unsigned short port); 			// used when we host
void connect_to(socket_type * s, char * ip, unsigned short port); 	// used when we connect
void create_rcv_thread(void); 					// used to create the receiving thread
#ifdef LINUX									
void * receive_thread(void * args);
#else											// the receiving thread
DWORD WINAPI receive_thread(LPVOID args);
#endif
void xor_crypt(byte * msg); 					// basic xor encryption to avoid transferring plain text
void handle_incoming(byte * msg, int len); 		// manages received messages
void receive_remote_name(byte * msg, int len);	// receives the remote host's name
void get_input(void); 							// handle input from the keyboard
void change_username(void); 					// changes your username
void shut_down(socket_type s);					// shuts down a socket
void print_help(void); 							// helps you
void say_goodbye(void);							// lets the other party know you are disconnecting
static void die_with_error(const char * msg);	// quits with a bang

// globals
static socket_type client, server; 			// socket variables
bool make_host, make_client, is_partner_on; // flags
static char * username, * serv_name; 		// username and server name pointers
int uname_len; 								// has length of the username and the prompt
static char hostname[NAME_SIZE]; 			// buffer for the hostname
static char remote_hostname[NAME_SIZE]; 	// buffer for the remote host's name
static unsigned short port; 				// has the port number
static char msg_buff[MSG_BUFF_SIZE + 1]; 	// everything typed in goes here
char * bin_name; 							// points to the name of the program
const char prompt[] = "> "; 				// this is printed after the username

int main (int argc, char * argv[])
{	
	int c;
	
	bin_name = argv[0]; // save the program name
	is_partner_on = false; // we are not connected
#ifdef WINDOWS
	socket_init();
#endif
	gethostname(hostname, NAME_SIZE);
	username = hostname; // make hostname the default username
	port = DEFAULT_PORT;
	
	make_host = make_client = false;
	while (--argc > 0)
	{
		if ((*++argv)[0] == ARG)
		{
			c = *(argv[0] + 1);
			switch (c)
			{
				case PORT:
					if (argc > 1)
						port = atoi(*(argv + 1));
					break;
				case USERNAME:
					if (argc > 1)
						username = *(argv + 1);
					break;
				case CONNECT:
					make_client = true; // do connect
					make_host = false; // don't host
					if (argc > 1)
						serv_name = *(argv + 1);
					else
						die_with_error("no target host");
					break;
				case HOST:
					make_host = true; // do host
					make_client = false; // don't connect
					break;
				case HELP:
					print_help();
#ifdef WINDOWS		// Windows cleanup
					WSACleanup();
#endif
					exit(EXIT_SUCCESS);
				default: // argument is not supported
					fprintf(stderr, "Err: bad argument: %s\n", *argv);
					FOR_HELP;
					die_with_error("");
					break;
			}
		}
	}
	
	if (make_host)
		create_server(&server, port);
	else if (make_client)
		connect_to(&client, serv_name, port);
	else
	{
		fprintf(stderr, "Err: specify -%c or -%c\n", CONNECT, HOST);
		FOR_HELP;
		die_with_error("");
	}
	
	// exchange hostnames
	encrypt(hostname);
	socket_send(&client, (const byte *)hostname, strlen(hostname) + 1);
	decrypt(hostname);
	socket_recv(&client, receive_remote_name);
	
	uname_len = strlen(username) + strlen(prompt); // save the length of these for get_input()
	is_partner_on = true; // we are connected
	display_clear_screen();
	create_rcv_thread();
	
#ifdef LINUX
	non_buff_in(); // make input non-buffered
#else
	display_move_cursor_xy(CUR_ROW, 0);
#endif
	if (make_host) // if we are the server
		printf("%s %s has connected\n", remote_hostname, socket_get_client_ip());
	else
		printf("you are now talking to %s %s\n", remote_hostname, socket_get_server_ip());
	
	// print help
	printf("%c%c <username> to change your username\n", COMMAND, USERNAME);
	printf("%c%c to quit\n\n", COMMAND, QUIT);
	
	// main loop
	while (true) 
	{
		strcpy(msg_buff, username);
		strcat(msg_buff, "> ");
#ifdef LINUX
		display_move_cursor_xy(CUR_ROW, 0);
#endif
		display_print_flush(msg_buff); // print username and prompt without a new line
		get_input(); // read from the keyboard
		encrypt(msg_buff); // encrypt before sending
		socket_send(&client, (const byte *)msg_buff, strlen(msg_buff) + 1);
	}
	
	return 0;
}

void create_server(socket_type * s, unsigned short port)
{
	/* creates the server and waits for connection */
	socket_create(s);
	socket_bind(s, port);
	puts("server created");
	printf("your hostname is: %s\n", hostname);
	puts("listening...");
	socket_listen(s);
	client = socket_accept(s);
	puts("connection accepted");
	
	return;
}

void connect_to(socket_type * s, char * ip, unsigned short port)
{
	/* connects to a server 
	 * ip can point to an ip string or a hostname string */
	socket_create(s);
	printf("connecting to %s...\n", serv_name);
	socket_connect(s, ip, port);
	puts("connection established");

	return;
}

 void create_rcv_thread(void)
{
	/* start the thread that's going to receive incoming communication */
	thread_args * pthr;
	// create the arguments
	if ( (pthr = (thread_args *)malloc(sizeof(*pthr))) == NULL)
		die_with_error("memory allocation failed");
		
	pthr->s = &client;
	pthr->fp = handle_incoming;
#ifdef LINUX 
	// POSIX thread
	pthread_t thread_id;
	if (pthread_create(&thread_id, NULL, receive_thread, (void *)pthr) != 0)
		die_with_error("could not create thread");
#else 
	// Windows thread
	CreateThread(NULL, 0, receive_thread, (LPVOID)pthr, 0, NULL);
#endif
}

#ifdef LINUX
void * receive_thread(void * args)
{
	pthread_detach(pthread_self());
#else
DWORD WINAPI receive_thread(LPVOID args)
{
#endif
	thread_args * thr = (thread_args *)args;
	
	// receive data until there's nothing to receive
	while (socket_recv(thr->s, thr->fp))
		;
	// close socket
	socket_close(&client);

	// free the arguments memory
	free(args);
}

void handle_incoming(byte * msg, int len)
{	
	/* this is the do_with_data function
	 * every message comes here after it's received */
	if (msg[0] != '\0')
	{
		decrypt(msg); // decrypt if not empty
		if (BYEBYE == msg[0])
		{
			// see if partner has said goodbye
			++msg;
			is_partner_on = false;
		}
		
		display_clear_line();
		// system specific printing
#ifdef LINUX
		puts(msg);
		display_move_cursor_xy(CUR_ROW, 0);
		display_print_flush(msg_buff);
#else	
		puts(msg);
		display_print_flush(msg_buff);
#endif
		// go home if partner is gone
		if (!is_partner_on)
		{
#ifdef LINUX
			putchar('\n');
#endif
			shut_down(client);
			if (make_host)
				socket_close(&server);
			
			exit(EXIT_SUCCESS);
		}
	}
	
	return;
}

void receive_remote_name(byte * msg, int len)
{
	/* gets the remote host's name */
	strcpy(remote_hostname, msg);
	decrypt(remote_hostname);
	return;
}

void get_input(void)
{
	/* gets non-buffered input from the keyboard */
	int c, count = uname_len;
	char d;
	
	while ( (c = getch_()) != EOL_ )
	{
		if (BCKSPC == c)
		{
			if (count - 1 < uname_len)
			{
				// don't delete the username part
				putchar(msg_buff[count]);
				continue;
			}
				
		// backspacing is terminal specific
#ifdef LINUX
			display_print_flush("\b\b\b   \b\b\b");
#else
			display_print_flush(" \b");
#endif
			msg_buff[--count] = '\0';
			continue;
		}

		if (isprint(c))
		{
			// if input is printable put it in the buffer and update the string
			msg_buff[count++] = c;
			msg_buff[count] = '\0';
		}
		
		if (MSG_BUFF_SIZE == count)
		{
			putchar(EOL_);
			break;
		}		
	}
	
	// send a null if there is nothing after the username
	if (!sscanf(msg_buff, "%*s %c", &d))
	{
		msg_buff[0] = '\0';
		return;
	}
	
	// check for commands
	if (COMMAND == d)
	{	
		sscanf(msg_buff, "%*s %*c%c", &d);
		switch (d)
		{
			case QUIT: 
				say_goodbye();
				shut_down(client);
				if (make_host)
					socket_close(&server);
				
				exit(EXIT_SUCCESS);
				break;
			case CHANGE_NAME:
				change_username();
				break;
			default:
				break;
		}
	}
#ifdef WINDOWS
	// in Windows we need one more new line
	putchar('\n');
#endif
	return;
}

void change_username(void)
{
	/* changes your username if it's not too big */
	if (strlen(msg_buff) < NAME_SIZE)
	{
		sscanf(msg_buff, "%*s %*c%*c %s", username);
		// update this for get_input()
		uname_len = strlen(username) + strlen(prompt);
	}
	msg_buff[0] = '\0';
	
	return;
}

void shut_down(socket_type s)
{
	/* shuts down socket s */
#ifdef WINDOWS
	socket_shutdown(&s, SD_BOTH);
#else
	socket_shutdown(&s, SHUT_RDWR);
#endif

	return;
}

void say_goodbye(void)
{
	/* executes when user ends the session */
	byte byebye[] = {BYEBYE, '\0'};
	
	strcpy(msg_buff, byebye); // put magic number first
	strcat(msg_buff, username);
	strcat(msg_buff, " has disconnected");
	encrypt(msg_buff);
	socket_send(&client, (const byte *)msg_buff, strlen(msg_buff) + 1);
	
	return;
}

void xor_crypt(byte * msg)
{
	/* basic application level encryption */
	static byte crypt_byte;	
	byte temp, * string = msg;
	
	// if crypt_byte is 0 give it another value
	if (!(crypt_byte = (byte)port))
		crypt_byte = 128;
	
	while (*string != '\0')
	{
		// do not allow any nulls in the string
		if ( (temp = *string ^ crypt_byte) != '\0' )
			*string = temp;
		
		++string;
	}
		
	return;
}

void print_help(void)
{
	/* prints help */
	printf("%s v1.0\n", bin_name);
	printf("%s -%c [-%c <username>] [-%c <port>] to host a chat session\n",
	bin_name, HOST, USERNAME, PORT);
	
	printf("%s -%c <ip/hostname> [-%c <username>] [-%c <port>] to connect to a session\n",
	bin_name, CONNECT, USERNAME, PORT);
	
	printf("%c%c <username> while chatting changes your username and %c%c quits\n",
	COMMAND, USERNAME, COMMAND, QUIT);
	
	return;
}

static void die_with_error(const char * msg)
{
	/* don't crash and burn, only crash 
	 * sometimes it's called with "" because of WSACleanup() */
	if (msg[0])
		fprintf(stderr, "Err: %s\n", msg);
	else
		fputc('\n', stderr);
#ifdef WINDOWS
	WSACleanup();
#endif
	exit(EXIT_FAILURE);
}
