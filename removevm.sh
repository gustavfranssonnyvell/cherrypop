VMNAME=$1
for i in 192.168.1.68 192.168.1.79; do ssh $i "virsh destroy $VMNAME" & ssh $i "virsh undefine $VMNAME" & done
for i in 192.168.1.68 192.168.1.79; do ssh $i "virsh undefine $VMNAME" & done
