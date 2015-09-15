# cherrypop ![](cherrypoplogo.png)
The decentralized cloud.

# Setting up a node
1. Install Ubuntu 14.04 LTS.
2. Install lizardfs-client. See http://www.lizardfs.com
3. Mount a lizardfs system on /store. Mkdir /store/images.
4. Install libvirt-bin and qemu-system-x86. $ sudo apt-get install libvirt-bin qemu-system-x86
5. Link /store/images to /var/lib/libvirt/images. ln -s /store/images /var/lib/libvirt/images
6. Copy ssh keys to the new node. $ ssh-copy-id newhost
7. Setup discoveryd.
8. Add distributevms to crontab. Say every 5 seconds.
9. TODO: add a daemon that determines if (for example, if localhost has the most resources, or some other election system which guarantees only one node is elected) and then boots up a copied vm.

# Setup of discoveryd
Mkdir /etc/discoveryd. Edit /etc/discoveryd/myservices and add what localhost has. One service per line. Service is a string with whatever you like.
Create /var/lib/discoveryd and chown it to the owner of the discoveryd process. mkdir /etc/discoveryd/discovery.d/ and add scripts that will be executed when a new service is found. Scripts take two arguments, first host, second service. Done.
