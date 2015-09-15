#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <libvirt/libvirt.h>

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

void main() {
	virConnectPtr src = virConnectOpen("qemu:///system");
	virConnectPtr *dests = (virConnectPtr*)malloc(sizeof(virConnectPtr)*1024);
	virConnectPtr *destsptr = dests;
	*dests = NULL;
	char **hosts = gethosts();
	while(*hosts != NULL) {
		printf("Adding vmhost %s\n", *hosts);
		char URL[512];
		sprintf(URL, "qemu+ssh://%s/system", *hosts);
		virConnectPtr dest = virConnectOpen(URL);
		*(destsptr++) = dest;
		*destsptr = NULL;
		hosts++;
	}
	virDomainPtr *domains;
	virConnectListAllDomains(src, &domains, VIR_CONNECT_LIST_DOMAINS_ACTIVE|VIR_CONNECT_LIST_DOMAINS_INACTIVE|VIR_CONNECT_LIST_DOMAINS_RUNNING|VIR_CONNECT_LIST_DOMAINS_SHUTOFF|VIR_CONNECT_LIST_DOMAINS_PERSISTENT|VIR_CONNECT_LIST_DOMAINS_OTHER|VIR_CONNECT_LIST_DOMAINS_TRANSIENT|VIR_CONNECT_LIST_DOMAINS_PAUSED);
	while(*domains != NULL) {
		char *config = virDomainGetXMLDesc(*domains, 0);
		char uuid[VIR_UUID_BUFLEN];
		char uuidstr[VIR_UUID_STRING_BUFLEN];
		virDomainGetUUIDString(*domains, uuidstr);
		virDomainGetUUID(*domains, uuid);
		printf("Domain: %s\n", uuidstr);
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
	}
}
