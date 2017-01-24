/*
   deletedom: A network discovery daemon
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

#include <stdlib.h>
#include <stdio.h>
#include <libvirt/libvirt.h>
#include <string.h>

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

int main(int argc, char *argv[]) {
	if(argc <= 1) {
		printf("usage: deletedom domname\n");
		return -1;
	}

	char **hosts = gethosts();
	char **hostsptr = hosts;
	virConnectPtr *dests = (virConnectPtr*)malloc(sizeof(virConnectPtr)*1024);
	virConnectPtr *destsptr = dests;
	while(*hostsptr != NULL) {
		printf("Adding vmhost %s\n", *hostsptr);
		char URL[512];
		sprintf(URL, "qemu+ssh://%s/system", *hostsptr);
		virConnectPtr dest = virConnectOpen(URL);
		*(destsptr++) = dest;
		*destsptr = NULL;
		hostsptr++;
	}

	char *searched_name = argv[1];

        virConnectPtr src = virConnectOpen("qemu:///system");

	virDomainPtr *domains, *domainsptr;
        virConnectListAllDomains(src, &domains, VIR_CONNECT_LIST_DOMAINS_ACTIVE|VIR_CONNECT_LIST_DOMAINS_INACTIVE|VIR_CONNECT_LIST_DOMAINS_RUNNING|VIR_CONNECT_LIST_DOMAINS_SHUTOFF|VIR_CONNECT_LIST_DOMAINS_PERSISTENT|VIR_CONNECT_LIST_DOMAINS_OTHER|VIR_CONNECT_LIST_DOMAINS_TRANSIENT|VIR_CONNECT_LIST_DOMAINS_PAUSED);
        domainsptr = domains;

	while(*domainsptr != NULL) {
		virDomainPtr d = *domainsptr;
		const char *name = virDomainGetName(d);
		
		if (!strcmp(name, searched_name)) {
			printf("Found it!\n");
			destsptr = dests;
			char uuid[VIR_UUID_BUFLEN];
			virDomainGetUUID(*domainsptr, uuid);
			while(*destsptr != NULL) {
				virDomainPtr target=virDomainLookupByUUID(*destsptr, uuid);
				virDomainSetMetadata(target, VIR_DOMAIN_METADATA_ELEMENT, "<value>1</value>", "do_delete", "/do_delete", VIR_DOMAIN_AFFECT_CURRENT);
				destsptr++;
			}
			virDomainSetMetadata(d, VIR_DOMAIN_METADATA_ELEMENT, "<value>1</value>", "do_delete", "/do_delete", VIR_DOMAIN_AFFECT_CURRENT);
			
		}
		domainsptr++;
	}
}
