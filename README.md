# esp32-tracker

This project is a tracker running on an ESP32.
It uses Wi-Fi sniffing and Bluetooth Classic scanning to take measurements.
These measurements are sent to the [Central Hub](https://github.com/cs-23-sw-7-06/CentralHub).


## Building

Configure the project with
```
idf.py menuconfig
```

The user configurable stuff is under the `Sniffer Configuration` menu item.
Here Wi-Fi hotspot the tracker connects to can be configured.
The Central Hub instance the tracker should send the measurements to is also configurable here.


Build the project with
```
idf.py build
```

Flash the project with
```
idf.py -p <PORT> flash
```

It should now be visible in the unregistered trackers menu in the Central Hub Web UI.
