#!/bin/sh
gcc distributevms.c `pkg-config libvirt --libs` -o distributevms
gcc discoveryd.c mkaddr.c -o discoveryd
gcc -g vmrunnerd.c -lcrypt `pkg-config libvirt --libs` -o vmrunnerd
