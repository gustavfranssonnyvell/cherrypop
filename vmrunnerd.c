/*
   vmrunnerd: A program to start and stop local vms based on the
		known network as discovered by discoveryd.
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
#include <string.h>
#include <stdlib.h>
#include <libvirt/libvirt.h>
#include <crypt.h>

char **gethosts() {
	char **hosts = (char**)malloc(sizeof(char*)*1024);
	*hosts = NULL;
	char **hostsptr = hosts;
	FILE *fp = fopen("/var/lib/discoveryd/servers", "r");
	if (fp) {
		char buf[1024];
		size_t r = fread(buf, 1, 1024, fp);
		buf[r] = '\0';
		char line[1024];
		memset(line, 0, 1024);
		size_t i=0,j=0,k=0;
		for(i; i<=r; i++,j++) {
			if (buf[i] == '\n' || buf[i] == '\0') {
				strncpy(line, buf+k+(buf[k]=='\n'?1:0), j-(buf[k]=='\n'?1:0));
				line[j] = '\0';

				size_t u=0,ap=-1;
				int hostdone=0, timedone=0;
				char host[128], time[128], service[128];
				memset(host, 0, 128);
				memset(time, 0, 128);
				memset(service, 0, 128);
				for(u; u<=j; u++) {
					if (line[u] == ' ' || line[u] == '\0') {
						char *work;
						if (timedone) {
							work = service;
						} else if (hostdone) {
							work = time;
							timedone = 1;
						} else {
							work = host;
							hostdone = 1;
						}
						ap++;
						strncpy(work, line+ap, u-ap);
						ap = u;
					}
				}
				if (!strcmp(service, "openvirthost")) {
					*(hostsptr++) = strdup(host);
					*hostsptr = NULL;
				}
			
				k+=j;
				memset(line, 0, 1024);
				j=0;
			}
		}
		fclose(fp);
	}
	return hosts;
}

struct hostwithvmhash {
	char *host;
	char *host_hash;
	char *vm;
	char *vm_hash;
	char *combined;
	size_t prio;
};

int cscompare(struct hostwithvmhash *a, struct hostwithvmhash *b) {
	return strcmp((*(struct hostwithvmhash **)a)->combined, (*(struct hostwithvmhash **)b)->combined);
}
int cscompareprio(struct hostwithvmhash *a, struct hostwithvmhash *b) {
	return (*(struct hostwithvmhash **)a)->prio > (*(struct hostwithvmhash **)b)->prio;
}

struct hostwithvmhash **cleanlist(struct hostwithvmhash **list) {
	struct hostwithvmhash **newlist=malloc(sizeof(struct hostwithvmhash *)*1024);;
	struct hostwithvmhash **newlistptr=newlist;
	*newlist = NULL;
	char **seenvms = malloc(1024*sizeof(char*));
	*seenvms = NULL;
	char **seenvmsptr = seenvms, **seenvmsptrpeek = seenvms;

	while (*list != NULL) {
		struct hostwithvmhash *this = *list;
		seenvmsptrpeek = seenvms;
		int its_seen = 0;
		while(*seenvmsptrpeek != NULL) {
			if (!strcmp(*seenvmsptrpeek, this->vm))
				its_seen = 1;
			seenvmsptrpeek++;
		}
		if (its_seen) {
			list++;
			continue;
		}
		*(seenvmsptr++) = this->vm;
		*seenvmsptr = NULL;

		*(newlistptr++) = this;
		*newlistptr = NULL;

		list++;
	}

	return newlist;
	
}

void main(int argc, char *argv[]) {
	char *myIP;
	if (argc<2)
		printf("arg1=myip\n"),exit(-128);
	myIP = argv[1];

	run(myIP);
}

void run(char *myIP) {
	virConnectPtr localhost = virConnectOpen("qemu:///system");
	virDomainPtr *domains, *domainsptr;
	virConnectListAllDomains(localhost, &domains, VIR_CONNECT_LIST_DOMAINS_ACTIVE|VIR_CONNECT_LIST_DOMAINS_INACTIVE|VIR_CONNECT_LIST_DOMAINS_RUNNING|VIR_CONNECT_LIST_DOMAINS_SHUTOFF);
	domainsptr = domains;
	char **domainuuids = malloc(sizeof(char*)*1024);
	char **domainuuidsptr = domainuuids;
	*domainuuids = NULL;
	printf("Domains\n");
	while(*domainsptr != NULL) {
		char buf[VIR_UUID_STRING_BUFLEN];
		virDomainGetUUIDString(*domainsptr, buf);
		char *name = virDomainGetName(*domainsptr);
		if (!strncmp("ignore",name,strlen("ignore"))) {
			domainsptr++;
			continue;
		}
		printf("%s\n", buf);
		*(domainuuidsptr++) = strdup(buf);
		*domainuuidsptr = NULL;
		domainsptr++;
	}
	domainuuidsptr = domainuuids;
	size_t domainuuids_count=0;
	while(*domainuuidsptr != NULL) domainuuidsptr++, domainuuids_count++;
	domainuuidsptr = domainuuids;
	domainsptr = domains;
	int **distance = malloc(sizeof(int)*1024*1024);
	*distance = NULL;
	char **hosts = gethosts();
	char **hostsptr=hosts;
	size_t hosts_count = 0;
	while (*hostsptr != NULL) hosts_count++, hostsptr++;
	hostsptr = hosts;
	struct hostwithvmhash **cs = malloc(sizeof(struct hostwithvmhash)*1024);
	*cs = NULL;
	struct hostwithvmhash **csptr = cs;
	int sign=0;
	while(*hostsptr != NULL) {
		if (!strcmp(*hostsptr, myIP)) printf("ME! ");
		sign = !sign;
		size_t k=1;
		while(*domainuuidsptr != NULL) {
			char *num = "$6$hest";
			char *hash = crypt(*domainuuidsptr, num);
			char *hash2 = crypt(*hostsptr, num);
			struct hostwithvmhash *c=malloc(sizeof(struct hostwithvmhash));
			c->host = *hostsptr;
			c->host_hash = strdup(hash+10);
			c->vm = *domainuuidsptr;
			c->vm_hash = strdup(hash2+10);
			c->combined = malloc(10000);
			strcpy(c->combined, "");
			strcat(c->combined, c->host);
			strcat(c->combined, c->vm);
			//c->combined = strdup(crypt(c->combined, num));


			*(csptr++) = c;
			*csptr = NULL;

			domainuuidsptr++;
		}
		domainuuidsptr = domainuuids;
		printf("%s\n", *hostsptr);
	/*	char URL[512];
		sprintf(URL, "qemu+ssh://%s/system", *hosts);
		virConnectPtr dest = virConnectOpen(URL);
		*(destsptr++) = dest;
		*destsptr = NULL;*/
		hostsptr++;
	}
	csptr = cs;
	
	printf("\n");

	size_t count=0;
	while(*csptr != NULL) count++, csptr++;
	csptr = cs;

	qsort(csptr, count, sizeof(struct hostwithvmhash*), cscompare);

	struct hostwithvmhash **csptrptr = csptr;
	size_t i=0,k=0,j=0,q=1,m=0;
	char *last_host=NULL,*last_vm=NULL;
	struct hostwithvmhash **seen=malloc(sizeof(struct hostwithvmhash*)*1024);
	*seen = NULL;
	struct hostwithvmhash **seenptr=seen;
	struct hostwithvmhash **seenaddptr=seen;
	while(*csptrptr != NULL) {
		if (last_host == NULL || strcmp(last_host, (*csptrptr)->host))
			last_host = (*csptrptr)->host,k++;
		
		seenptr=seen;
		int is_seen=0;
		while(*seenptr != NULL) {
			if (!strcmp((*seenptr)->vm, (*csptrptr)->vm))
				is_seen=1;
			seenptr++;
		}
		if (!is_seen) {
			m++,*(seenaddptr++) = *csptrptr, *seenaddptr = NULL;
			(*csptrptr)->prio = 100*m+(k+m)%hosts_count;
			if (m == domainuuids_count)
				m = 0, seenaddptr=seenptr=seen,*seen=NULL;
		} else
			(*csptrptr)->prio = k;
		csptrptr++;
	}
	qsort(csptr, count, sizeof(struct hostwithvmhash*), cscompareprio);

	struct hostwithvmhash **cleaned = cleanlist(csptr);

	while(*cleaned != NULL) {
		virDomainPtr d = virDomainLookupByUUIDString(localhost, (*cleaned)->vm);
		char *name = virDomainGetName(d);
		if (!strcmp((*cleaned)->host, myIP)) {
			printf("I should be running %s (%s)\n", name, (*cleaned)->vm);
			virDomainCreate(d);
		} else {
			printf("I should NOT be running %s (%s)\n", name, (*cleaned)->vm);
			virDomainDestroy(d);
		}
		//printf("prio:%d host:%s, vm:%s combined:%s\n", (*cleaned)->prio, (*cleaned)->host, (*cleaned)->vm, (*cleaned)->combined);
		cleaned++;
	}
/*
	virDomainPtr *domains;
	virConnectListAllDomains(src, &domains, VIR_CONNECT_LIST_DOMAINS_ACTIVE);
	while(*domains != NULL) {
		char *config = virDomainGetXMLDesc(*domains, 0);
		char uuid[VIR_UUID_BUFLEN];
		char uuidstr[VIR_UUID_STRING_BUFLEN];
		virDomainGetUUIDString(*domains, uuidstr);
		virDomainGetUUID(*domains, uuid);
		destsptr = dests;
		while (*destsptr != NULL) {
			virConnectPtr dest = *destsptr;
			virDomainPtr d = virDomainLookupByUUID(dest, uuid);
			if (d)
				printf("%s already in dest\n", uuidstr);
			if (!d) {
				printf("%s\n", config);
				printf("Injecting domain on dest\n");
				virDomainPtr new = virDomainDefineXML(dest, config);
			}
			fflush(stdout);
			destsptr++;
		}
		domains++;
	}*/
}
