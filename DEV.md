# MagAO-X Developer Install (Windows 11 + Docker + VSCode, no hardware)

Reproducible steps to build, run, and test the MagAO-X software on Windows with **no instrument
hardware**, inside a Linux container. Two repos are involved:

- `magao-x-setup/` — provisions the **environment image** (dependencies only).
- `MagAOX/` — the **software** (apps, `libMagAOX`, utils, tests), built with `make` in the container.

Target OS is **Rocky Linux 9**. The setup image ships all dependencies but **not** the MagAOX
software — you build that yourself in Step 7.

---

## Conventions

- 🪟 **PowerShell** — a window on the Windows host.
- 🐧 **Container** — a `bash` shell inside the running container.

Each block states its working directory (cwd); it does **not** carry between blocks. Host paths use
`$HOME\Documents\Lazuli` (PowerShell expands `$HOME` to `C:\Users\<you>`) — use any directory, but
be consistent. Container paths like `/opt/MagAOX/...` are fixed; leave them verbatim.

---

## How the pieces persist

| Thing | Name | Lives in | Survives `docker rm`? |
| --- | --- | --- | --- |
| Image (dependencies) | `magaox:cli` | image store | yes |
| Container | `magaox-dev` | container layer | no — recreated from the image |
| Source + build + config | `magaox-opt` → `/opt/MagAOX` | volume | **yes** |
| `/etc` + `~` tweaks (Steps 5, 6.1) | — | container layer | no |

Rule of thumb: anything under **`/opt/MagAOX`** is on the volume and safe; everything else (`/etc`,
`/home`, installed packages) is in the container layer and comes back fresh if you `docker rm` the
container.

**run vs exec vs start:** the container runs one idle `bash` (PID 1) just to stay alive.
`docker run` creates it (once); `docker exec` opens extra shells in it (how you "get in");
`docker start` restarts it after a reboot. Open shells with `bash -l` so `/etc/profile.d/*` — GCC 14,
conda, paths — is loaded.

---

## 0. Prerequisites

- **Windows 11**, ≥ 60 GB free on `C:`, ≥ 8 GB RAM / 4 CPUs available to Docker.
- **Docker Desktop** (WSL2 backend) — install, then wait for **"Engine running."**
- **VSCode** + the **Dev Containers** extension (`ms-vscode-remote.remote-containers`).

Confirm Docker works before the long build:

🪟 **PowerShell** — cwd: *any*

```powershell
docker version             # the server section must not error
docker run --rm hello-world
```

---

## 1. Disable CRLF conversion

The setup scripts must keep LF endings or they break inside Linux (`$'\r': command not found`).
Set this **before** cloning:

🪟 **PowerShell** — cwd: *any*

```powershell
git config --global core.autocrlf false
```

---

## 2. Clone the setup repo

Only `magao-x-setup` is needed on the host (it builds the image). The MagAOX source is cloned
inside the container in Step 5.

🪟 **PowerShell** — cwd: `$HOME`

```powershell
mkdir $HOME\Documents\Lazuli -Force
cd $HOME\Documents\Lazuli
git clone https://github.com/magao-x/magao-x-setup.git
```

---

## 3. Build the environment image

Long and unattended — compiles the full dependency stack into a reusable image. `--target cli` =
headless (no GUI/X), which is all you need for command-line dev.

🪟 **PowerShell** — cwd: `$HOME\Documents\Lazuli\magao-x-setup`

```powershell
cd $HOME\Documents\Lazuli\magao-x-setup
docker build --target cli -t magaox:cli .
docker images magaox        # confirm the cli tag exists
```

- Success ends with `naming to docker.io/library/magaox:cli`; noisy red `bash -lx` trace is normal.
- `No space left on device` → reclaim with `docker system prune -af --volumes`.
- `$'\r'` errors → CRLF leaked in; redo Step 1, re-clone, rebuild.

About this image: GCC 14 is installed (via `gcc-toolset-14`) but isn't the default `gcc`; and
`xsup` can `sudo` but is prompted for a password (`extremeAO!`). Step 5 fixes both.

---

## 4. Start the dev container

Create the container once, with the `magaox-opt` volume holding `/opt/MagAOX`. It runs detached
with an idle `bash` so it stays alive for you to `exec` into.

🪟 **PowerShell** — cwd: *any*

```powershell
docker run -dit --name magaox-dev -v magaox-opt:/opt/MagAOX magaox:cli bash
docker ps --filter name=magaox-dev        # STATUS should say Up
```

`-d` background · `-it` live tty so PID 1 doesn't exit · `--name` stable name · `-v` mount the
volume (Docker creates and seeds it from the image on first use).

---

## 5. One-time root setup

Do the privileged setup as **root** (`-u root`, no password needed). It enables GCC 14, gives
`xsup` passwordless sudo, clones the source, and puts apps on `PATH`.

🪟 **PowerShell** — cwd: *any*

```powershell
docker exec -u root -it magaox-dev bash -l
```

🐧 **Container (root)** — cwd: *any*

```bash
# (a) GCC 14 in every login shell
echo 'source /opt/rh/gcc-toolset-14/enable' > /etc/profile.d/zz-gcc14.sh

# (b) passwordless sudo for xsup (dev only); chmod 440 is required or sudo ignores the file
printf 'xsup ALL=(ALL) NOPASSWD:ALL\n' > /etc/sudoers.d/zzz-xsup-nopasswd
chmod 440 /etc/sudoers.d/zzz-xsup-nopasswd

# (c) clone the source where the build expects it
git clone https://github.com/magao-x/MagAOX.git /opt/MagAOX/source/MagAOX

# (d) let xsup own it, so you can build without sudo
chown -R xsup:magaox /opt/MagAOX/source/MagAOX

# (e) put installed apps on PATH (apps install to /opt/MagAOX/bin, which isn't on PATH by default)
echo 'export PATH=/opt/MagAOX/bin:$PATH' > /etc/profile.d/zz-magaox-bin.sh

exit
```

---

## 6. Open the container in VSCode

1. Command Palette (`Ctrl+Shift+P`) → **Dev Containers: Attach to Running Container** →
   **`magaox-dev`** (attaches as `xsup`).
2. **File → Open Folder** → `/opt/MagAOX/source/MagAOX`.
3. Open a terminal (`` Ctrl+` ``) — all 🐧 blocks below run here.

### 6.1 Load the dev environment in every terminal

VSCode terminals are **non-login** shells, so they skip `/etc/profile.d/*` — no GCC 14, no `xpy`,
no app `PATH`. (VSCode's login-shell profile setting is unreliable in attach mode, so don't rely on
it.) Instead, source `profile.d` from the system rc so **every** interactive shell gets the full
environment — the same approach the MagAO-X repo uses for Ubuntu GUI terminals:

🐧 **Container** — cwd: *any*

```bash
echo 'for f in /etc/profile.d/*.sh; do [ -r "$f" ] && . "$f"; done' | sudo tee -a /etc/bashrc
source /etc/bashrc
```

Verify in a **new** terminal:

```bash
gcc --version | head -1     # 14.x
type xpy                    # "xpy is a function"
```

- `shopt login_shell` stays `off` — fine; you have the *environment*, which is what matters.
- **Conda activates on demand:** `$CONDA_DEFAULT_ENV` is empty until you run **`xpy`**. C++ work
  doesn't need it — run `xpy` only when you need the Python tools.

---

## 7. Build and install MagAOX

Two commands. `make` builds flatlogs → libs → apps in the right order automatically; `make install`
copies binaries to `/opt/MagAOX/bin`.

🐧 **Container** — cwd: `/opt/MagAOX/source/MagAOX`

```bash
cd /opt/MagAOX/source/MagAOX
make ALL_APPS=1
make install ALL_APPS=1
```

- `ALL_APPS=1` builds every app that can compile without hardware SDKs (omit it and only 5 "basic"
  apps build). Pass it to **both** commands.
- **Do not write `sudo make install`.** The Makefile escalates internally for the privileged copy
  steps. Running the whole thing under `sudo` sanitizes `PATH` to system GCC 11 and the build aborts
  with *"Detected GCC 11; GCC >= 14 is required."*
- First build is long; later builds are incremental.

---

## 8. Verify

🐧 **Container** — cwd: `/opt/MagAOX/source/MagAOX`

```bash
whoami                          # xsup
gcc --version | head -1         # 14.x
ls /opt/MagAOX/bin              # many binaries
command -v xindiserver          # /opt/MagAOX/bin/xindiserver  -> apps on PATH (Step 5e)
/opt/MagAOX/bin/xindiserver -h  # prints usage
```

If `xindiserver -h` prints usage, the install worked.

---

## 9. Run tests and check coverage

The repo ships its **own** coverage tooling in `tests/coverage/`, and the CI runs it. To get numbers
that match the pipeline, use those same scripts rather than a custom wrapper:

| Script (run from the repo root) | What it does |
| --- | --- |
| `tests/coverage/make_coverage` | build everything + all tests, run the whole suite, render the report — the **full baseline**, exactly what CI does |
| `tests/coverage/make_coverage_fast` | re-render the report from the **existing** `.gcda` (no rebuild/rerun) — for fast iteration |

Both call `tests/coverage/update_coverage`, which captures with `lcov` and renders with
`genhtml --merge-aliases --suppress-aliases --filter function` — the same invocation as the
Coverage workflow.

### 9.1 One-time setup

These scripts need an **lcov newer than 2.0** — the repo's `update_coverage` calls
`genhtml --merge-aliases`, which the v2.0 release doesn't have (and the stock image's lcov 1.14 is
far older). So build the latest from source. Also patch the logger tests (the full suite needs them).

🐧 **Container** — cwd: *any*

```bash
# Build deps for lcov + the man pages its `make install` builds:
sudo dnf install -y perl-Capture-Tiny perl-DateTime perl-JSON perl-Time-HiRes perl-Digest-MD5 \
                    perl-PerlIO-gzip perl-Module-Load-Conditional git make \
                    python3-sphinx python3-sphinx_rtd_theme

# Latest lcov from the canonical source (>2.0 -> has --merge-aliases); installs to /usr/local/bin
# rm first: a stale clone here (e.g. an older v2.0 tree) would make `make install` reinstall the OLD
# lcov — and 2.0 rejects `--ignore-errors path`, which the repo's update_coverage uses.
sudo rm -rf /tmp/lcov-src
sudo git clone --depth 1 https://github.com/linux-test-project/lcov.git /tmp/lcov-src
sudo make -C /tmp/lcov-src install
lcov --version        # confirm: LCOV version 2.5.x (must be > 2.0)

# Patch two upstream bugs in the logger generated tests (the full suite needs them):
#   bug 1: hardcoded python3 is 3.9, the generators need >=3.12  -> use the conda interpreter
#   bug 2: the test compile omits mxlib's cflags, so <fitsio.h> isn't found  -> add them
( cd /opt/MagAOX/source/MagAOX/libMagAOX/logger/tests
  grep -q MXLIB_CFLAGS Makefile || {     # skip if already patched -- the sed below is not idempotent
    sed -i -e '/^C_STD=c++20$/a MXLIB_CFLAGS := $(shell pkg-config --cflags mxlib)' \
           -e '/^C_STD=c++20$/a include ../../../Make/python.mk' Makefile
    sed -i 's/python3 generate/$(PYTHON) generate/g' Makefile
    sed -i 's/-std=$(C_STD) $(CXXFLAGS)/-std=$(C_STD) $(CXXFLAGS) $(MXLIB_CFLAGS)/g' Makefile
  } )

# Headless fix: the top-level `coverage_clean` cleans the GUI apps (which need qmake/Qt, absent on
# the `cli` image) even though the coverage *build* already skips them (`NO_GUIS=1`). Add it to the
# clean too, so `make coverage` / `tests/coverage/make_coverage` run without qmake:
sed -i 's#all_clean COVERAGE=1 ALL_APPS=1$#all_clean COVERAGE=1 ALL_APPS=1 NO_GUIS=1#' \
    /opt/MagAOX/source/MagAOX/Makefile
```

> The logger patch is in the source tree on the volume, so it survives `docker rm`. lcov installs
> into the container layer (`/usr/local/bin`) — **re-run the lcov install after a `docker rm` +
> recreate.**
>
> ⚠ Both seds patch **tracked files**, so any git operation that rewrites them (merge, checkout,
> restore, pull) silently reverts the patches — `make_coverage` then dies in `coverage_clean` with
> *"No qmake found on PATH"*. Check with `grep NO_GUIS=1 Makefile` and re-run the sed if it's gone.
> Don't commit the patched Makefile — the MR deliberately leaves it untouched.

### 9.2 Full-repo coverage (matches CI)

After Step 7, from the repo root, build everything + all tests, run the whole suite, and render the
report — the exact sequence the Coverage pipeline runs:

🐧 **Container** — cwd: `/opt/MagAOX/source/MagAOX`

```bash
cd /opt/MagAOX/source/MagAOX
tests/coverage/make_coverage
```

View it (run from your **VSCode terminal** so the port forwards to your browser); leave the server
running and refresh after each update below:

```bash
python3 -m http.server 8000 --directory /opt/MagAOX/source/MagAOX/coverage_report
```

Open **`http://localhost:8000/`** — red = not covered; directories are relative to the source root.

> **Some red lines are tooling artifacts, not gaps — and the CI report has them too.** With the
> repo's pipeline you'll still see red on: a closing `}` after a `return` (gcov maps no instruction
> there); an argument line that's *only* arithmetic (`x + 1` on its own line); and lines in the
> heavily-templated headers (`stdFileName.hpp`, `stdSubDir.hpp`) reached only through the
> `XWCTEST_*` multi-include aliases — `--merge-aliases` fixes the *function* counts but not those
> *line* counts. These are inherent to the project's test pattern, so the official report shows the
> same. **Don't `LCOV_EXCL` them** — your numbers already match CI.

### 9.3 Fast iteration (rebuild only what changed)

`make_coverage` reruns the whole suite (slow). While iterating, rebuild + run just the test(s) you
changed, then re-render the report from the existing `.gcda` — **without** rerunning everything:

🐧 **Container** — cwd: `/opt/MagAOX/source/MagAOX`

```bash
# 1. rebuild + run only the changed test(s) (repeat the t=… / run pair per test):
( cd tests && make -f Makefile.one COVERAGE=1 t=../libMagAOX/common/tests/common_exceptions_test \
            && ../libMagAOX/common/tests/common_exceptions_test )

# 2. re-render the full report from all existing .gcda (no suite rerun):
tests/coverage/make_coverage_fast
```

Refresh the browser. Loop 1→2 as you work; run `make_coverage` again for a guaranteed-clean rebuild.

> **The fast path is for *test-file* edits only.** `make_coverage_fast` re-renders from the existing
> `.gcda`. If you edit a **header** (anything in `libMagAOX/`), every test that includes it now has
> stale `.gcda` (gcno/gcda mismatch) — re-run the full `tests/coverage/make_coverage` to get accurate
> numbers.

List the test sources you've changed (formatted as `t=` arguments — `M` = modified, `??` = new):

```bash
cd /opt/MagAOX/source/MagAOX
git status --short | awk '{print $NF}' | grep -E '/tests/[^/]*_test\.cpp$' | sed 's#^#../#; s#\.cpp$##'
```

### 9.4 Test results (pass/fail)

Coverage (above) is *which lines ran*; **results** are *which tests passed* — separate, from the
Catch2 output when you run a test. No coverage build needed.

**One test — console summary:**

🐧 **Container** — cwd: `/opt/MagAOX/source/MagAOX/tests`

```bash
make -f Makefile.one t=../libMagAOX/common/tests/common_exceptions_test   # build (no COVERAGE)
../libMagAOX/common/tests/common_exceptions_test                          # "All tests passed (N assertions ...)"
```

Flags: `-s` (assertion detail), `-r compact` (one line per test), `[tag]` (run only a tag),
`--list-tests`.

**Whole suite — capture + grep:**

```bash
cd /opt/MagAOX/source/MagAOX/tests
bash testMagAOX.bash 2>&1 | tee /tmp/results.log
grep -iE "All tests passed|[0-9]+ failed|FAILED:" /tmp/results.log
```

**Structured (JUnit XML)** — for a Test-Explorer / CI view: `<test-binary> -r junit -o /tmp/results.xml`.

### 9.5 Known caveat — not every test builds on this headless image

~95 of 110 tests in `tests.list` build, run, and contribute coverage; **15 cannot build here** and
are absent from the report (pre-existing, not caused by this setup):

- **Missing optional libraries** (9): the `xInstGraph` family (needs `instGraph`), `usbtempMon`
  (`DS18B20`), `loPredCtrl` (`DDSPC`) — not in the headless `cli` image.
- **Test source that doesn't compile on GCC 14** (6): `xindiserver`, `sshDigger`, `tcsInterface`,
  `siglentSDG`, `zaberLowLevel`, `timeSeriesSimulator` — upstream test bugs.

So "full-repo" here means every test that **builds** on this image, not all 110.

> **A file's coverage looks wrong / lower than expected?** First check its test is actually listed in
> `tests/tests.list` — a test that isn't listed never builds or runs, so the file only gets the
> partial view from whatever *other* tests happen to include it (this is what made `exceptions.hpp`
> read 77.8% instead of 100%). Then re-run the **full** `tests/coverage/make_coverage`:
> `make_coverage_fast` re-renders from whatever `.gcda` exist, so a stale or partial set can collapse
> a file's numbers. A clean full run (zero counters → whole suite → capture) merges it correctly.

### 9.6 Faster builds (parallel)

The test **build** is the slow part. `tests/Makefile` builds tests **in parallel** with `-jN` (the
stock upstream version built them one at a time, on a single core). On 8 cores a clean test build is
~5× faster.

🐧 **Container** — cwd: `/opt/MagAOX/source/MagAOX/tests`

```bash
make -j"$(nproc)"             # build every test across all cores
make -j"$(nproc)" coverage    # parallel coverage build of every test
```

For the full-repo report, parallelize **only the tests build**. The **main tree build (libs/apps) is
not `-j`-safe** — each app's `Make/magAOX.mk` *generates and then `rm`s* a single shared
`magaox_git_version.h` at the repo root, so under `-j` one build deletes it while another is still
compiling ("No such file"). So scope `-j` to `tests`, keep the main build serial:

```bash
cd /opt/MagAOX/source/MagAOX
make coverage                               # main tree — SERIAL (do NOT add -j here)
( cd tests && make -j"$(nproc)" coverage )   # all tests — parallel (the slow part)
tests/coverage/update_coverage              # run the suite + render the report
```

> ⚠ **Don't** wrap the whole pipeline in `MAKEFLAGS="-j…" tests/coverage/make_coverage` — that pushes
> `-j` into the main build and fails on the shared-header race. Only the `tests` build is `-j`-safe
> (that's what these changes fixed). Hardening the main tree for `-j` is a separate, larger upstream fix.

The tests-side speedup rests on two small changes (both PR-worthy):
- **`tests/Makefile`** exposes one `build@<test>` goal per test (real prerequisites, deduped with
  `$(sort)`) instead of a serial `for` loop, so `make -jN` schedules them concurrently; the shared
  `testMain.o` is built once up front. A failed test build is **skipped** (printed as `SKIP`), not
  fatal — matching the old loop's tolerance of the 15 un-buildable tests (§9.5).
- **`Makefile.one`** writes the generated `magaox_git_version.h` atomically (temp file + `mv`), so
  concurrent test builds can't race on it (the recipe regenerates it on every build).

Optional extra speedups (not installed here): **`ccache`** (`sudo dnf install -y ccache`, then build
with `CXX="ccache g++"`) for near-instant rebuilds after edits, and a **faster linker**
(`sudo dnf install -y lld`, then `LDFLAGS+=-fuse-ld=lld`) since each of ~120 tests links
`libMagAOX.a`.

### 9.7 After a coverage build, plain `make install` fails to link

A coverage run (`make coverage` / `tests/coverage/make_coverage`) rebuilds `libMagAOX.a`,
`INDI/libcommon/libcommon.a`, and `libs/libtelnet/libtelnet.a` **with gcov instrumentation**
and leaves them that way. A later plain `make ALL_APPS=1` / `make install ALL_APPS=1` links
apps *without* `--coverage` against those instrumented archives and dies with:

```
undefined reference to `__gcov_init' / `__gcov_exit' / `__gcov_merge_add'
```

(The first app alphabetically — `adcTracker` — is where it surfaces.) Two ways out:

- **Staying in coverage work** (cheapest — no clean, incremental): pass `COVERAGE=1` to the
  app build too, so the link line gains `--coverage`:

  ```bash
  make ALL_APPS=1 COVERAGE=1
  make install ALL_APPS=1 COVERAGE=1
  ```

  The installed binaries are instrumented — running them drops `.gcda` files into the source
  tree (harmless untracked clutter; never `git add -A`).

- **Clean production install**: rebuild the instrumented archives without coverage first,
  then redo the normal install. (The next `make_coverage` re-instruments them anyway, so
  only do this if you actually need clean binaries now.)

  ```bash
  make -C libMagAOX clean && make -C libMagAOX
  make -C INDI/libcommon clean
  make -C libs/libtelnet clean
  make ALL_APPS=1 && make install ALL_APPS=1
  ```

To check which state an archive is in: `nm libMagAOX/libMagAOX.a | grep -q __gcov_init && echo instrumented`.
Note the coverage pipeline never uses the installed apps — if install isn't needed for what
you're doing next, just skip it.

---

## 10. Rebuild after editing

Pick the smallest rebuild that covers your change (run from the VSCode terminal):

🐧 **Container** — cwd: `/opt/MagAOX/source/MagAOX`

```bash
make all ALL_APPS=1 && make install ALL_APPS=1     # whole tree, incremental
```

- **One app only** (fastest): `cd apps/<name> && make && make install`.
- **Debug build** (`-O0 -g`, for gdb/valgrind): `cd apps/<name> && make debug`.
- **Clean rebuild** if the build misbehaves: `make clean` (or `make all_clean` to also rebuild INDI,
  flatlogs, and the precompiled header), then rebuild.

No leading `sudo` on `make install` (see Step 7). Editing `libMagAOX` (header-only) recompiles its
precompiled header and everything that includes it — expect a longer build.

> `undefined reference to __gcov_init` at link → the libs are still in their coverage build
> state from an earlier coverage run; see §9.7.

---

## 11. Run an app (no hardware)

Use the full path `/opt/MagAOX/bin/<app>` (or the bare name once Step 6.1's `PATH` is loaded).
Without hardware, exercise the simulator apps (`timeSeriesSimulator`, `cameraSim`, `aoSim`).

🐧 **Container** — cwd: *any*

```bash
/opt/MagAOX/bin/xindiserver -h                              # inspect options (no side effects)
/opt/MagAOX/bin/timeSeriesSimulator -n timeSeriesSimulator  # run; -n sets the config name; Ctrl-C to stop
ls /opt/MagAOX/config                                       # per-app config files
```

Logs are binary — read them with `logdump` (`-f` to follow a running app):

```bash
logdump timeSeriesSimulator
```

If an app won't restart after a crash, clear its stale lock/FIFO:

```bash
ls /opt/MagAOX/sys              # PID/lock files
ls /opt/MagAOX/drivers/fifos    # INDI FIFOs
```

---

## 12. Stop and clean up

Pick what you need (these are independent, not a sequence):

🪟 **PowerShell** — cwd: *any*

```powershell
docker stop magaox-dev          # stop, keep everything
docker rm -f magaox-dev         # delete container (source/build survive on the volume)
docker volume rm magaox-opt     # also wipe source/build/config — commit/push first!
docker image rm magaox:cli      # delete the image (redo Step 3 to rebuild it)
```

After `docker rm` (volume kept), redo Step 5's `/etc` drop-ins and Step 6.1 — the clone and build on
the volume are intact.

---

## 13. Resume after the machine was off

Nothing is deleted on shutdown. Start Docker Desktop, then:

🪟 **PowerShell** — cwd: *any*

```powershell
docker start magaox-dev                # restart the existing container (NOT docker run)
docker exec -it magaox-dev bash -l     # or reattach in VSCode
```

Then continue — `make` is incremental:

🐧 **Container** — cwd: `/opt/MagAOX/source/MagAOX`

```bash
cd /opt/MagAOX/source/MagAOX
git pull
make all ALL_APPS=1 && make install ALL_APPS=1   # add COVERAGE=1 to both if the libs are
                                                 # still coverage-instrumented (§9.7)
```

Optional: `docker update --restart unless-stopped magaox-dev` makes the container auto-start with
Docker.

**What to redo after each kind of reset:**

| You did | Redo |
| --- | --- |
| Reboot / Docker restart (container stopped) | `docker start` + reconnect. Nothing else. |
| `docker rm magaox-dev` (volume kept) | Step 4 + Step 5 `/etc` drop-ins + Step 6.1. |
| `docker volume rm magaox-opt` | Step 4 + all of Step 5 (re-clone) + Step 7 (rebuild). |
| `docker image rm magaox:cli` | Step 3 (rebuild image) + everything after. |

---

## 14. Push your changes to GitHub (branches & forks)

The container's clone has `origin` = the canonical `magao-x/MagAOX`, and you start on its `dev`
branch. VSCode forwards your GitHub login to git — but **only inside the VSCode-integrated terminal**
(a plain `docker exec` shell has no credentials and fails with *"could not read Username"*), so run
every `git push` from the VSCode terminal.

🐧 **Container (VSCode terminal)** — cwd: `/opt/MagAOX/source/MagAOX`

**1. Make a branch — don't commit on `dev`.** The repo's convention is `username/topic`:

```bash
cd /opt/MagAOX/source/MagAOX
git checkout -b yourname/topic        # carries your uncommitted changes onto the new branch
```

**2. Stage the real changes; skip generated / build artifacts:**

```bash
git status
git checkout -- tests/.fftw_wisdom.float        # discard build artifacts; never commit generated files
git add libMagAOX/ tests/Makefile tests/Makefile.one tests/tests.list Makefile
git commit -m "Describe your change"
```

**3. Push.** First check whether you can push to the upstream — the dry-run creates nothing:

```bash
git push --dry-run origin dev:refs/heads/zz-permcheck-delete-me
```

- **Succeeds** → you have write access; push your branch straight to upstream:
  ```bash
  git push -u origin yourname/topic
  ```
- **`403` / permission denied** → you don't; use a **fork** (works for anyone):
  1. On github.com, open `magao-x/MagAOX` → **Fork** (creates `github.com/<you>/MagAOX`).
  2. Add it as a second remote and push there:
     ```bash
     git remote add <you> https://github.com/<you>/MagAOX.git   # any name except "origin"
     git push -u <you> yourname/topic
     ```

Either way, open a Pull Request into `magao-x/MagAOX:dev` from your pushed branch.

> A "remote" is just a saved *name → URL* mapping (`git remote -v` lists them); nothing is sent until
> you `git push`. `origin` is already the upstream, so the fork needs a different name — your GitHub
> username reads well and matches its URL.

---

## Reference

- MagAO-X source: <https://github.com/magao-x/MagAOX>
- Setup repo: <https://github.com/magao-x/magao-x-setup>
- Handbook: <https://magao-x.org/docs/handbook/>
- Adding an app: `MagAOX/apps/template/readme.md` (its test-wiring section is stale — the current
  harness uses `tests/tests.list` + `tests/Makefile.one`).
</content>
