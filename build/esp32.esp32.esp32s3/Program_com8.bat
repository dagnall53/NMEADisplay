esptool.exe --chip esp32s3 --port COM8 --baud 921600  --before default_reset --after hard_reset write_flash  -z --flash_mode dio --flash_freq 80m --flash_size 8MB 0x0 NMEADisplay.ino.bootloader.bin 0x8000 NMEADisplay.ino.partitions.bin 0xe000 boot_app0.bin 0x10000 NMEADisplay.ino.bin
 
