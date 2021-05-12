rpi-lora-demonstrator
====================
This is a simple demonstrator for LoraWAN Nodes using a Raspberry PI.
It sends CPU temperature on port 1 and Uptime, Load1, Load5 and Load15 on port 2.
Downlink Messages are written to /var/log/rpi-lora-demonstrator.down

Port 1 Uplink
------------
Port 1 only has one payload field. It is a 32 bit value containing the CPU temperature. (read from /sys/class/thermal/thermal_zone0/temp)

Port 2 Uplink
------------
Port 2 consists of 4 payload fields. all are 32 bit values.
- Byte 1 to 4: system uptime
- Byte 5 to 8: load1
- Byte 9 to 12: load5
- Byte 13 to 16: load15

Downlink
--------
not implemented yet

Installing for Raspberry PI
---------------------------

- `sudo apt-get install build-essential git-core wget`
- [bcm2835_library](http://www.airspayce.com/mikem/bcm2835/):
  ```
  # download the latest version of the library (for example):
  wget -O bcm2835.tar.gz http://www.airspayce.com/mikem/bcm2835/bcm2835-1.68.tar.gz
  tar zxvf bcm2835.tar.gz
  cd bcm2835-*
  ./configure
  make
  sudo make check
  sudo make install
  # and very important
  sudo reboot now
  ```
  ```
  git clone --recursive https://github.com/racsnet/rpi-lora-demonstrator.git
  ```
- change APPEUI, DEVEUI and APPKEY in rpi-lora-demonstrator.h
- compile and install
  ```
  make
  sudo make install
  ```

Systemd Service (not implemented yet)
---------------
If everything is working good you can user systemd to start / stop or to check the status of the application.
```
systemctl start rpi-lora-demonstrator
```
```
systemctl stop rpi-lora-demonstrator
```
```
systemctl status rpi-lora-demonstrator
```
