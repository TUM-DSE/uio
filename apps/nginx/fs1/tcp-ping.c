#include "unicall_wrapper.h"

#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/time.h>

#define NULL 0

extern int errno;

extern void ushell_puts(char *);
extern int atoi(const char *str);
extern unsigned int sleep(unsigned int seconds);

// lwip_socket is the defacto socket function implemented
// socket does exist but MAY crash if called
extern int lwip_socket(int domain, int type, int protocol);
extern int lwip_connect(int s, const struct sockaddr *name, socklen_t namelen);
extern int lwip_close(int s);

// inet_addr is included under ngx namespace
extern in_addr_t ngx_inet_addr(const char *cp);
#define inet_addr(cp) ngx_inet_addr(cp)

extern uint16_t htons(uint16_t hostshort);

extern int sprintf(char *str, const char *format, ...);

int US_2_SEC = 1000000;
double getTimestamp()
{
	struct timeval tv;
	gettimeofday(&tv, NULL);
	return (double)tv.tv_sec + ((double)tv.tv_usec) / US_2_SEC;
}

// TODO show error codes for the Errors
char msgCreatSocketErr[] =  "FATAL Failed to create socket\n";
char msgConnectionErr[] = "General Exception: Connection failed\n";
char msgOK[] = "TCP ping: %s:%d  %5.2fms\n";

int tcpPing(char *targetServerIP, unsigned short targetServerPort)
{
	int clientSocket = lwip_socket(AF_INET, SOCK_STREAM, 0);
	if (clientSocket < 0) {
		ushell_puts(msgCreatSocketErr);
		return errno;
	}

	struct sockaddr_in server;
	server.sin_addr.s_addr = inet_addr(targetServerIP);
	server.sin_family = AF_INET;
	server.sin_port = htons(targetServerPort);

	double start = getTimestamp();
	// Connect to remote server
	if (lwip_connect(clientSocket, (struct sockaddr *)&server, sizeof(server))
	    < 0) {
		ushell_puts(msgConnectionErr);
		return errno;
	} else {
		double end = getTimestamp();
		//char buffer[256];
		//sprintf(buffer, msgOK, targetServerIP, targetServerPort, end-start);

		//ushell_puts(buffer);
	}

	// clean up
	lwip_close(clientSocket);

	return 0;
}

char msgHello[] = "DEBUG Starting TCP ping tool.\n";
char msgInvalidUsage[] = "Invalid usage. Correct usage: run tcp-ping <ipv4 addr> <port>\n";
char msgInvalidPort[] = "Invalid port.\n";

__attribute__((section(".text"))) int main(int argc, char *argv[])
{
	ushell_puts(msgHello);
	if (argc != 3) {
		ushell_puts(msgInvalidUsage);
		return -1;
	}

	int port = atoi(argv[2]);
	if (port <= 0 || port > 65535) {
		ushell_puts(msgInvalidPort);
		return -1;
	}

	/*while (1) {
		tcpPing(argv[1], port);
		sleep(1);
	}*/
	return tcpPing(argv[1], port);
}