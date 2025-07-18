# Flashing Teensy 4.1 Firmware

This project builds a firmware `.hex` file for Teensy 4.1 using PlatformIO and GitHub Actions.

## Download the Firmware

1. Go to your repository's **Actions** tab on GitHub.
2. Select the latest workflow run (e.g., "Build Teensy 4.1 Firmware").
3. Download the `teensy41-firmware` artifact. This contains the `firmware.hex` file.

## Flash the Firmware to Teensy 4.1

1. **Install Teensy Loader**
   - Download and install the [Teensy Loader](https://www.pjrc.com/teensy/loader.html) for your operating system.

2. **Connect Teensy 4.1 to your computer**
   - Use a USB cable.

3. **Open Teensy Loader**
   - Click `File > Open HEX File...` and select your downloaded `firmware.hex`.

4. **Put Teensy in Bootloader Mode**
   - Press the physical button on the Teensy 4.1 board.

5. **Upload the Firmware**
   - Click the `Program` button in Teensy Loader.
   - Wait for the progress bar to complete. The Teensy will reboot and run your new firmware.

## Troubleshooting
- If the Teensy Loader does not detect your board, try a different USB cable or port.
- Make sure you are using the correct `.hex` file for Teensy 4.1.
- For more help, see the [Teensy Loader documentation](https://www.pjrc.com/teensy/loader.html).

---

**Tip:** You can always rebuild the firmware locally with PlatformIO:

```sh
platformio run --target upload
```

But the GitHub Actions workflow will always provide a downloadable `.hex` file for manual flashing.
