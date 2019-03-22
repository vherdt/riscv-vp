 #!/bin/sh
# Bind the USBCAN device
slcand -o -c -f -s6 /dev/ttyUSB_CAN slcan0
sleep 1
ifconfig slcan0 up
