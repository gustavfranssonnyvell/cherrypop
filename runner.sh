#!/bin/sh
ME=$1
if [ "$ME" = "" ]; then
	echo Need one arg
	exit 
fi

a() {
	while /bin/true; do
		sudo -u cherrypop /home/cherrypop/vmrunnerd $ME >/dev/null 2>/dev/null
		sudo -u cherrypop /home/cherrypop/distributevms $ME >/dev/null 2>/dev/null
		sleep 1
	done
}

a &
