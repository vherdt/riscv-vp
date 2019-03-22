```bash
sudo apt-get install can-utils
```
Check if the kernel modules „can“, „can_raw“ and „slcan“ are already loaded:

```bash
lsmod | grep can
```
if not, load them manually:

```bash
sudo modprobe can
sudo modprobe can_raw
sudo modprobe slcan
````

If modprobe says „Module not found“ at this point, your linux distribution is missing the can modules. If modprobe says nothing, the modules exist.
To autoload these modules, insert into new file `/etc/modules-load.d/can` following lines:

```
can
can_raw
slcan
```

(perhaps test by restart, haha)


```bash
sudo cp 90-slcan.rules /etc/udev/rules.d/90-slcan.rules
sudo cp slcan* /usr/local/bin/
```

You may plug in the CAN-Controller now. *kiss*

- If `ip link` does not show a _slcan0_, execute the `sudo slcan_add.sh`
- To see can traffic, run `candump slcan0`
