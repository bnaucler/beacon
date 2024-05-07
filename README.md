
# beacon
Very simple server / client solution for keeping track of where your clients are located on various networks. Possible implementation to have a static instance running `beacond`, and clients on DHCP connections, possibly behind firewalls, 'reporting home' on a regular basis.

# Server setup
`make` will create the `beacond` binary. Create a systemd service, start & enable. Ensure that the user account running `beacond` has write access to the path `#define`d in the source (normally `/var/log/beacon`) Oh, and make sure to create that directory, too.

A file for each client will be created in this directory as they report in, containing the external and internal IP address.

# Client setup
Copy the client script `beacon.py` to the client and run on regular intervals through cron or a systemd timer. Make sure to set the `BEACONADDR` in the script to the endpoint where `beacond` is running.

# Dependencies
`beacond` requires libjansson.  
`beacon.py` requires python-requests
