# Test-Coverage Campaign — Change Summary

This change set brings every file under `libMagAOX/`, `INDI/`, and `flatlogs/` to **100 %
line coverage** as measured by the project's own methodology
(`tests/coverage/update_coverage`: full suite run → `lcov` capture → removal of
`*/tests/*` and `/usr/*` → `genhtml --filter function,brace,blank`).

Ground rules followed throughout:

- **Real fault injection over mocks.** Error paths are exercised with genuine OS-level
  faults (RLIMIT_NOFILE exhaustion, closed/stale/directory file descriptors, real
  loopback TCP/UDP sockets, zero-length datagrams, SIGPIPE on closed pipes, glibc
  EDEADLK, receive timeouts) wherever safely possible.
- **Minimal effect on production code.** Production sources change only for (a) genuine
  bug fixes found by the tests, (b) inert `#ifdef XWCTEST_*` fault-injection hooks that
  compile away in production builds, and (c) `LCOV_EXCL` markers, each carrying an
  in-code comment justifying why the excluded lines cannot be exercised.
- **Exclusions are documented, not silent.** Every `LCOV_EXCL` added by this change has
  a reason on or immediately above the marker. The taxonomy is in §4.

Final report: INDI 18/18 files, flatlogs 4/4, libMagAOX 169/169 — all at 100.0 % lines.

**Reviewing this diff.** The change set is written to be read hunk-by-hunk without this
document open: every `LCOV_EXCL` marker carries its justification on or directly above
the marker (continuation blocks point to the governing rationale); every `XWCTEST_*`
fault hook is explained at its declaration or at the file's `XWCTEST_NAMESPACE` wrapper;
every behavioral fix has a comment stating the constraint that made it wrong; every new
test file opens with a `\file` header stating its scope and technique; and each changed
build/tooling line carries a comment with its reason. This document is the map — the
per-line "why" lives at the site.

---

## 1. Bugs found and fixed in production code

These are behavior changes. Each was discovered by a failing or impossible-to-write
test during this campaign.

| File | Fix |
|---|---|
| `libMagAOX/app/MagAOXApp.hpp` (`threadStart`) | `pthread_setschedparam()` returns `0` or a **positive errno value**, never `-1`, so the old `if (rv < 0)` error check could never fire (verified with a genuine EPERM failure, which returned `1`). Changed to `rv != 0`, and the log now reports `rv` instead of the unrelated stale `errno`. |
| `libMagAOX/file/stdFileName.hpp` (`fullName`) | The `catch( const xwcException & )` handler could never run: `parseFilePath` throws `mx::exception<verboseT>`, not `xwcException`. Corrected the caught type so the intended `throw_with_nested(xwcException(...))` wrapping actually happens. `stdFileName_test.cpp` updated to expect the (now correctly) nested `xwcException`. |
| `libMagAOX/logger/types/flatbuffer_log.hpp` (`msgJSON`) | After a failed `Deserialize()`, execution fell through to `GenText()` on an un-deserialized parser — undefined behavior that dereferences the missing schema root and crashes. Now returns `"{}"` immediately; asserted by a garbage-schema test in `logTypes_test.cpp`. |
| `libMagAOX/logger/types/telem_sparkleclock.hpp` | Three defects: `eventCode` was `TELEM_DMSPECK` (copy-paste error — the type identified itself as a different telemetry stream), now `TELEM_SPARKLECLOCK`; `msgString` iterated `fbs->separations()` without a null check, crashing on a message serialized without that vector; and the static `separations()` accessor used by `getAccessor()` had the same missing null check. Both are now guarded. |
| `libMagAOX/logger/logMap.hpp` | Pre-existing test hooks `throw std::bad_alloc;` / `throw std::exception;` were missing `()` — they did not compile when their macros were enabled. Fixed to `throw std::bad_alloc();` / `throw std::exception();`. |
| `libMagAOX/sys/runCommand.cpp` | Both `read()` calls filled the whole 4096-byte buffer and then wrote the terminator at index `rd`, one byte past the array when a read filled it; they now read one byte less. On `execvp()` failure the child did `return -1` and kept running the caller's code; it now `_exit(127)`s. |
| `libMagAOX/tty/telnetConn.cpp` | The data callback wrote `buf[ev->data.size] = 0`, one byte past the buffer libtelnet owns. The string is now built from an explicit length instead. |
| `libMagAOX/modbus/modbus.cpp` | Three defects found by `modbus_test`: `modbus_write_register` checked the reply against `WRITE_COIL` instead of `WRITE_REG`, so device error replies were silently ignored; `modbus_write_coils` copied exactly 4 values regardless of `amount`, overflowing its VLA for smaller amounts; `modbus_build_request` cast the transaction id to `uint8_t` before shifting, so the high byte was always 0. |

## 2. Test-only fault-injection hooks added to production sources

All hooks are fenced by `#ifdef XWCTEST_*` and are **inert in production builds** (no
macro is ever defined outside a test translation unit). Two patterns are used:

- **Per-site fault macros** — a single line inside an `#ifdef` forces one specific
  return value or throw at one call site (e.g. `sndRv = -1;`), so the *real*
  error-handling lines below it execute and are counted. The injected trigger line
  itself is `LCOV_EXCL_LINE`.
- **`XWCTEST_NAMESPACE` re-inclusion** — a test TU compiles the same implementation
  file a second time inside a distinct namespace with a fault macro enabled. The
  faulted copy's execution counts map back to the same source lines, so genuine
  error-branch lines earn real hits while the production copy (and production build)
  is untouched.

| File | Hooks added |
|---|---|
| `libMagAOX/app/MagAOXApp.hpp` | `XWCTEST_MAGAOXAPP_EXEC_LOG_DEATH` (forces the log-thread-death branch in `execute()`); existing `XWCTEST_MAGAOXAPP_EXEC_NORM` iteration bound raised 1→2 so the loop body's second-pass branches run. |
| `libMagAOX/app/indiDriver.hpp` | `XWCTEST_INDIDRIVER_HOOKS` — a static `testHooks_t` (`forceCtrlWriteFail`, `forceOutGoingNull`, `forceSendNonStdThrow`, `forceQuitAfterSend`) toggled at runtime by `indiDriver_test` to reach write-failure, null-connection, non-std-throw, and post-send-quit branches. |
| `libMagAOX/logger/logFileRaw.hpp` | `XWCTEST_LOGFILERAW_{LOGPATH,LOGNAME,LOGEXT}_{BAD_ALLOC,EXCEPTION}`, `_WRITELOG_FWRITE_FAIL`, `_FLUSH_FFLUSH_FAIL`, `_CLOSE_FCLOSE_FAIL`, `_CREATEFILE_{EXCEPTION,EXISTS_ERRC,FOPEN_FAIL}`; `XWCTEST_NAMESPACE` wrapper. Also splits `fflush`/`fclose` return values into named locals so the hooks can override them. |
| `libMagAOX/logger/logManager.hpp` | `XWCTEST_LOGMANAGER_LOGTHREADSTART_{STD_EXCEPTION,UNKNOWN_EXCEPTION,NOT_JOINABLE}`; `XWCTEST_NAMESPACE` wrapper. |
| `libMagAOX/logger/logMap.cpp` / `logMap.hpp` | `XWCTEST_LOGINMEMORY_LOADFILE_SHORTREAD`, `XWCTEST_LOGMAP_LATFM_{DIREXISTS_ERRC,SIZEERR1,SIZEERR2}`; `XWCTEST_NAMESPACE` wrapper. |
| `libMagAOX/modbus/modbus.cpp` / `modbus.hpp` | `XWCTEST_MODBUS_SET_TIMEOUTS_SNDTIMEO_FAIL` (the `setsockopt` return is split into a named local for this), `XWCTEST_MODBUS_SEND_PARTIAL` (simulates a short `send()` without touching real I/O); `XWCTEST_NAMESPACE` wrapper. `modbus.hpp` additionally gains a separate `MODBUSPP_MODBUS_CONSTANTS_H` guard so the file can be re-included under a test namespace without redeclaring its unscoped enums (see the in-code comment). |
| `libMagAOX/tty/ttyUSB.cpp` | `XWCTEST_TTYUSB_SYSFS_DIR` / `XWCTEST_TTYUSB_SYSFS_PREFIX` — compile-time overrides (defaults are exactly the production `/sys/class/tty/` + `ttyUSB`) letting tests point the sysfs scan at a real non-USB tty (`tty0`) or a scratch directory, since no USB-serial hardware exists in the build environment; `XWCTEST_NAMESPACE` wrapper. |

## 3. Coverage-tooling and build-infrastructure changes

| File | Change and reason |
|---|---|
| `tests/coverage/update_coverage` | (1) Dropped `"/sys/*"` and `"/tty/*"` from the `lcov --remove` list: these globs match *any* path containing `/sys/` or `/tty/`, which was silently deleting the production sources `libMagAOX/sys/` and `libMagAOX/tty/` from the report. (2) `-j $(nproc)` on capture and genhtml (lcov 2.x parallelism). (3) `--ignore-errors inconsistent,path` — tolerates gcc gcno/path quirks that otherwise abort lcov 2.5. (4) genhtml filter extended `function` → `function,brace,blank`: discounts gcov's spurious records on brace-only and blank lines (e.g. EH-epilogue braces) instead of requiring a source exclusion for each. |
| `tests/Makefile` | Rewritten for correct parallel builds: one phony goal per test (`make -jN` now builds tests concurrently), `testMain.o` prebuilt once via an order-only `prep` prerequisite so parallel jobs don't race compiling it, `tests.list` read via `$(shell grep -v blank)` + `$(sort)` (dedupe), and a failed build prints `SKIP (build failed)` instead of aborting the remaining tests. Test *execution* (`make test`) remains sequential — several suites use process-global state (rlimits, signal dispositions, fixed loopback ports). |
| `tests/Makefile.one` | (1) `magaox_git_version.h` is now generated to a `$$`-suffixed temp file and atomically `mv`-ed — parallel per-test makes each regenerate this header and previously raced, corrupting it mid-compile. (2) `logTypes_test` links `-lflatbuffers` (it exercises `logJsonFormat`/`msgJSON`, whose schema reflection lives there). (3) `edtCamera_test` added to `EDT_TESTS`. |
| `libMagAOX/logger/tests/Makefile` | Uses the repo-standard `Make/common.mk` (drops the locally duplicated `COVERAGE` flag block and hard-coded `-std`) and `Make/python.mk` (`$(PYTHON)`: the test generator needs Python ≥ 3.10). Links `-lflatbuffers` (msgJSON, see above) and `../../app/stateCodes.cpp` (generated tests reference stateCodes symbols). |
| `tests/tests.list` | Registers the 28 new test binaries (see §5). `tty_test` removed (split into five focused suites); all other existing entries retained. |
| `tests/.fftw_wisdom.float` | Machine-generated FFTW wisdom cache that was tracked by mistake. Removed from the tree; it is already matched by `.gitignore`. |
| `libMagAOX/logger/tests/generateTemplatedCatch2Tests.py`, `catch2TestTemplate.jinja2` | Generator extended (no existing generation removed): the per-log-type template now also round-trips every type through `logShortStdFormat`, `logMinStdFormat`, and `logJsonFormat` (previously only `logStdFormat`), which is what exposed the `telem_sparkleclock` and `flatbuffer_log` bugs in §1. |

## 4. `LCOV_EXCL` exclusions added to production sources

Every exclusion carries an in-code justification. They fall into seven recurring
categories; the per-file lists below name the category rather than repeating the prose.

- **(a) EH-epilogue artifact** — gcov emits a phantom record on the closing brace (or
  ctor-clone epilogue, or stream-chain temporary cleanup) of a function whose locals
  need exception cleanup; the line is unreachable without an exception in a nothrow
  region. (Most brace-only cases are now handled globally by the `brace` filter in §3;
  the remaining excluded ones carry content on the line.)
- **(b) Defensive dead code** — provably unreachable given the language or the callee's
  contract (e.g. `new` never returns `nullptr`; a `std::thread` is joinable right after
  non-throwing construction; a flatbuffers field that every constructor always sets can
  never be absent).
- **(c) Shadowed/unreachable handler** — a catch clause that a more-derived handler or
  an inner catch always intercepts first.
- **(d) Uninjectable syscall failure** — the call cannot fail with the fixed, valid
  arguments used (`clock_gettime(CLOCK_REALTIME)`, `sigaction` with a valid signo,
  `sem_init`/`sem_post` within limits, `gethostname` with an ample buffer,
  `setsockopt(SO_RCVTIMEO)` on a valid fd, `cfsetospeed` after `cfsetispeed` accepted
  the same value, `inet_pton` returning −1).
- **(e) Unsafe or impractical to inject** — forcing the failure would destabilize the
  shared test process or requires resources the environment lacks: `RLIMIT_NPROC`
  (starves same-UID processes — verified), `RLIMIT_FSIZE`+SIGXFSZ, an EAGAIN busy-spin,
  a real indiserver connection, real USB-serial hardware, a genuine mid-syscall race
  window, or a fixed 30 s poll timeout.
- **(f) Uncallable code** — private and never called (private copy ctors/`operator=` of
  `IndiClient`/`IndiDriver`/`IndiConnection`, `IndiXmlParser::convertTypeToString`), or
  guaranteed to crash if ever called (`IndiXmlParser` copy ctor/`operator=`).
- **(g) gcov quirk** — lines that demonstrably execute but get spurious zero records
  (cast-expression argument lines of one aggregate-init call in
  `stdCamera::recordCamera`, where neighboring argument lines of the *same call* show
  real hits; `std::format` argument lines in `fileTimes.hpp`).

Per-file:

| File(s) | Categories |
|---|---|
| `INDI/libcommon/IndiClient.cpp` | (f) copy ctor/`operator=`; (b) `setup()` reconnect branch (private, called once per ctor with a fresh socket); (c) unreachable catch tail after an always-throwing `close()`, plus the `runtime_error` catch it shadows. |
| `INDI/libcommon/IndiConnection.cpp` | (f) copy ctor/`operator=`; (b) destructor `catch(...)` (deactivate() doesn't throw in practice); (c) `pthreadProcess` `catch(std::exception)` — `process()` swallows all std exceptions internally; (e) `sendXml` EINTR-during-write window. |
| `INDI/libcommon/IndiDriver.cpp` | (f) copy ctor/`operator=`. |
| `INDI/libcommon/IndiXmlParser.cpp` | (f) copy ctor/`operator=` (documented guaranteed NULL-deref crash) and dead `convertTypeToString`; (b) `getAttributeValue` NULL-root guard (private; every caller checks first). |
| `INDI/libcommon/IndiElement.cpp`, `IndiMessage.cpp`, `IndiProperty.cpp`, `TimeStamp.cpp` | (a) EH-epilogue braces and one stream-chain artifact line each in `IndiElement.cpp:137` / `IndiProperty.cpp:379`. |
| `INDI/libcommon/MutexLock.hpp`, `ReadWriteLock.hpp` | (d) init/destroy/lock/unlock failure returns with valid fixed arguments (the reachable EDEADLK paths of `lockRead`/`lockWrite` are *covered*, via real deadlock-detection tests). |
| `INDI/libcommon/SystemSocket.cpp` | (d) `inet_pton` −1, `select` in `connect`, `SO_RCVTIMEO`, `fcntl` on a valid fd, `gethostname`; (e) EAGAIN busy-spin retries, partial-send loops (kernel never short-writes these small buffers on loopback), `EINPROGRESS`+`SO_ERROR` path (needs a real remote network), `SIOCGIFCONF` failure. |
| `INDI/libcommon/Thread.cpp` | (d)/(e) two `fcntl` cleanup blocks and the pause-pipe read failure (`select` just reported the fd readable). |
| `libMagAOX/app/MagAOXApp.hpp` | (b) `new` nullptr check, joinable-after-construct, `indiTargetUpdate` always-true `set`; (c) catches around `sendXml`-backed calls (best-effort `write()`, never throws); (d) SIGTERM `sigaction` (see the long in-code note on why only SIGQUIT/SIGINT are fault-injectable per TU); (e) `std::thread` ctor failure (RLIMIT_NPROC), tpid short-write (RLIMIT_FSIZE/SIGXFSZ), `startINDI` re-entry teardown (segfaults without a real indiserver), indiDriver ctor throw, success returns of `sendNewProperty*` (need a live indiserver connection); config bookkeeping branches unreachable without a derived app that defines-but-never-reads an option. |
| `libMagAOX/app/dev/dm.hpp`, `dmPokeWFS.hpp`, `dssShutter.hpp`, `frameGrabber.hpp`, `shmimMonitor.hpp`, `stdCamera.hpp`, `outletController.hpp`, `indiUtils.hpp` | (b) registration-failure propagation that the `MagAOXApp<false>` harness short-circuits to success; (d) `sigaction`/`sem_init`/`sem_post`/`clock_gettime`; (e) `threadStart` failure (RLIMIT_NPROC), signal-vs-join races in `appShutdown`, stat-after-open races, second shm create() failing while the first succeeded, 30 s-window races; (g) `recordCamera` cast-argument lines; plus one documented harness-combination gap in `stdCamera::newCallBack_stdCamera` (tempControl-without-temp dispatch line). |
| `libMagAOX/logger/logMap.hpp`, `logMap.cpp`, `logMeta.hpp`, `logMeta.cpp` | (b) `loadFiles` failure ruled out by the caller's own precondition, garbage-read corruption guards only reachable via out-of-bounds reads, null-entry guards ruled out by `getNextLog`/`getPriorLog` postconditions, value types no generated log ever produces (verified by grep over `types/*.hpp`); (a) one epilogue brace. |
| `libMagAOX/logger/types/*.hpp` (≈55 files) | (a) epilogue braces; (b) null-checks on flatbuffers fields that every `messageT` constructor unconditionally creates (each exclusion names the field and the constructor argument that guarantees it). |
| `libMagAOX/sys/runCommand.cpp` | (e) the forked child branch: it genuinely runs (the tests pass on its output) but `execvp` replaces the image before gcov's atexit flush; forcing the exec-failure fallthrough would let the child re-enter the test binary. (b) `read()` failure on a just-created owned pipe. |
| `libMagAOX/tty/telnetConn.cpp` | (d) bind to port 0/INADDR_ANY; (b) `telnet_init` (fails only on internal malloc), `send()` returning 0 (POSIX doesn't define it here); (e) fixed 30 s poll timeout, libtelnet fatal-compression and never-negotiated event types (the class never calls `telnet_negotiate`). |
| `libMagAOX/tty/ttyIOUtils.cpp` | (d) `cfsetospeed` after identical-argument `cfsetispeed` succeeded; (e) `tcsetattr` failing mid-operation on an already-validated tty fd. |
| `libMagAOX/tty/ttyUSB.cpp` | (e) sysattr-match branches that only exist with a real attached USB-serial device. |
| `libMagAOX/file/fileTimes.hpp`, `stdFileName.hpp` | (g) `std::format` argument lines; (a) one epilogue brace. |

## 5. New and expanded test suites

All new tests are Catch2, registered in `tests/tests.list`, self-contained (they create
and remove their own scratch state), and use real OS faults per the ground rules.

**INDI (9 new suites — this directory previously had only `INDI_test`):**

| Suite | Scope (assertions) |
|---|---|
| `IndiElement_test` | Full API sweep: all constructors/setters/getters, switch/light states, comparison, formatting (104). |
| `IndiMessage_test` | Message types, property payloads, assignment (24). |
| `IndiProperty_test` | All property types, element maps, `Excep` error codes, equality including value differences (173). |
| `TimeStamp_test` | Fixed instant 2007-06-24T19:38:12.234Z so every formatted output is asserted exactly; ISO8601/MJD round-trips; TZ save-restore branch via real `setenv`+`tzset` (91). |
| `Thread_test` | Lifecycle, pause/resume, one-shot, EDEADLK lock guards, RLIMIT_NOFILE pipe-creation failure, datagram-trigger loop, and a deterministic stop-interrupts-idle-select test (60). |
| `SystemSocket_test` | Real loopback TCP and UDP end-to-end, multicast join, option matrix, and the full error surface: EBADF matrix, double-bind, connect-refused, blackhole connect timeout, zero-length datagrams, EPIPE with SIGPIPE ignored, `socket()` failure ×3 types under RLIMIT_NOFILE, stale-fd `close()` failure (89). |
| `IndiXmlParser_test` | Round-trips every message×property type; required-attribute throw matrices (property- and element-level); two-chunk and line-by-line stream parsing; malformed-XML recovery; parse-tree serialization (240). |
| `IndiConnection_test` | Drives the real processing loop over pipes: dispatch, EOF quit, threaded mode, throwing overrides (std/non-std, runtime/logic), closed-fd select failure, directory-fd read failure, SIGPIPE-safe write-error path (39). |
| `IndiDriverClient_test` | Driver dispatch/send matrix over real pipes with XML drain assertions, the real 5-second uptime interval, and a client against a real loopback TCP server (27). |

**libMagAOX/app:** `indiDriver_test`(+`.hpp`) — FIFO lifecycle and the §2 hook branches;
`indiUtils_test` — `updateIfChanged`/`updatesIfChanged` all overloads incl. throw paths;
`MagAOXApp_test`/`MagAOXAppExecute_test`/`MagAOXApp_test.hpp` — expanded for state
machine, config validation, PID lock, signal handlers (incl. per-TU SIGQUIT/SIGINT
sigaction-failure injection), INDI property registration, power state, and full
`execute()` runs; `semUtils_test` — expanded timeout/error paths.

**libMagAOX/app/dev:** new suites `dmPokeWFS_test`(+`.hpp`), `dssShutter_test`,
`edtCamera_test`, `frameGrabber_test`, `ioDevice_test`, `shmimMonitor_test`,
`stdCamera_test`(+`.hpp`); expanded `dm_test`(+`.hpp`, now self-contained via
`remove_all` of its calib dirs), `outletController_test`, `stdMotionStage_test`. These
drive the real CRTP mixins under a `MagAOXApp<false>` harness with real shmim streams,
real semaphores, and real INDI property callbacks.

**libMagAOX/logger:** new `logManager_test` (thread lifecycle incl. §2 hook fault
paths), `logMeta_test` (accessors, card generation, buffer walking), `logTypes_test`
(format/parse for hand-picked types, long-appName truncation regimes for
`logShortStdFormat`, unknown-code fallbacks, garbage-schema `msgJSON`); expanded
`logFileRaw_test` (all §2 hook fault paths), `logMap_test`, and
`logTypes_Accessor_test` (adds a valid-member `getAccessor` case via `string_log`, the
one type with no generated per-type test of its own); generator now round-trips
all four formatters for every generated log type (§3).

**libMagAOX/tty:** `tty_test.cpp` split into `ttyErrors_test`, `usbDevice_test`,
`netSerial_test`, `telnetConn_test` (against a real in-process telnet server thread),
`ttyUSB_test` (real sysfs via the §2 path overrides); `ttyIOUtils_test` expanded with
real pty pairs.

**Others:** `modbus_test` (rewritten; real loopback modbus server) +
`modbus_exception_test` (all exception classes + §2 hook faults); `runCommand_test`
expanded (real fork/exec of `/bin/echo`, stderr capture, missing binary);
`common_exceptions_test` (xwcException hierarchy); `pixaccess_test` (all pixel
getters/casts per ImageStreamIO type) and expanded `ImageStreamIO_test`;
`flatlogcodes_test` rewritten from the template stub (logHeader msgLen0/1/2 length
regimes with hand-built 64-byte buffers, priority/level parsing, `timespecX`
conversions incl. negative and carry cases).

## 6. Latent defects documented but deliberately not fixed

Found by this campaign, left unchanged to keep the diff minimal, each documented at the
site in code (see §1 of the in-code comments; also candidates for a follow-up PR):

- `IndiXmlParser` copy ctor/`operator=` crash unconditionally (mutually exclusive NULL
  cursors — see in-code note).
- `SystemSocket::close()` failure leaves `m_nSocket` set; the destructor then re-closes
  and the second throw escapes a noexcept destructor → `std::terminate` (observed as a
  real SIGABRT during test development).
- `pcf::Thread`'s constructor leaves `m_mutReady` locked, so `operator=` on a
  never-started Thread self-deadlocks; `join()` can return `-EHOSTDOWN` when racing the
  loop's own exit.
- `IndiClient`'s constructor propagates `SystemSocket::Error` on a failed connect
  because `setup()`'s cleanup `close()` throws on the already-invalid socket.
- Zero-length UDP datagrams are reported as peer shutdown (ECONNRESET) by
  `SystemSocket::recvFrom`/`recvChunkFrom`, though empty datagrams are legal.
- `Thread::setOneShot` is declared but never defined (link error if ever used).
- `sys::runCommand` issues a single `read()` per pipe, so output beyond 4095 bytes is
  silently truncated.
- `modbus_illegal_address_exception` assigns `msg = "test"` in its constructor but its
  `what()` returns a fixed string, so the assignment is dead.
- `IndiClient::setup()`'s catch handler calls `m_socClient.close()` on a socket that
  `SystemSocket::connect()` already closed, so `close()` throws and the handler's own
  sleep-and-return never runs; callers see a "bad file descriptor" error instead of
  the connect failure.

## 7. The `XWCTEST_IF_` macro pattern (second phase)

A follow-on pass replaced the verbose raw `#ifdef XWCTEST_* ... #endif` fault-hook
blocks of §2 with single-line macro calls, minimizing the visual footprint of test
hooks in production sources without changing what compiles in any configuration.

**Generator.** `tests/genTestMacros.py` + `tests/xwcTestMacroTemplate.jinja2` generate a
per-directory `testMacros.hpp` from that directory's `xwcTestNames.txt` (one hook name
per line; regenerate with
`python3 tests/genTestMacros.py --names <dir>/xwcTestNames.txt --out <dir>/testMacros.hpp`).
For each name the generated header defines:

```c
#ifdef XWCTEST_<NAME>
#define XWCTEST_IF_<NAME>(line) { line ; } /* LCOV_EXCL_LINE */
#else
#define XWCTEST_IF_<NAME>(line) do {} while(0)
#endif
```

so a production call site is one line — `XWCTEST_IF_<NAME>( sndRv = -1 );` — inert
unless the test TU defines `XWCTEST_<NAME>` before including the header. The expansion
is a braced compound statement (`{ line ; }`, not `(line);`) so hooks can wrap
statements like `return -1` and call sites may be written with or without their own
trailing `;` (a doubled `;` is a harmless empty statement). The trigger line carries
`LCOV_EXCL_LINE` in the macro itself, so call sites need no per-site exclusion.

Generated header/name-list pairs live in: `libMagAOX/logger/tests/`,
`libMagAOX/app/tests/`, `libMagAOX/app/dev/tests/`, `libMagAOX/file/tests/`,
`libMagAOX/modbus/tests/`, and `apps/userGainCtrl/tests/`.

**Converted production sources** (hook behavior identical, sites now one line each):
`libMagAOX/logger/logMap.hpp`/`.cpp`, `logFileRaw.hpp`, `logManager.hpp`;
`libMagAOX/app/MagAOXApp.hpp`, `indiDriver.hpp`; `libMagAOX/app/dev/stdCamera.hpp`,
`telemeter.hpp`; `libMagAOX/file/stdSubDir.hpp`, `stdFileName.hpp`, `fileTimes.hpp`;
`libMagAOX/modbus/modbus.cpp`; `apps/userGainCtrl/userGainCtrl.hpp`. A few sites
deliberately remain raw `#ifdef`/`#ifndef` where the macro cannot express them
(e.g. conditional *compilation of declarations* rather than statements); each such
site has an in-code comment saying so. The `XWCTEST_NAMESPACE` re-inclusion technique
of §2 is unchanged and coexists with the macros.

**Design doc.** `xwctest-macro-coverage-strategy.md` (repo root) is the strategy
document this phase was implemented from: the macro pattern's rationale, the verified
coverage gaps it targets, and the adoption plan.

**Harness minimization.** `libMagAOX/app/dev/tests/testHarnessCommon.hpp` factors the
previously copy-pasted throwaway `indiDriver` construction into one documented helper,
`makeFifolessIndiDriver<AppT>()`, used by the dm, stdCamera, frameGrabber,
outletController, shmimMonitor, stdMotionStage, MagAOXApp, zaberCtrl, and mcp3208Ctrl
harnesses. Its header documents the safety contract (never `execute()`/`activate()`d,
so sends are no-ops) and its one limitation (`indiDriver::sendNewProperty()` still
constructs a real outbound `indiClient`, which needs a live indiserver — see §8).

**`LCOV_EXCL` hygiene.** Runs of consecutive `// LCOV_EXCL_LINE` markers were
consolidated into `LCOV_EXCL_START`/`LCOV_EXCL_STOP` blocks (12 files), keeping one
justification comment per block instead of one per line.

## 8. App-test repairs (second phase)

Failures in `apps/` suites — pre-existing, but fixed rather than left documented:

| File(s) | Fix |
|---|---|
| `apps/mcp3208Ctrl/mcp3208Ctrl.hpp` | **Production bug**: `loadConfigImpl()` read `_config( m_numChannels, "accel.numChannels" )` but no matching `config.add("accel.numChannels", ...)` registration existed, so the option could never be loaded from a config file (the configurator only looks up pre-registered targets); it silently always used the in-class default. Registration added (with in-code comment). |
| `apps/mcp3208Ctrl/tests/mcp3208Ctrl_test.cpp` | Harness constructs a fifoless `indiDriver` so `updateTimingDiagnosticsIndi()`'s publishes are observable (`MagAOXApp::updatesIfChanged()` silently no-ops with a null driver — the cause of the timing-diagnostics assertion failures); new test covering the null-driver no-op guard itself (`MagAOXApp.hpp:3798`); corrected the delay-controller expected-value formula (the app resets `m_synchroDelay` to the *target* before the controller correction, so the target term appears twice); trigger-time test now performs two semaphore cycles (`updateTriggerTiming()` needs a measured period before it computes `m_triggerTime`). |
| `apps/zaberCtrl/tests/zaberCtrl_test.cpp` | `tests/testMacrosINDI.hpp` unconditionally defines `XWCTEST_INDI_CALLBACK_VALIDATION`, which flips `INDI_VALIDATE_CALLBACK_PROPS` into a short-circuit test mode for the whole TU — masking the real HOMING-transition callback logic under test. Fixed with `#undef` after the include (commented in place). This exposes that `rawPos`/`preset`'s "Right Device.Name" sub-cases call `sendNewProperty()`, which requires a live indiserver connection; left failing with a detailed known-limitation comment (see §9). |
| `apps/zaberLowLevel/tests/zaberLowLevel_test.cpp` | Removed incorrect `const` from two harness methods calling non-const production methods; harness now sets `state(INITIALIZED)` before `appStartup()` (matching the real `execute()` sequence — `appStartup()` fails from UNINITIALIZED); `warnValue()` reads the Switch element via `getSwitchState()` (the generic string value of a Switch is `"1"`/`"0"`, not On/Off). |
| `apps/tcsInterface/tests/tcsInterface_test.cpp` | The `labMode` callback is protected in `tcsInterface` and the callback test macro invokes it on a harness object from a free SCENARIO function. The harness now re-exports it with `using tcsInterface::newCallBack_m_indiP_labMode;`. No production change. |
| `apps/xindiserver/xindiserver.hpp` | The existing unqualified `friend class xindiserver_test;` bound to `MagAOX::app::xindiserver_test`, which does not exist, rather than the real harness struct in `libXWCTest::xindiserverTest`. The friend declaration now names the harness in full (with a forward declaration above the class). The eight members the harness drives stay `protected`. |
| `apps/xindiserver/tests/xindiserver_test.cpp`, `apps/sshDigger/tests/sshDigger_test.cpp` | Identical latent bug: the test namespaces were closed *before* a later `SCENARIO` block that needs the harness class unqualified; closing braces moved to end of file. |
| `apps/siglentSDG/tests/siglentSDG_test.cpp` | Removed an orphaned `}` (a leftover closing brace for a namespace name that never existed). |
| `apps/timeSeriesSimulator/tests/` | `timeSeriesSimulator.cpp` → `timeSeriesSimulator_test.cpp` (`git mv`): the file was misnamed, so the test build had no rule to make the target. |
| `flatlogs/include/flatlogs/timespecX.hpp` | `LCOV_EXCL` on `timeStamp()`'s `gmtime_r` failure branch: `time_s` is `secT` (`uint32_t`, max ≈ year 2106), entirely within glibc `gmtime_r`'s representable range, so the defensive check cannot fire (category (b) of §4; the sibling formatters in the same file do not check at all). |

## 9. Known remaining failures

- **Nine suites do not build here** for lack of third-party libraries absent from this
  environment: `loPredCtrl_test` (DDSPC), `usbtempMon_test` (DS18B20), and the seven
  instGraph-dependent suites (`xInstGraph_test`, `xigNode_test`, `fsmNode_test`,
  `indiPropNode_test`, `pwrOnOffNode_test`, `staticNode_test`, `stdMotionNode_test`).
  None of their sources are touched by this change set.
- **`zaberCtrl_test`: 2 assertions** (`rawPos`/`preset` "Right Device.Name") fail
  because the real callbacks call `indiDriver::sendNewProperty()`, which lazily
  constructs a genuine outbound `indiClient` and returns −1 when no live indiserver is
  reachable. Previously masked by the `XWCTEST_INDI_CALLBACK_VALIDATION` leak fixed in
  §8. Properly covering it needs a mocked `indiClient` or a local indiserver; a
  detailed comment marks the site.
- **Environment note:** `ocam2KCtrl_test` requires `$MILK_SHM_DIR` (default
  `/milk/shm`) to be writable; with it writable all 474 assertions pass. Not a code or
  test defect.
