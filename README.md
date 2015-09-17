# cherrypop ![](cherrypoplogo.png)
The decentralized cloud.
# Description
A cloud software with no masters or central points. Nodes autodetect other nodes and autodistribute virtual machines
and autodivide up the workload. Also there is no minimum limit for hosts, well, one might be nice. It's perfect for
setting up low-end servers in a cloud or a cloud where you want the most bang for the bucks.

# Setting up a node
Nice to have: a DNS server and a DHCP server where you can lock in IP addresses for new virtual machines and nodes, also good for LizardFS mfsmaster.

Steps to install:

- Install Ubuntu 14.04 LTS.

- Preferably add a new user such as cherrypop where you run and do the rest.

- Install libvirt-bin.
$ sudo apt-get install libvirt-bin
- Edit /etc/network/interfaces to contain: (exclude the ```)
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
- Install lizardfs-client. See http://www.lizardfs.com

- Mount a lizardfs system on /store.
$ sudo mkdir /store/images

- Install qemu-system-x86.
$ sudo apt-get install qemu-system-x86

- Edit /etc/libvirt/libvirtd.conf and change unix_sock_rw_perms to 0777.

- Link /store/images to /var/lib/libvirt/images.
$ sudo ln -s /store/images /var/lib/libvirt/images

- Copy ssh keys to the new node.
$ ssh-copy-id newhost

- Setup discoveryd.

- Copy init.d/cherrypop to /etc/init.d. Customize it for where you put discoveryd and the rest.

- Run
sudo service cherrypop start

- Done.

# Setup of discoveryd
Mkdir /etc/discoveryd. Edit /etc/discoveryd/myservices and add what localhost has. One service per line. Service is a string with whatever you like.
Create /var/lib/discoveryd and chown it to the owner of the discoveryd process. mkdir /etc/discoveryd/discovery.d/ and add scripts that will be executed when a new service is found. Scripts take two arguments, first host, second service. Done.

# Author
Gustav Fransson Nyvell
