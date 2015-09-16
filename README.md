# cherrypop ![](cherrypoplogo.png)
The decentralized cloud.

# Setting up a node
1. Install Ubuntu 14.04 LTS.
2. Edit /etc/network/interfaces to contain:
auto eth0
iface eth0 inet dhcp

auto br-eth0
iface br-eth0 inet dhcp
    bridge_ports eth0
    bridge_fd 5
    bridge_stp off
    bridge_maxwait 1

3. Install lizardfs-client. See http://www.lizardfs.com
4. Mount a lizardfs system on /store. Mkdir /store/images.
5. Install libvirt-bin and qemu-system-x86. $ sudo apt-get install libvirt-bin qemu-system-x86
6. Link /store/images to /var/lib/libvirt/images. ln -s /store/images /var/lib/libvirt/images
7. Copy ssh keys to the new node. $ ssh-copy-id newhost
8. Setup discoveryd.
9. Copy init.d/cherrypop to /etc/init.d. Customize it for where you put discoveryd and the rest.
10. Run sudo service cherrypop start
11. Done.

# Setup of discoveryd
Mkdir /etc/discoveryd. Edit /etc/discoveryd/myservices and add what localhost has. One service per line. Service is a string with whatever you like.
Create /var/lib/discoveryd and chown it to the owner of the discoveryd process. mkdir /etc/discoveryd/discovery.d/ and add scripts that will be executed when a new service is found. Scripts take two arguments, first host, second service. Done.
