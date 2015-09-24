# cherrypop ![](cherrypoplogo.png)
The decentralized cloud/IaaS, with virtual management.
# Description
A cloud software with no masters or central points. Nodes autodetect other nodes and autodistribute virtual machines
and autodivide up the workload. Also there is no minimum limit for hosts, well, one might be nice. It's perfect for
setting up low-end servers in a cloud or a cloud where you want the most bang for the bucks.

# Requirements
At least/only amd64 with VT-x for now. However you can run without VT-x it's not realistic to do so.

# Usage
This section be filled in more later. To deploy a vm, have a disk image ready and create a machine on one of the nodes from it. When you press OK/done in virt-manager for instance, it will be distributed immediately. It is not recommended to install new VMs on the cloud as if the cloud changes then the VM might be moved and the installation broken. So have an image ready beforehand. Any VM with the prefix ignoreXXX (XXX=name) will not be managed by Cherrypop other than copied to other nodes.

# Installing a Cherrypop node using deb
- Install Ubuntu 14.04 LTS. Update & upgrade & dist-upgrade it after installation is done.
- Install LizardFS and configure a mount. Mkdir /var/lib/libvirt. Link in images e.g.: ln -s /YOURLFSMOUNT/images /var/lib/libvirt/images
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
- Install deb with dpkg -i cherrypop*deb.
- If it didn't install you need to install the dependencies.
sudo apt-get upgrade -f
- Done. Start deploying machines to libvirt and they will distribute to other nodes later when you have more. Remember it's wisest to only deploy ready made harddrive images since nodes might start reorganizing without warning.

# Setting up a Cherrypop node
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

- Compile cherrypop with compile.sh.

- Setup discoveryd.

- Copy init.d/cherrypop to /etc/init.d. Customize it for where you put discoveryd and the rest.

- Run
sudo service cherrypop start

- Done.

# Setup of discoveryd
Mkdir /etc/discoveryd. Edit /etc/discoveryd/myservices and add what localhost has. One service per line. Service is a string with whatever you like.
Create /var/lib/discoveryd and chown it to the owner of the discoveryd process. mkdir /etc/discoveryd/discovery.d/ and add scripts that will be executed when a new service is found. Scripts take two arguments, first host, second service. Done.

# Getting help/community
IRC: Join #cherrypop on freenode.

# Author
Gustav Fransson Nyvell

