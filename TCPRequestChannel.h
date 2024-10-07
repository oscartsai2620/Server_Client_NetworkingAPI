#ifndef _TCPRequestChannel_H_
#define _TCPRequestChannel_H_

#include "common.h"


class TCPRequestChannel {
private:
	/* Since a TCP socket is full-duplex, we need only one
	   This is unlike FIFO that needed one read fd and another for write for each side
	*/
	int sockfd;

public:
	/* Constructor takes 2 arguments:
	   	- ip address
		- port number
	   If the ip address is an empty string, set up the channel for the server side
	   If the address is non-empty, the constructor works for the client side
	   
	   Both cases prepare the sockfd in the respective way so that it can work as a server or client communication endpoint
	*/
	TCPRequestChannel (const std::string _ip_address, const std::string _port_no);

	/* This is used by the server to create a channel out of a newly accepted client socket
	   Note that an accepted client socket is ready for communication
	*/
	TCPRequestChannel (int _sockfd);

	/* Destructor */
	~TCPRequestChannel ();

	/* This is used by the server to accept a connection on the socket 
	   Returns the connection's socket file descriptor which is used in 2nd constructor above
	*/
	int accept_conn ();

	int cread (void* msgbuf, int msgsize);
	int cwrite (void* msgbuf, int msgsize);
};

#endif
