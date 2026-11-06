# Lab-PC guide: get the code, run it on the board, bring the results home

This covers **Part 1 (Ζητούμενο 1)**, the only part that needs the lab PC and the
S7G2 board. Part 2 (gem5) runs on your own computer; see `part2/README.md`.

Takes about 30–40 minutes the first time.

---

## 0. Before you go (on your Mac)

- Make sure everything is pushed:
  ```bash
  cd ~/university/embedded_systems && git status && git push
  ```
- Know your GitHub login (you'll need it at the end to upload the results), or bring
  a USB stick.

---

## 1. Get the code onto the lab PC

The repo is public, so no login is needed to download it.

**Option A: Git (if the PC has Git).** Open *PowerShell* or *Command Prompt*:
```bash
cd %USERPROFILE%\Desktop
git clone https://github.com/Zajason/embedded_systems.git
```
(In PowerShell use `cd $HOME\Desktop`.) If `git` is "not recognized", use option B.

**Option B: ZIP (always works).** In a browser open
`https://github.com/Zajason/embedded_systems` → green **Code** button → **Download ZIP**.
Extract it to the Desktop.

Either way, the files you need are in `embedded_systems\lab\Lab1\src\`:

```
hal_entry.c  images.h  phods_baseline.h  phods_config.h
phods_impl.h  phods_instances.h  phods_runs.h  phods_variants.h
```

---

## 2. Create the e2studio project

Follow the course's *Renesas Starter Guide* (§ "Για τη δημιουργία νέου project"):

1. Open **e2studio**. Pick a workspace folder you can write to (e.g. `Desktop\ws`).
2. **File → New → Renesas C/C++ Project → Renesas Synergy**.
3. **Renesas Synergy C Executable Project** → Next.
4. Name: `lab1_phods` → Next.
5. Board: **S7G2 SK**. Device is filled in automatically (R7FS7G27H3A01CFC).
   Toolchain: GNU ARM Embedded. Debugger: **J-Link ARM** → Next.
6. Template: **Blinky** → **Finish**.
7. The configuration window (`configuration.xml`) opens. Click
   **Generate Project Content** (green arrow, top right) once.

### Q1 screenshot (do it now, it takes 10 s)
In `configuration.xml` open the **Clocks** tab and take a screenshot (Win+Shift+S).
It shows the PLL settings and the ICLK frequency for the Q1 answer.

---

## 3. Put our code into the project

1. In **Project Explorer** expand `lab1_phods → src`.
2. Right-click `hal_entry.c` → **Delete** → OK. Only delete the one in `src/`, not
   anything in `src/synergy_gen`.
3. Open the downloaded `embedded_systems\lab\Lab1\src` folder in Windows Explorer,
   select **all 8 files**, and **drag them onto the `src` folder** in Project Explorer.
   When asked, pick **Copy files** → OK.
4. `src` should now contain the 8 files. **Do not** copy anything from `reference\`:
   that file also defines `hal_entry` and the build would fail.

---

## 4. Turn off compiler optimizations (-O0)

Right-click the **project** → **Properties** → **C/C++ Build → Settings** →
*Tool Settings* tab → **GNU ARM Cross C Compiler → Optimization**:

- **Optimization Level: None (-O0)**, or put `-O0` in *Other optimization flags* as the
  starter guide says.
- Make sure the *Configuration* drop-down at the top is **Debug**.
- **Apply and Close**.

(Setting it only on `hal_entry.c`, like the guide does, is also fine: all PHODS code is
in headers that `hal_entry.c` includes.)

---

## 5. Build

Right-click the project → **Build Project** (or the hammer icon).
The *Console* should end with `Build Finished. 0 errors`.

| If you see… | Do this |
|---|---|
| `implicit declaration of function '__BKPT'` | In `hal_entry.c` replace `__BKPT(0);` with `__asm volatile ("bkpt #0");` |
| `multiple definition of 'hal_entry'` | You copied `exercise1a_original.c` or kept the old `hal_entry.c`: delete it |
| `images.h: No such file` | `images.h` was not copied into `src/` |
| warnings about unused variables | harmless |

---

## 6. Run on the board

1. Plug the USB cable into the board's **DEBUG USB** connector (bottom middle, marked
   in red in the starter guide). The board's power LED lights up.
2. Right-click the project → **Debug As → Renesas GDB Hardware Debugging**.
   - If it asks to switch to the *Debug perspective* → **Yes**.
   - If it asks for a debug configuration, leave the defaults (J-Link, S7G2).
   - The first time, J-Link may ask to update its firmware → OK.
3. The debugger stops at the start (`Reset_Handler` or `main`). Press **Resume (F8)**,
   and again if it stops a second time.
4. In well under a second the program stops at the line
   `__BKPT(0);   /* halts here ... */` in `hal_entry.c`. **That means all measurements
   are done.** (If it never stops, press *Suspend* (pause) and check that it is at that
   line or in `while(1)`.)

---

## 7. Read the results and save them

### 7a. Quick sanity check in the Expressions view
*Window → Show View → Expressions* (if it isn't open). Click **Add new expression** and add:

| expression | expected |
|---|---|
| `g_all_correct` | **1** (every 5×5 version gave the same motion vectors as the original) |
| `g_core_clock_hz` | the CPU clock in Hz, probably 240000000 → Q1 |
| `g_done` | 1 |
| `g_results` | expand it to look at the numbers |

If `g_all_correct` is 0, save the dump anyway (step 7b). The rows with `correct = 0`
show which version is wrong.

### 7b. Dump everything as text (this is what gets analysed)
1. Open the **Console** view. Click the small *Display Selected Console* drop-down
   (monitor icon) and choose the one ending in **arm-none-eabi-gdb** (the GDB console).
2. Click into it and type these lines, pressing Enter after each:
   ```
   set print elements 0
   set print pretty off
   p g_core_clock_hz
   p g_all_correct
   p g_results
   ```
3. Select all the output in the console (click in it, **Ctrl+A**, **Ctrl+C**).
4. Open **Notepad**, paste, and save as
   `embedded_systems\lab\Lab1\results\gdb_dump.txt`.
5. Also take a screenshot of the expanded `g_results` in the Expressions view, as a backup.

### 7b-check. Are the numbers plausible?

Before you leave, compare with what past NTUA students measured on the same board,
exercise and skeleton (public GitHub repos, 2023–2025). The printed
`report/report_reference.pdf` has one past report's full tables (in blue) next to
the empty cells, so you can write your numbers beside them:

| what | expected | from |
|---|---|---|
| `g_core_clock_hz` | **240000000** | all reports |
| `phods_v0_original` (Q2) `avg_us` | **≈ 1820–1850 µs** (≈ 437k–444k cycles) | 1.821 ms and 1.850 ms in two reports (an older version: 1.648 ms) |
| best optimized B = 5 version (Q3) | **≈ 700–800 µs**, i.e. roughly 2.5× faster than v0 | 0.738 ms and 0.729 ms (with *their* optimizations; ours differ, so treat this as a ballpark) |
| square B (Q4): B = 1 / 2 / 5 / 10 | B = 1 slowest, **B = 10 fastest**, steadily decreasing | 1.24 / 0.85 / 0.74 / 0.68 ms and 1.42 / 0.96 / 0.75 / 0.70 ms |
| rectangular (Q5) | wide blocks (**large By**, e.g. By = 10) fastest | both reports |
| `min_us` vs `max_us` of one row | almost equal (difference of ns to a few µs) | all reports |

What to do if something is off:
- **v0 much faster (e.g. < 1000 µs).** Optimizations are probably still on: check `-O0`
  (step 4), rebuild, run again.
- **v0 much slower (several ms).** Check `g_core_clock_hz`. If it isn't 240 MHz, the
  clock configuration differs, so note it and report times using the real clock.
- **`g_all_correct` = 0.** Save the dump anyway and check which row has `correct = 0`.
- **Our v1–v4 won't match their per-step numbers.** Their transformations were different,
  so only v0 (identical code) is directly comparable.
- **Possible surprise with v4.** One past report found that copying the current block into
  a local buffer reduced the time *significantly*, so v4 may well beat v3. If it does,
  do 7c.

### 7c. (Only if v4 was faster than v3) second run for Q4/Q5
In `g_results`, compare `avg_us` of `phods_v3_share` (row 3) and `phods_v4_copy`
(row 4). If **v4 is lower**:
1. Stop the debugger (red square).
2. Open `src/phods_instances.h` and change `#define PHODS_DSE_COPY_BLOCK 0` to `1`.
3. Build, debug, resume, and dump again (7b). Save as `results\gdb_dump_copy.txt`.

### 7d. Optional: more runs
Run the whole thing 2–3 times (Terminate → Debug again) and save
`gdb_dump_run2.txt` etc. That's useful if the numbers jump around.

---

## 8. Bring the results back to your Mac

Pick one:

**A. Upload through the GitHub website (no Git needed)**
1. Go to `https://github.com/Zajason/embedded_systems/tree/main/lab/Lab1/results` and
   log in.
2. **Add file → Upload files**, drag `gdb_dump.txt` (+ screenshots) in.
3. Commit message "lab results", **Commit changes**.
4. **Log out of GitHub** before you leave (lab PCs are shared).

**B. Git from the lab PC**
```bash
cd %USERPROFILE%\Desktop\embedded_systems
git add lab/Lab1/results
git commit -m "Lab 1 board results"
git push
```
`git push` opens a browser window to sign in to GitHub. Afterwards remove the saved
login: *Control Panel → Credential Manager → Windows Credentials → delete the
`git:https://github.com` entry*.

**C. USB stick / email to yourself**
Copy `gdb_dump.txt` and the screenshots, then put them into
`~/university/embedded_systems/lab/Lab1/results/` on the Mac.

---

## 9. Back on the Mac

```bash
cd ~/university/embedded_systems
git pull                      # if you used option A or B
cd lab/Lab1
python3 -m pip install matplotlib
python3 tools/analyze.py results/gdb_dump.txt
```

This prints a table and writes `results/results.csv` plus the plots `q2_q3.png`,
`q4.png`, `q5.png` and `q2_q5_all.png`. It also writes
`report/generated/part1_values.tex`, so rebuilding the report fills every Part 1 table
and plot automatically:
```bash
cd report && tectonic report.tex
```
What is left after that is the written discussion of the board results (the orange
"Εκκρεμεί" boxes in the report). Ask Claude to write it from `results/results.csv`,
then set `\discussiondonetrue` in `report.tex`.

---

## Checklist before leaving the lab

- [ ] `results\gdb_dump.txt` saved and uploaded/copied
- [ ] `g_all_correct` = 1
- [ ] `g_core_clock_hz` noted
- [ ] Clocks-tab screenshot (Q1)
- [ ] (if v4 < v3) `gdb_dump_copy.txt`
- [ ] Logged out of GitHub / credentials removed
