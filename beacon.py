import socket
import requests

BEACONADDR="http://localhost:6893"
IPECHOADDR="http://ipecho.net/plain"

hostname = socket.gethostname()
extip = requests.get(IPECHOADDR).text
intip = socket.gethostbyname(hostname)

pdata = {"host": hostname, "extip": extip, "intip": intip}
requests.post(BEACONADDR, json=pdata)
