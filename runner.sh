#!/bin/sh
ME=$1
if [ "$ME" = "" ]; then
	echo Need one arg
	exit 
fi

a() {
	b() {
		while /bin/true; do
			for i in `cat /var/lib/discoveryd/servers|cut -f1 -d" "`; do
				HOME=~cherrypop SSHPASS=`cat /etc/cherrypop/password` sshpass -e ssh-copy-id -i ~cherrypop/.ssh/id_rsa $i >/dev/null 2>/dev/null;
			done
			sleep 1
		done
	}
	b &
}

a &
