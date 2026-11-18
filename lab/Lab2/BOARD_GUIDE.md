# Lab 2 on the AUP-ZU3: step-by-step

What is already done on the PC (nothing to do for these at the lab):
- All the code: Q2 (DDTs), Q3 (allocators), Q4 (`-DNMS_FLAG`), in `Source_codes/yolo_nms/`.
  All 90 DDT x allocator builds print `validate: OK`.
- Memory footprints: Massif (`results/q1`, `q2`, `q4`) and `-DALLOC_STATS` (`results/q3`, `q4`).

What needs the board: **execution time** and **perf (`l2d_cache_refill`)** for every version.
That is one script, `board_measure.sh`.

Contents:
1. Before the lab: make the bundle
2. Connect to the board (UART), macOS and Windows
3. Get the code on the board (microSD)
4. On the board: set up perf
5. Run everything (all-in-one)
6. Run single things on their own
7. Get the numbers back to the laptop
8. Clean up
9. Troubleshooting

---

## 1. Before the lab: make the bundle

Everything goes over as **one `.tar.gz` file**. A tar keeps the folder layout and the
executable bit of `build.sh` / `board_measure.sh`, which a plain copy to the SD card's FAT
partition would lose.

The bundle is `Source_codes/` (`yolo_nms/`, `ddtr_student/`, `perf_setup/`): `build.sh` needs
`../ddtr_student/include`, so keep the three folders together. Leave out `bin/`: the binaries
are built on the board.

**macOS** (Terminal):
```bash
cd ~/university/embedded_systems/lab/Lab2
COPYFILE_DISABLE=1 tar --no-xattrs --no-mac-metadata --exclude=bin --exclude=m.out --exclude=.DS_Store -czf lab2_board.tar.gz Source_codes
ls -lh lab2_board.tar.gz        # about 8 MB
```
(`COPYFILE_DISABLE=1`, `--no-xattrs` and `--no-mac-metadata` keep macOS extras out of the tar: without them the board prints hundreds of harmless `Ignoring unknown extended header keyword` warnings.)

**Windows** (PowerShell; `tar` is built into Windows 10/11). First get the code:
`git clone https://github.com/Zajason/embedded_systems.git`, or on GitHub *Code → Download ZIP*
and extract it. Then:
```powershell
cd embedded_systems\lab\Lab2
tar --exclude=bin --exclude=m.out -czf lab2_board.tar.gz Source_codes
dir lab2_board.tar.gz
```

---

## 2. Connect to the board (UART)

Hardware (from the AUP-ZU3 guide):
1. BOOT switch on **SD**, microSD card in its slot.
2. USB-C cable from the **PROG UART** port to the laptop.
3. Power to **EXT PWR**. Open the terminal program first (below), then the power switch to **ON**.

Serial settings everywhere: **115200 baud, 8 data bits, no parity, 1 stop bit, no flow control**.
Two serial ports appear (JTAG and UART); the console is usually the one with the bigger number.

### macOS
```bash
ls /dev/cu.usbserial*
screen /dev/cu.usbserial-XXXX 115200      # the one with the bigger number
```
Press Enter. To quit `screen`: **Ctrl+A, then K, then y**.

To keep a log of everything that appears (useful as a backup of the results, section 7):
```bash
screen -L -Logfile ~/lab2_board_log.txt /dev/cu.usbserial-XXXX 115200
```

### Windows
1. Right-click Start → **Device Manager** → *Ports (COM & LPT)* → note the `USB Serial Port (COMx)`
   with the bigger number.
2. Install **PuTTY** (putty.org). Connection type **Serial**, Serial line `COMx`, Speed `115200`.
   Under *Connection → Serial*: 8 data bits, 1 stop bit, parity None, flow control None.
3. For a log (backup of the results): *Session → Logging → All session output*, pick a file.
4. **Open**, press Enter.

### Log in
User `xilinx`, password `xilinx`. If you land on a `ZynqMP>` prompt, type `boot`.

---

## 3. Get the code on the board (microSD)

The SD card has two partitions:
- a small **FAT** partition (boot files): macOS and Windows can read and write it;
- a big **ext4** partition: the Linux system, including `/home/xilinx`. **Neither macOS nor
  Windows can write ext4 on their own.**

So the bundle goes through the FAT partition, and the board copies it to `/home/xilinx`.
This works the same on macOS and Windows.

> **When you plug the SD card into the laptop:**
> - macOS says *"The disk you inserted was not readable by this computer"* → click **Ignore**.
> - Windows says *"You need to format the disk in drive X: before you can use it"* → click
>   **Cancel**.
>
> That is the ext4 partition. **Never format it**: it is the board's operating system.

### 3a. On the laptop
1. Board off (if it was on: `sudo poweroff` on the board, wait, switch off). Take out the microSD.
2. Put it in the laptop. Open the small FAT drive that appears (often called `BOOT` or `PYNQ`).
3. Copy `lab2_board.tar.gz` to the top of that drive. Do not touch the other files on it.
4. Eject it properly (macOS: eject in Finder; Windows: *Safely Remove*), put it back in the board.

### 3b. On the board
Power on, log in over UART (section 2). On the PYNQ image the FAT partition is already mounted
at `/boot` (`lsblk` is not installed; check with `mount | grep mmcblk`):
```
/dev/mmcblk0p2 on / type ext4 (rw,relatime)
/dev/mmcblk0p1 on /boot type vfat (...)
```
If the `vfat` line is missing, mount it yourself and use `/mnt/sd` instead of `/boot` below
(password `xilinx` if `sudo` asks): `sudo mkdir -p /mnt/sd && sudo mount /dev/mmcblk0p1 /mnt/sd`.

Then unpack into `/home/xilinx/lab2`:
```bash
mkdir -p ~/lab2
tar xzf /boot/lab2_board.tar.gz -C ~/lab2
cd ~/lab2/Source_codes/yolo_nms
ls                # build.sh  board_measure.sh  postprocess.cpp  data/  allocators/ ...
g++ --version     # the compiler is there
```
Warnings like *"time stamp ... is in the future"* are harmless (the board's clock is wrong).

**Alternative, if the board has internet** (`ping -c 1 github.com` works): skip the SD card and
`git clone https://github.com/Zajason/embedded_systems.git ~/lab2repo`, then
`cd ~/lab2repo/lab/Lab2/Source_codes/yolo_nms`. Everything else is the same.

---

## 4. On the board: set up perf

```bash
cd ~/lab2/Source_codes/perf_setup
./test_perf.sh
```
- If it ends with `OK: perf works on this board.`, you're done. It will also say that
  `LLC-load-misses` / `LLC-store-misses` are not supported on the Cortex-A53; that is expected,
  and `board_measure.sh` uses `l2d_cache_refill` (the L2 is the last-level cache here).
- If perf is missing:
  ```bash
  ./install_perf.sh
  source ~/.profile
  ./test_perf.sh
  ```
Then go back:
```bash
cd ~/lab2/Source_codes/yolo_nms
```

---

## 5. Run everything (all-in-one)

```bash
cd ~/lab2/Source_codes/yolo_nms
nohup ./board_measure.sh > measure.log 2>&1 &
tail -f measure.log
```
`nohup ... &` keeps it running even if the UART connection drops; `tail -f` shows the progress
(Ctrl+C stops only `tail`, not the measurement). To check later: `tail measure.log`.

What it does, for the 91 versions (Q1 orig, Q2 9 DDTs with glibc, Q3 9 DDTs x 4 allocators,
Q4 `-DNMS_FLAG` 9 DDTs x 5 allocators):
1. builds the `-DVALIDATE` version and checks `validate: OK` (skips the version if not);
2. builds the measurement binary;
3. **time**: 3 repetitions of `runs=20` → mean of each, then mean and variance of the 3;
4. **perf**: `perf stat -r 20 -e l2d_cache_refill` with `runs=1` (one pass over all frames) and
   with `runs=0` (only loading the data; the difference is what `nms()` causes).

Output in `board_results/`:
```
board_results/environment.txt          date, kernel, g++ and perf versions
board_results/q1/summary.csv           one line per binary
board_results/q2/summary.csv
board_results/q3/summary.csv
board_results/q4/summary.csv
board_results/qN/raw/                  every ms= line and every perf output, as printed
board_results/qN/errors.txt            only if a validate failed
```
`summary.csv` columns:
`binary, ddt, allocator, rep1_ms, rep2_ms, rep3_ms, mean_ms, var_ms2, l2d_refill_runs1, l2d_refill_runs0`

**How long:** compiling is the slow part on the Cortex-A53 (182 compiles with validation). It
was under 2 minutes in total on the PC; expect the board to be many times slower, so probably
well over half an hour (an estimate, not measured). To save time:
```bash
nohup ./board_measure.sh --skip-validate > measure.log 2>&1 &    # half the compiles
```
`--skip-validate` is safe: all 90 versions already passed validation on the PC.

A good order if the time is short: first `q1 q2` (to see everything works), then `q3 q4`:
```bash
nohup ./board_measure.sh q1 q2 > measure_q12.log 2>&1 &
nohup ./board_measure.sh q3 q4 > measure_q34.log 2>&1 &     # after the first one finishes
```
(Each part writes its own `board_results/qN/`, and re-running a part overwrites only that part.)

Options:

| command | does |
|---|---|
| `./board_measure.sh` | Q1–Q4: build, validate, measure |
| `./board_measure.sh q2` | only Q2 (any of `q1 q2 q3 q4`, several allowed) |
| `./board_measure.sh --skip-validate` | no `-DVALIDATE` builds |
| `./board_measure.sh --no-build q3` | measure the binaries already in `bin/`, no compiling |

---

## 6. Run single things on their own

All from `~/lab2/Source_codes/yolo_nms`. The names:
- DDTs: `vector single_linked_list double_linked_list roving_single_linked_list
  roving_double_linked_list single_linked_list_no_sentinel double_linked_list_no_sentinel
  roving_single_linked_list_no_sentinel roving_double_linked_list_no_sentinel`
- allocators: `glibc arena first-fit best-fit worst-fit`

**Build** (binary name in brackets):
```bash
./build.sh orig                                    # [bin/nms-orig]
./build.sh double_linked_list glibc                # [bin/nms-double_linked_list-glibc]       Q2
./build.sh double_linked_list arena                # [bin/nms-double_linked_list-arena]       Q3
./build.sh double_linked_list arena -DNMS_FLAG     # [bin/nms-flag-double_linked_list-arena]  Q4
```

**Check correctness** (adds `-validate` to the name):
```bash
./build.sh double_linked_list arena -DVALIDATE
./bin/nms-double_linked_list-arena-validate input=data/detrac_y5s_640.bin    # validate: OK
```

**Time** (prints one `ms=` per run; the second line averages them):
```bash
./bin/nms-double_linked_list-arena input=data/detrac_y5s_640.bin runs=20
./bin/nms-double_linked_list-arena input=data/detrac_y5s_640.bin runs=20 | awk -F= '/^ms=/{s+=$2;n++} END{printf "mean %.4f ms over %d runs\n", s/n, n}'
```

**perf (L2 = last-level cache misses)**:
```bash
perf stat -r 20 -e l2d_cache_refill ./bin/nms-double_linked_list-arena input=data/detrac_y5s_640.bin
perf stat -r 20 -e l2d_cache_refill ./bin/nms-double_linked_list-arena input=data/detrac_y5s_640.bin runs=0   # loading only
```

**Allocator footprint** (only for arena / first-fit / best-fit / worst-fit; already measured on
the PC, the numbers do not depend on the machine):
```bash
./build.sh double_linked_list arena -DALLOC_STATS
./bin/nms-double_linked_list-arena-stats input=data/detrac_y5s_640.bin     # mmap_bytes=... malloc_bytes=...
```

**Massif** is not run on the board: the assignment says Valgrind runs on the PC, and it is done.

---

## 7. Get the numbers back to the laptop

### 7a. On the board
```bash
cd ~/lab2/Source_codes/yolo_nms
tar czf board_results.tar.gz board_results measure*.log
sudo cp board_results.tar.gz /boot/        # the FAT partition (see 3b); sudo password: xilinx
sync
ls -l /boot/board_results.tar.gz
```

### 7b. Backup over the serial console (no SD card needed)
In case the SD route fails, print the summaries; with logging on (section 2) they end up in the
log file on the laptop, or you can copy them from the terminal window:
```bash
for f in board_results/q*/summary.csv; do echo "=== $f"; cat $f; done
```

### 7c. On the laptop
Shut the board down first (section 8), then put the SD card in the laptop (again: **Ignore** /
**Cancel**, never format) and copy `board_results.tar.gz` from the FAT drive.

**macOS:**
```bash
mkdir -p ~/university/embedded_systems/lab/Lab2/results/board
cd ~/university/embedded_systems/lab/Lab2/results/board
cp /Volumes/<FAT-drive-name>/board_results.tar.gz .
tar xzf board_results.tar.gz
```
**Windows (PowerShell):**
```powershell
mkdir embedded_systems\lab\Lab2\results\board
cd embedded_systems\lab\Lab2\results\board
copy E:\board_results.tar.gz .          # E: = the FAT drive
tar xzf board_results.tar.gz
```

You then have `board_results/q1..q4/summary.csv` (in this repository they were moved up to
`Lab2/results/board/q1..q4/`). Together with the
footprints already in `results/q1..q4/`, that is everything needed for the tables and the
Pareto plots (time = `mean_ms`, variance = `var_ms2`, LLC misses = `l2d_refill_runs1`).

---

## 8. Clean up (the lab guide asks for it)

On the board, **after** 7a:
```bash
rm -rf ~/lab2
sudo poweroff
```
Wait until the console stops printing, then switch the board off.

On the laptop, after 7c: delete `lab2_board.tar.gz` and `board_results.tar.gz` from the SD
card's FAT drive, then eject it properly and put it back in the board.

---

## 9. Troubleshooting

| problem | fix |
|---|---|
| empty `screen`/PuTTY window | press Enter; try the other serial port; check power ON and BOOT on SD |
| garbage characters | baud rate is not 115200 |
| macOS: `Cannot open line` / `screen is terminating` | an old `screen` still holds the port: `screen -ls`, then `screen -X -S <id> quit` |
| Windows: `Access denied` on COMx | another program (or a second PuTTY) has the port open |
| `ZynqMP>` prompt | type `boot` |
| stops at `ZynqMP>` with `no mmc device at slot 1` or `No partition table - mmc 0` | U-Boot cannot read the SD card. Unplug the power for ~10 s and reseat the card with the power off. If it persists, try another SD card (ours read fine on a PC but not in the board) |
| `screen` says `[screen is terminating]` | an older `screen` still holds the port: `screen -ls`, then `screen -r <id>` to get back into it |
| `./build.sh: Permission denied` | the executable bit was lost: `chmod +x build.sh board_measure.sh` (or use the tar as in section 1) |
| `perf not found` after installing | `source ~/.profile` |
| perf: `Permission error` / `perf_event_paranoid` | `sudo sh -c 'echo -1 > /proc/sys/kernel/perf_event_paranoid'` |
| `l2d_cache_refill` columns empty in `summary.csv` | the raw perf output is in `board_results/qN/raw/*.perf_runs*.txt`; bring it back anyway |
| `cannot open data/detrac_y5s_640.bin` | run from `~/lab2/Source_codes/yolo_nms` |
| a line in `errors.txt` | that version failed `validate` on the board and was not measured; note which one |
| the run stopped (UART dropped, board rebooted) | re-run only the missing parts, e.g. `./board_measure.sh q4` |
