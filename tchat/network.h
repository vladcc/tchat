/* tcp socket abstraction layer interface */
#ifndef NETWORK_

#define NETWORK_

#ifdef LINUX
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#else
#include <winsock2.h>
#endif

#ifdef WINDOWS
void socket_init(void);
#endif
void socket_create(socket_type * s);
void socket_connect(socket_type * s, const char * ip, unsigned short port);
void socket_send(socket_type * s, const byte * message, int len);
int socket_recv(socket_type * s, void(*do_with_data)(byte * data, int len));
void socket_shutdown(socket_type * s, int how);
void socket_close(socket_type * s);
// server only functions
void socket_bind(socket_type * s, unsigned short port);
void socket_listen(socket_type * s);
socket_type socket_accept(socket_type * serv_s);
// misc
struct in_addr socket_resolve_name(const char * name);
char * socket_get_client_ip(void);
char * socket_get_server_ip(void);

#endif
