#!/bin/sh
ME=$1
if [ "$ME" = "" ]; then
	echo Need one arg
	exit 
fi

a() {
	while /bin/true; do
		b() {
		for i in `cat /var/lib/discoveryd/servers|cut -f1 -d" "`; do
			SSHPASS=`cat /etc/cherrypop/password` sshpass -e ssh-copy-id $i >/dev/null 2>/dev/null;
		done
		}
		b &
		/usr/sbin/distributevms $ME >/dev/null 2>/dev/null
		sleep 1
	done
}

a &
