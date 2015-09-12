/* mkaddr.c 
 * The mkaddr() Subroutine using inet_aton 
 * Make a socket address: 
 */  
#include <stdio.h>  
#include <unistd.h>  
#include <stdlib.h>  
#include <errno.h>  
#include <ctype.h>  
#include <string.h>  
#include <sys/types.h>  
#include <sys/socket.h>  
#include <netinet/in.h>  
#include <arpa/inet.h>  
#include <netdb.h>  

/* 
 * Create an AF_INET Address: 
 * 
 * ARGUMENTS: 
 * 1. addr Ptr to area 
 * where address is 
 * to be placed. 
 * 2. addrlen Ptr to int that 
 * will hold the final 
 * address length. 
 * 3. str_addr The input string 
 * format hostname, and 
 * port. 
 * 4. protocol The input string 
 * indicating the 
 * protocol being used. 
 * NULL implies  tcp . 
 * RETURNS: 
 * 0 Success. 
 * -1 Bad host part. 
 * -2 Bad port part. 
 * 
 * NOTES: 
 *  *  for the host portion of the 
 * address implies INADDR_ANY. 
 * 
 *  *  for the port portion will 
 * imply zero for the port (assign 
 * a port number). 
 * 
 * EXAMPLES: 
 *  www.lwn.net:80  
 *  localhost:telnet  
 *  *:21  
 *  *:*  
 *  ftp.redhat.com:ftp  
 *  sunsite.unc.edu  
 *  sunsite.unc.edu:*  
 */  
int mkaddr(void *addr,  
		int *addrlen,  
		char *str_addr,  
		char *protocol) {  

	char *inp_addr = strdup(str_addr);  
	char *host_part = strtok(inp_addr, ":" );  
	char *port_part = strtok(NULL, "\n" );  
	struct sockaddr_in *ap =  
		(struct sockaddr_in *) addr;  
	struct hostent *hp = NULL;  
	struct servent *sp = NULL;  
	char *cp;  
	long lv;  

	/* 
	 * Set input defaults: 
	 */  
	if ( !host_part ) {  
		host_part =  "*" ;  
	}  
	if ( !port_part ) {  
		port_part =  "*" ;  
	}  
	if ( !protocol ) {  
		protocol =  "tcp" ;  
	}  

	/* 
	 * Initialize the address structure: 
	 */  
	memset(ap,0,*addrlen);  
	ap->sin_family = AF_INET;  
	ap->sin_port = 0;  
	ap->sin_addr.s_addr = INADDR_ANY;  

	/* 
	 * Fill in the host address: 
	 */  
	if ( strcmp(host_part, "*" ) == 0 ) {  
		; /* Leave as INADDR_ANY */  
	}  
	else if ( isdigit(*host_part) ) {  
		/* 
		 * Numeric IP address: 
		 */  
		ap->sin_addr.s_addr =  
			inet_addr(host_part);  
		// if ( ap->sin_addr.s_addr == INADDR_NONE ) {  
		if ( !inet_aton(host_part,&ap->sin_addr) ) {  
			return -1;  
		}  
	}   
		else {  
			/* 
			 * Assume a hostname: 
			 */  
			hp = gethostbyname(host_part);  
			if ( !hp ) {  
				return -1;  
			}  
			if ( hp->h_addrtype != AF_INET ) {  
				return -1;  
			}  
			ap->sin_addr = * (struct in_addr *)  
				hp->h_addr_list[0];  
		}  

		/* 
		 * Process an optional port #: 
		 */  
		if ( !strcmp(port_part, "*" ) ) {  
			/* Leave as wild (zero) */  
		}  
		else if ( isdigit(*port_part) ) {  
			/* 
			 * Process numeric port #: 
			 */  
			lv = strtol(port_part,&cp,10);  
			if ( cp != NULL && *cp ) {  
				return -2;  
			}  
			if ( lv < 0L || lv >= 32768 ) {  
				return -2;  
			}  
			ap->sin_port = htons( (short)lv);  
		}   
		else {  
			/* 
			 * Lookup the service: 
			 */  
			sp = getservbyname( port_part, protocol);  
			if ( !sp ) {  
				return -2;  
			}  
			ap->sin_port = (short) sp->s_port;  
		}  

		/*  
		 * Return address length  
		 */  
		*addrlen = sizeof *ap;  

		free(inp_addr);  
		return 0;  
	}  
