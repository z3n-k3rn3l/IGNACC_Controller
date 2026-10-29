# IGNACC_Controller v1.0.0 binaries

Prebuilt files for v1.0.0: ESP32 firmware that switches two relays (ACC / IGN) on serial commands, plus a Windows GUI tester. They're built from the commit that added this folder, and no build tools are needed to use them.

## Files in this folder

| File | What it is |
|---|---|
| `IGNACC_Tester-v1.0.0-windows-x64.exe` | GUI tester. A single file that runs without installing Python. |
| `IGNACC_Controller-v1.0.0-esp32-merged.bin` | Complete firmware image (bootloader + partition table + app). Flash it at address `0x0`. |
| `IGNACC_Controller-v1.0.0-esp32-parts.zip` | The same firmware as separate images, with `flash_args` (offsets and flash settings). |
| `SHA256SUMS.txt` | Checksums to verify the downloads. |

## Flash the firmware

You need [esptool](https://github.com/espressif/esptool) (`pip install esptool`). Replace `COM15` with your board's port, and close any serial terminal first.

```bash
python -m esptool --chip esp32 -p COM15 -b 460800 write-flash 0x0 IGNACC_Controller-v1.0.0-esp32-merged.bin
```

Or, with the parts zip extracted:

```bash
python -m esptool --chip esp32 -p COM15 -b 460800 write-flash "@flash_args"
```

## Use the tester

1. Run `IGNACC_Tester-v1.0.0-windows-x64.exe`. Windows SmartScreen may warn because the file isn't signed. Click **More info → Run anyway**.
2. Pick the COM port, click **Connect**. The status shows `FW v1.0.0`.
3. Run all tests, the ticked ones, or a range. The step delay and the latency limit (default 20 ms) are adjustable.

## Protocol (115200 8N1, end each line with `\n`)

`IN1_ON`, `IN1_OFF`, `IN2_ON`, `IN2_OFF`, `STATUS`, `HeyU`. Each reply ends with `\r\n`.
Relay commands answer `_OK` after the pin is driven, a 15 ms settle delay has passed, and the pin reads back correctly. Otherwise they answer `_FAIL`, and both relays are switched OFF.

## Hardware notes

- ESP32-WROOM-32 devkit. IN1 is on GPIO26 and IN2 on GPIO27, active-low.
- 5 V relay modules need a **3.3 V → 5 V level shifter** on IN1/IN2. Otherwise the relays stay ON.
- Measured round trip on a CP210x bridge: `STATUS` 4–8 ms, relay commands 18.6–23 ms (including the 15 ms settle).

## Verify downloads

```powershell
Get-FileHash .\IGNACC_Tester-v1.0.0-windows-x64.exe -Algorithm SHA256
```

Compare the result with `SHA256SUMS.txt`. On Linux, run `sha256sum -c SHA256SUMS.txt`.
