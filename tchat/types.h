/* types are redefined here */
#ifndef TYPES_

#define TYPES_

// abstract the socket variables
#ifdef LINUX
typedef int socket_type;
#else
#include <winsock2.h>
typedef SOCKET socket_type;
#endif
// for shorthand declaration
typedef struct sockaddr_in saddr_in; 
typedef unsigned char byte;

#endif
