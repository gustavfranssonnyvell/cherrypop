# cherrypop
# Setup
Edit /etc/discoveryd/myservices with what localhost has. One service per line. Service is a string with whatever you like.
Create /var/lib/discoveryd and chown it to the owner of the discoveryd process. Done. Optional: mkdir /etc/discoveryd/discovery.d/ and add scripts that will be executed when a new service is found. Scripts take two arguments, first host, second service.
