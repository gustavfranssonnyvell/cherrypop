# cherrypop ![](cherrypoplogo.png)
The decentralized cloud.

# Setting up a node
1. Install Ubuntu 14.04 LTS.
2. Preferably add a new user such as cherrypop where you run and do the rest.
3. Install libvirt-bin.
$ sudo apt-get install libvirt-bin
4. Edit /etc/network/interfaces to contain: (exclude the ```)
```
auto eth0
iface eth0 inet manual

auto br-eth0
iface br-eth0 inet dhcp
    bridge_ports eth0
    bridge_fd 5
    bridge_stp off
    bridge_maxwait 1
```
5. Install lizardfs-client. See http://www.lizardfs.com
6. Mount a lizardfs system on /store.
$ sudo mkdir /store/images
7. Install qemu-system-x86.
$ sudo apt-get install qemu-system-x86
8. Edit /etc/libvirt/libvirtd.conf and change unix_sock_rw_perms to 0777.
9. Link /store/images to /var/lib/libvirt/images.
$ sudo ln -s /store/images /var/lib/libvirt/images
8. Copy ssh keys to the new node.
$ ssh-copy-id newhost
10. Setup discoveryd.
11. Copy init.d/cherrypop to /etc/init.d. Customize it for where you put discoveryd and the rest.
12. Run
sudo service cherrypop start
13. Done.

# Setup of discoveryd
Mkdir /etc/discoveryd. Edit /etc/discoveryd/myservices and add what localhost has. One service per line. Service is a string with whatever you like.
Create /var/lib/discoveryd and chown it to the owner of the discoveryd process. mkdir /etc/discoveryd/discovery.d/ and add scripts that will be executed when a new service is found. Scripts take two arguments, first host, second service. Done.
