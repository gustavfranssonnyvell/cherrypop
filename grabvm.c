#include <stdlib.h>
#include <libvirt/libvirt.h>

void main() {
	virConnectPtr src = virConnectOpen("qemu:///system");
	virConnectPtr dest = virConnectOpen("qemu+ssh://192.168.1.68/system");
	virDomainPtr *domains;
	virConnectListAllDomains(src, &domains, VIR_CONNECT_LIST_DOMAINS_ACTIVE);
	while(*domains != NULL) {
		printf("%x\n", *domains);
		char uuid[VIR_UUID_BUFLEN];
		char uuidstr[VIR_UUID_STRING_BUFLEN];
		virDomainGetUUIDString(*domains, uuidstr);
		virDomainGetUUID(*domains, uuid);
		virDomainPtr d = virDomainLookupByUUID(dest, uuid);
		if (d)
			printf("%s already in dest\n", uuidstr);
		if (!d) {
			char *config = virDomainGetXMLDesc(*domains, 0);
			printf("%s\n", config);
			printf("%x\n", d);
			printf("Injecting domain on dest\n");
			virDomainPtr new = virDomainDefineXML(dest, config);
		}
		domains++;
	}
}
