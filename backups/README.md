# Original firmware backup

`wt32-sc01-plus-original-flash.bin` is a read-only, full 16 MiB flash dump
captured from the connected WT32-SC01 Plus before any firmware was changed.

Chip identified by esptool: ESP32-S3, revision v0.2, 16 MiB flash, 2 MiB PSRAM.
USB interface: `/dev/ttyACM0` (USB-Serial/JTAG).

SHA-256:

```
3772c1bf7d6d2b713973212ddf5c671e3c844a13a8464f675343d9aed4e7f044  wt32-sc01-plus-original-flash.bin
```

Restore only if you intentionally want to replace the current firmware:

```
uvx --from esptool esptool --port /dev/ttyACM0 write-flash 0x0 backups/wt32-sc01-plus-original-flash.bin
```

The dump may contain device-specific settings or credentials. Keep it private.
