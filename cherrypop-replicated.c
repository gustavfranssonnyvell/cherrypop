/*
   cherrypop-replicated: A program to distribute local libvirt vms to other machines
			found by discoveryd.
   Copyright (C) 2015 Gustav Fransson Nyvell

   This file is a part of cherrypop.

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
#include <unistd.h>

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

void run() {
	virConnectPtr src = virConnectOpen("qemu:///system");
	virConnectPtr *dests = (virConnectPtr*)malloc(sizeof(virConnectPtr)*1024);
	virConnectPtr *destsptr = dests;
	*dests = NULL;
	char **hosts = gethosts();
	char **hostsptr = hosts;
	while(*hostsptr != NULL) {
		printf("Adding vmhost %s\n", *hostsptr);
		char URL[512];
		sprintf(URL, "qemu+ssh://%s/system", *hostsptr);
		virConnectPtr dest = virConnectOpen(URL);
		*(destsptr++) = dest;
		*destsptr = NULL;
		hostsptr++;
	}
	hostsptr = hosts;
	while(*hostsptr != NULL) {
		free(*hostsptr);
		hostsptr++;
	}
	free(hosts);
	virDomainPtr *domains, *domainsptr;
	virConnectListAllDomains(src, &domains, VIR_CONNECT_LIST_DOMAINS_ACTIVE|VIR_CONNECT_LIST_DOMAINS_INACTIVE|VIR_CONNECT_LIST_DOMAINS_RUNNING|VIR_CONNECT_LIST_DOMAINS_SHUTOFF|VIR_CONNECT_LIST_DOMAINS_PERSISTENT|VIR_CONNECT_LIST_DOMAINS_OTHER|VIR_CONNECT_LIST_DOMAINS_TRANSIENT|VIR_CONNECT_LIST_DOMAINS_PAUSED);
	domainsptr = domains;
	while(*domainsptr != NULL) {
		char *config = virDomainGetXMLDesc(*domainsptr, 0);
		const char *name = virDomainGetName(*domainsptr);
		char *data = virDomainGetMetadata(*domainsptr, VIR_DOMAIN_METADATA_ELEMENT, "/do_delete", VIR_DOMAIN_AFFECT_CURRENT);
		printf("Data: %s\n", data);
		short delete=0;
		char uuid[VIR_UUID_BUFLEN];
		char uuidstr[VIR_UUID_STRING_BUFLEN];
		if (data && !strcmp(data, "<value>1</value>")) {
			delete = 1;
		}
		virDomainGetUUIDString(*domainsptr, uuidstr);
		virDomainGetUUID(*domainsptr, uuid);
		printf("Domain: %s\n", uuidstr);
		destsptr = dests;
		while (*destsptr != NULL) {
			virConnectPtr dest = *destsptr;
			virDomainPtr d = virDomainLookupByUUID(dest, uuid);
			if (d) {
				printf("%s already in dest\n", uuidstr);
				if (delete) {
					virDomainDestroy(d);
					virDomainUndefine(d);
				}
				virDomainFree(d);
			}
			if (!d && !delete) {
				printf("%s\n", config);
				printf("Injecting domain on dest\n");
				virDomainPtr new = virDomainDefineXML(dest, config);
				virDomainFree(new);
			}
			fflush(stdout);
			destsptr++;
		}
		free(config);
		domainsptr++;
	}
	domainsptr = domains;
	while(*domainsptr != NULL) {
		printf("Free domain\n");
		virDomainFree(*domainsptr);
		domainsptr++;
	}
	free(domains);
	destsptr = dests;
	while(*destsptr != NULL) {
		printf("Free dest\n");
		virConnectClose(*destsptr);
		destsptr++;
	}
	free(dests);
	virConnectClose(src);
}

int main() {
	if (fork() == 0) {
		while(1) {
			run();
			sleep(1);
		}
		return 0;
	}
	return 0;
}
