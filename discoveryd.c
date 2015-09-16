/*
   discoveryd: A network discovery daemon
   Copyright (C) 2015 Gustav Fransson Nyvell

   This file is part of cherrypop.

   Foobar is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 2 of the License, or
   (at your option) any later version.

   Foobar is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with cherrypop.  If not, see <http://www.gnu.org/licenses/>.
 */

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
#define SCRIPTS_PATH "/etc/discoveryd/discovery.d/"
#define LIFE 2
#define GIVEUP 3

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

struct libvirtentry **parse(struct libvirtentry **stale, struct libvirtentry **giveup) {
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
				strcpy(service, "");
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
				if (strcmp(host, "")) {
					struct libvirtentry *newentry = (struct libvirtentry*)malloc(sizeof(struct libvirtentry));
					memset(newentry, 0, sizeof(struct libvirtentry));
					newentry->service = strdup(service);
					newentry->host = strdup(host);
					newentry->addedat = atoi(addedat);
					if (atoi(addedat)+LIFE >= time(NULL)) { // Exclude stale entries
						virts[virtindex++] = newentry;
						virts[virtindex] = NULL;
					} else {
						if (atoi(addedat)+GIVEUP < time(NULL)) {
							*(giveup++) = newentry;
							*giveup = NULL;
						}
						*(stale++) = newentry;
						*stale = NULL;
					}
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
void dealloclibvirtentryarray(struct libvirtentry **entries, struct libvirtentry *keep, int keepentries) {
	struct libvirtentry **start = entries;
	if (!keepentries) {
		while(*entries != NULL) {
			if (*entries != keep)
				dealloclibvirtentry(*entries);
			entries++;
		}
	}
	free(start);
}
struct libvirtentry **newentries() {
	struct libvirtentry **new = (struct libvirtentry**)malloc(sizeof(struct libvirtentry *)*1024);
	memset(new, 0, sizeof(struct libvirtentry *)*1024);
	*new = NULL;
	return new;
}

struct libvirtentry **excludeinnew(struct libvirtentry **array, struct libvirtentry *b) {
	struct libvirtentry **new = newentries();
	struct libvirtentry **start = new;
	struct libvirtentry **ptr = array;
	while (*ptr != NULL) {
		if (b != *ptr) {
			*(new++) = *ptr;
			*new = NULL;
		}
		ptr++;
	}
	return start;
}

struct libvirtentry **combine(struct libvirtentry **a, struct libvirtentry **b) {
	struct libvirtentry **new = newentries();
	struct libvirtentry **start = new;
	while(*a != NULL) {
		*(new++) = *(a++);
		*new = NULL;
	}
	while(*b != NULL) {
		*(new++) = *(b++);
		*new = NULL;
	}
	return start;
}

struct libvirtentry *findlibvirtserver(char *host, char *service, int *is_stale, struct libvirtentry ***current, struct libvirtentry ***staleout, struct libvirtentry **stale_entry, struct libvirtentry ***giveupout) {
	struct libvirtentry **stale = newentries();
	struct libvirtentry **stale_start = stale;
	struct libvirtentry **giveup = newentries();
	struct libvirtentry **all = parse(stale, giveup);
	*giveupout = giveup;
	*staleout = stale;
	*current = all;
	while (*stale != NULL) {
		struct libvirtentry *entry = *stale;
		if (!strcmp(entry->host, host) && !strcmp(entry->service, service)) {
			*is_stale = 1;
			*stale_entry = entry;
			break;
		}
		stale++;
	}
	struct libvirtentry **start = all;
	if (all == NULL) return NULL;
	while (*all != NULL) {
		struct libvirtentry *entry = *all;
		if (!strcmp(entry->host, host) && !strcmp(entry->service, service)) {
			return entry;
		}
		all++;
	}
	return NULL;
}

size_t count(struct libvirtentry **entries) {
	size_t r=0;
	while (*entries != NULL) r++, entries++;
	return r;
}


void addlibvirtserver(struct libvirtentry **entries, struct libvirtentry *newentry) {
	struct libvirtentry **all = entries;
	struct libvirtentry **cleanptr;
	FILE *fp = fopen(STATE_FILE, "w");
	if (fp) {
		if (all) {
			while (*all != NULL) {/*
				cleanptr = cleanthesestale;
				int next = 0;
				while (*cleanptr != NULL) {
					if (*all == *cleanptr) {
						next = 1;
						break;
					}
					cleanptr++;
				}
				if (!next)*/
					fprintf(fp, "%s %ld %s\n", (*all)->host, (long)((*all)->addedat), (*all)->service);
				all++;
			}
		}
		fprintf(fp, "%s %ld %s\n", newentry->host, (long)(newentry->addedat), newentry->service);
		fsync(fileno(fp));
		fclose(fp);
	}
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
		memset(dgram, 0, sizeof(dgram));
		/* 
		 * Wait for a broadcast message: 
		 */  
		z = recvfrom(s,      /* Socket */  
				dgram,  /* Receiving buffer */  
				sizeof dgram,/* Max rcv buf size */  
				0,      /* Flags: no options */  
				(struct sockaddr *)&adr, /* Addr */  
				&x);    /* Addr len, in & out */  

		if ( z < 0 )  {
			displayError("recvfrom(2)", 0); /* else err */  
			continue;
		}
		dgram[z] = '\0';

		if (!strncmp(dgram, msg, strlen(msg))) {
			char *remote = inet_ntoa(adr.sin_addr);
			if (!strncmp(remote, bc_addr, strlen(remote))) continue; // We don't know who this is.
			char *service = dgram+strlen(msg)+1;
			int is_stale = 0;
			struct libvirtentry **all;
			struct libvirtentry **stale;
			struct libvirtentry *stale_entry=NULL;
			struct libvirtentry **giveups;
			struct libvirtentry *entry = findlibvirtserver(remote, service, &is_stale, &all, &stale, &stale_entry, &giveups);
			struct libvirtentry **work=stale;
			struct libvirtentry **giveupsptr=giveups;
			while(*giveupsptr != NULL) {
				struct libvirtentry **worknew = excludeinnew(work, *giveupsptr);
				dealloclibvirtentry(*giveupsptr);
				dealloclibvirtentryarray(work, NULL, 1);
				work = worknew;
				giveupsptr++;
			}
			stale = work;
			
			if (entry == NULL && is_stale) {
				struct libvirtentry newentry;
				newentry.host = remote;
				newentry.addedat = time(NULL);
				newentry.service = service;
				struct libvirtentry **stalewithnonstale = combine(stale, all);
				struct libvirtentry **stalewithnonstalewithoutfound = stalewithnonstale;
				stalewithnonstalewithoutfound = excludeinnew(stalewithnonstale, stale_entry);
				addlibvirtserver(stalewithnonstalewithoutfound, &newentry);
				if (stalewithnonstale != stalewithnonstalewithoutfound)
					dealloclibvirtentryarray(stalewithnonstalewithoutfound, NULL, 1);
				dealloclibvirtentryarray(stalewithnonstale, NULL, 1);
			} else if (entry == NULL) {
				struct libvirtentry newentry;
				newentry.host = remote;
				newentry.addedat = time(NULL);
				newentry.service = service;
				struct libvirtentry **stalewithnonstale = combine(stale, all);
				addlibvirtserver(stalewithnonstale, &newentry);
				char command[1024];
				sprintf(command, "for i in `ls %s`; do %s$i %s %s; done", SCRIPTS_PATH, SCRIPTS_PATH, newentry.host, newentry.service);
				system(command);
				dealloclibvirtentryarray(stalewithnonstale, NULL, 1);
			}
			dealloclibvirtentryarray(giveups, NULL, 1);
			dealloclibvirtentryarray(all, NULL, 0);
			dealloclibvirtentryarray(stale, NULL, 0);
		}
	}  

	return 0;  
}  
