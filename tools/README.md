# Tools

## Use this: IGNACC Tester (current)

| | |
|---|---|
| **Ready-to-run app** | [ignacc_tester/bin/v1.2.0/IGNACC_Tester-v1.2.0-windows-x64.exe](ignacc_tester/bin/v1.2.0/IGNACC_Tester-v1.2.0-windows-x64.exe), no Python needed |
| Source | [ignacc_tester/](ignacc_tester/): `ignacc_tester_gui.py` (window), `tester_core.py` (tests + command line) |
| Features | COM port picker, 15 automated tests, ALL ON / ALL OFF, `UNK_NACK` checks, latency check, manual buttons |
| Matches firmware | **1.2.0**. Older versions are kept in `bin/v1.1.0` and `bin/v1.0.0` for older firmware |

On GitHub: open the `.exe` link above, then click **Download raw file**.

How it is built, run and adapted: [docs/toolchain.md](../docs/toolchain.md), section 3.

## Legacy scripts (kept as extra checks)

| Script | What it does |
|---|---|
| `hil_test.py` | The first command-line hardware test (33 checks). Superseded by the IGNACC Tester; still passes |
| `pin_check.py` | Steps IN1/IN2 one at a time, so you can measure the pins with a multimeter |

Both need Python with pyserial and run against the board's COM port:
`python tools/hil_test.py --port COM15`.
