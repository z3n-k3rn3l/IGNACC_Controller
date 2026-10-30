# IGNACC Tester v1.1.0

A prebuilt Windows GUI tester for the IGNACC_Controller relay board. New in 1.1.0: ALL ON / ALL OFF buttons and tests (needs firmware 1.1.0; with 1.0.0 firmware, use the v1.0.0 tester). It's a single file and runs without Python installed.

| File | What it is |
|---|---|
| `IGNACC_Tester-v1.1.0-windows-x64.exe` | The tester (built with PyInstaller from `tools/ignacc_tester/`) |
| `SHA256SUMS.txt` | Checksum to verify the file |

## Use it

1. Run `IGNACC_Tester-v1.1.0-windows-x64.exe`. The file isn't signed, so Windows SmartScreen may warn. Click **More info → Run anyway**.
2. Close any other program using the board's COM port (PuTTY, Serial Studio, `idf.py monitor`).
3. Pick the COM port and click **Connect**. The status shows the firmware version, e.g. `FW v1.1.0`.
4. Run all tests, the ticked ones, or a range. The step delay (default 1 s) and the latency limit (default 20 ms) are adjustable.
5. Use **Manual control** to send single commands, including **ALL ON** / **ALL OFF**. Both relays are switched OFF after every run, on Stop, and when the window closes.

The board must run IGNACC_Controller firmware. Build and flash that from source with `idf.py -p COMx flash`.

## Verify the download

```powershell
Get-FileHash .\IGNACC_Tester-v1.1.0-windows-x64.exe -Algorithm SHA256
```

Compare the result with `SHA256SUMS.txt`. On Linux, run `sha256sum -c SHA256SUMS.txt`.

## Rebuild

```bash
powershell -ExecutionPolicy Bypass -File tools/ignacc_tester/build_exe.ps1
```

The output goes to `tools/ignacc_tester/dist/IGNACC_Tester.exe`, which git ignores. For a new version, copy it here as `bin/vX.Y.Z/IGNACC_Tester-vX.Y.Z-windows-x64.exe` with a fresh `SHA256SUMS.txt`.
