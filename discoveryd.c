#include <stdio.h>  
#include <unistd.h>  
#include <stdlib.h>  
#include <errno.h>  
#include <string.h>  
#include <time.h>  
#include <signal.h>  
#include <sys/types.h>  
#include <sys/socket.h>  
#include <netinet/in.h>  
#include <arpa/inet.h>  

#define msg "IHAVE"
#define STATE_FILE "/var/lib/discoveryd/servers"
#define MYSERVICES_FILE "/etc/discoveryd/myservices"
#define LIFE 2

#ifndef TRUE
#define TRUE 1
#define FALSE 0
#endif

struct libvirtentry {
	char *host;
	char *service;
	time_t addedat;
};

extern int mkaddr(  
		void *addr,  
		int *addrlen,  
		char *str_addr,  
		char *protocol);  

/* 
 * This function reports the error and 
 * exits back to the shell: 
 */  
static void  
displayError(const char *on_what, int kill) {  
	fputs(strerror(errno),stderr);  
	fputs(": ",stderr);
	fputs(on_what,stderr);
	fputc('\n',stderr);
	if (kill)
		exit(1);
} 

char **getservices() {
	FILE *fp = fopen(MYSERVICES_FILE, "r");
	char **services = (char**)malloc(sizeof(char*)*1024);
	memset(services, 0, sizeof(char*)*1024);
	char **ptr = services;
	if (fp) {
		char buf[1024];
		size_t len = fread(buf, 1, 1024, fp);
		buf[len] = 0;
		char line[512];
		size_t k=0, i=0;
		for(i=0; i<=len; i++) {
			if (buf[i] == '\n' || buf[i] == '\0') {
				line[k] = '\0';
				k=0;
				if (!strcmp(line, "")) continue;
				*(ptr++) = strdup(line);
				*(ptr) = NULL;
			} else {
				line[k++]=buf[i];
			}
		}
		fclose(fp);
	}
	return services;
}

struct libvirtentry **parse() {
	FILE *fp = fopen(STATE_FILE, "r");
	if (fp) {
		char buf[1024];
		size_t len = fread(buf, 1, 1024, fp);
		buf[len] = 0;
		char line[512];
		size_t k=0, i=0, g=0;
		struct libvirtentry **virts = (struct libvirtentry**)malloc(sizeof(struct libvirtentry *)*1024);
		size_t virtindex = 0;
		virts[virtindex] = NULL;
		for(i=0; i<=len; i++) {
			if (buf[i] == '\n' || buf[i] == '\0') {
				line[k] = '\0';
				k=0;
				char host[512], addedat[512], service[512];
				strcpy(host, "");
				strcpy(addedat, "");
				int hostdone=0;
				int addedatdone=0;
				size_t j=0,u=0;
				for(g=0; g<=strlen(line); g++) {
					if (addedatdone) {
						if (line[g] == '\0') {
							service[u] = '\0';
							break;
						} else {
							service[u++] = line[g];
						}
					} else if (hostdone) {
						if (line[g] == ' ') {
							addedat[j] = '\0';
							addedatdone = 1;
						} else {
							addedat[j++] = line[g];
						}
					} else {
						if (line[g] == ' ') {
							hostdone = 1;
							host[g]='\0';
						} else {
							host[g]=line[g];
						}
					}
				}
				if (strcmp(host, "") && atoi(addedat)+LIFE >= time(NULL)) { // Exclude stale entries
					struct libvirtentry *newentry = (struct libvirtentry*)malloc(sizeof(struct libvirtentry));
					memset(newentry, 0, sizeof(struct libvirtentry));
					newentry->service = strdup(service);
					newentry->host = strdup(host);
					newentry->addedat = atoi(addedat);
					virts[virtindex++] = newentry;
					virts[virtindex] = NULL;
				}
			} else {
				line[k++]=buf[i];
			}
		}
		fclose(fp);
		return virts;
	}
	return NULL;
}

void dealloclibvirtentry(struct libvirtentry *entry) {
	free(entry->service);
	free(entry->host);
	free(entry);
}
void dealloclibvirtentryarray(struct libvirtentry **entries, struct libvirtentry *keep) {
	struct libvirtentry **start = entries;
	while(*entries != NULL) {
		if (*entries != keep)
			dealloclibvirtentry(*entries);
		entries++;
	}
	free(start);
}

struct libvirtentry *findlibvirtserver(char *host, char *service) {
	struct libvirtentry **all = parse();
	struct libvirtentry **start = all;
	if (all == NULL) return NULL;
	fflush(stdout);
	while (*all != NULL) {
		struct libvirtentry *entry = *all;
		if (!strcmp(entry->host, host) && !strcmp(entry->service, service)) {
			dealloclibvirtentryarray(start, entry);
			return entry;
		}
		all++;
	}
	dealloclibvirtentryarray(start, NULL);
	return NULL;
}


void addlibvirtserver(struct libvirtentry *newentry) {
	struct libvirtentry **all = parse();
	struct libvirtentry **start = all;
	FILE *fp = fopen(STATE_FILE, "w");
	if (fp) {
		if (all) {
			while (*all != NULL) {
				fprintf(fp, "%s %ld %s\n", (*all)->host, (long)((*all)->addedat), (*all)->service);
				all++;
			}
		}
		fprintf(fp, "%s %ld %s\n", newentry->host, (long)(newentry->addedat), newentry->service);
		fsync(fileno(fp));
		fclose(fp);
	}
	dealloclibvirtentryarray(start, NULL);
}

int  
main(int argc,char **argv) {  
	int z;  
	int x;  
	char bcbuf[512];
	struct sockaddr_in adr;  /* AF_INET */  
	struct sockaddr_in adr_srvr;
	struct sockaddr_in adr_bc;
	static int so_broadcast = TRUE;
	int len_inet;            /* length */  
	int s, transmitsock;                   /* Socket */  
	char dgram[512];         /* Recv buffer */  
	static int so_reuseaddr = TRUE;  
	static char  
		*sv_addr = "192.168.1.80",
		*bc_addr = "192.168.1.255:2500";  
	int len_srvr;
	int len_bc;

	/* 
	 * Use a server address from the command 
	 * line, if one has been provided. 
	 * Otherwise, this program will default 
	 * to using the arbitrary address 
	 * 127.0.0.: 
	 */  
	if ( argc > 1 )  {
		/* Broadcast address: */  
		sv_addr = argv[1];
	}
	if (argc > 2) {
		bc_addr = argv[2];
	}

	/* SERVER */
        len_srvr = sizeof adr_srvr;

        z = mkaddr(
                        &adr_srvr,  /* Returned address */
                        &len_srvr,  /* Returned length */
                        sv_addr,    /* Input string addr */
                        "udp");     /* UDP protocol */

        if ( z == -1 )
                displayError("Bad server address", 1);

        /* 
         * Form the broadcast address: 
         */
        len_bc = sizeof adr_bc;

        z = mkaddr(
                        &adr_bc, /* Returned address */
                        &len_bc, /* Returned length */
                        bc_addr, /* Input string addr */
                        "udp"); /* UDP protocol */

        if ( z == -1 )
                displayError("Bad broadcast address", 1);

        /* 
         * Create a UDP socket to use: 
         */
        transmitsock = socket(AF_INET,SOCK_DGRAM,0);
        if ( transmitsock == -1 )
                displayError("socket()", 1);

        /* 
         * Allow broadcasts: 
         */
        z = setsockopt(transmitsock,
                        SOL_SOCKET,
                        SO_BROADCAST,
                        &so_broadcast,
                        sizeof so_broadcast);

        if ( z == -1 )
                displayError("setsockopt(SO_BROADCAST)", 1);

        /* 
         * Bind an address to our socket, so that 
         * client programs can listen to this 
         * server: 
         */
        z = bind(transmitsock,
                        (struct sockaddr *)&adr_srvr,
                        len_srvr);

        if ( z == -1 )
                displayError("bind()", 1);


	/* /SERVER */


	/* 
	 * Create a UDP socket to use: 
	 */  
	s = socket(AF_INET,SOCK_DGRAM,0);  
	if ( s == -1 )  
		displayError("socket()", 1);  

	/* 
	 * Form the broadcast address: 
	 */  
	len_inet = sizeof adr;  

	z = mkaddr(&adr,  
			&len_inet,  
			bc_addr,  
			"udp");  

	if ( z == -1 )  
		displayError("Bad broadcast address", 1);  

	/* 
	 * Allow multiple listeners on the 
	 * broadcast address: 
	 */  
	z = setsockopt(s,  
			SOL_SOCKET,  
			SO_REUSEADDR,  
			&so_reuseaddr,  
			sizeof so_reuseaddr);  

	if ( z == -1 )  
		displayError("setsockopt(SO_REUSEADDR)", 1);  

	/* 
	 * Bind our socket to the broadcast address: 
	 */  
	z = bind(s,  
			(struct sockaddr *)&adr,  
			len_inet);  

	if ( z == -1 )  
		displayError("bind(2)", 1);  

	if (fork() != 0) { // Go to background, kill foreground process.
		exit(0);
	}

	if (fork() == 0) { // Yes? I'm the child.
		for (;;) {
			// Attention-seeker
			char **myservices = getservices();
			char **ptr = myservices;
			while(*ptr != NULL) {
				memset(bcbuf, 0, sizeof(bcbuf));
				char *service = *(ptr++);
				sprintf(bcbuf, "%s:%s", msg, service);
				free(service);
				/* 
				 * Broadcast the updated info: 
				 */
				z = sendto(transmitsock,
						bcbuf,
						strlen(bcbuf),
						0,
						(struct sockaddr *)&adr_bc,
						len_bc);
			}
			free(myservices);
			sleep(1);
		}
		exit(0);
	}

	for (;;) {  
		/* 
		 * Wait for a broadcast message: 
		 */  
		z = recvfrom(s,      /* Socket */  
				dgram,  /* Receiving buffer */  
				sizeof dgram,/* Max rcv buf size */  
				0,      /* Flags: no options */  
				(struct sockaddr *)&adr, /* Addr */  
				&x);    /* Addr len, in & out */  

		if ( z < 0 )  
			displayError("recvfrom(2)", 0); /* else err */  
		dgram[z] = '\0';

		if (!strncmp(dgram, msg, strlen(msg))) {
			char *remote = inet_ntoa(adr.sin_addr);
			char *service = dgram+strlen(msg)+1;
			struct libvirtentry *entry = findlibvirtserver(remote, service);
			if (entry == NULL) {
				struct libvirtentry newentry;
				newentry.host = remote;
				newentry.addedat = time(NULL);
				newentry.service = service;
				addlibvirtserver(&newentry);
			}
		}
		fflush(stdout);  
	}  

	return 0;  
}  
