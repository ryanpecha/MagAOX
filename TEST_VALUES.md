# Hard coded test values to review

Inventory of numeric literals introduced by the new libMagAOX tests, with the human
friendly replacement chosen for each. Ticked rows have been applied in the code, and each
value now carries a short comment in the test that says why it has that value.

Rule: only values introduced by the new tests are changed. A value that already exists on
`dev` stays as it is, even inside a file that was otherwise modified, and new lines in such
a file follow the existing convention of that file.

## 1. Unix timestamp `1732170780` (2024-11-21 06:33:00 UTC)

Used as the `base` time in `libMagAOX/logger/tests/{logMap,logMeta,logTypes}_test.cpp`
(new lines). logFileRaw_test.cpp also uses it, but there it predates this work, so that
file keeps `1732170780` and its derived `_20241121063300...` names, in old and new lines.
It is not random. It is a coordinate on the fixture timeline that the logMap tests build
under `/tmp/logMap_test/dev1/2024_11_19 .. 2024_11_23`. It has to stay on 2024-11-21 and
between the 05:23 fixture file and the 21:00 fixture file, and `base + 21600` must stay on
the same day.

- [x] Replace `1732170780` with `1732176000` (2024-11-21 08:00:00 UTC). Same day, round hour,
      all `base + N` offsets in the tests (max 21600 s) stay before 21:00.
- [x] The expected file names derived from it change from `_20241121063300...` to
      `_20241121080000...` in logFileRaw_test.cpp lines 233, 290, 374, 423, 463.
- [x] The comment `// This is 2024_11_21 06:33:00.` in logMap_test.cpp:916 changes to 08:00:00.
- [x] `logMapDebugTime` check in logMap_test.cpp:1107-1108 (`"1732170780.5"`) changes to `"1732176000.5"`.

## 2. Fixture file name timestamps in `logMap_test.cpp`

These encode ordering and day gap relationships (previous and following subdirectory
search, empty days, end of day). They all exist on `dev`, so they stay exactly as they are.

| value | meaning | proposed |
|---|---|---|
| `20241119025526000004000` | 02:55:26.000004 | `20241119025500000000000` if no test depends on the 4 us |
| `20241123023002000002000` | 02:30:02.000002 | `20241123023000000000000` |
| `20241123044510000000012` | 04:45:10.000000012 | `20241123044510000000000` |
| `20241119000061000000000` | 00:00:**61** (invalid seconds field) | verify intent; if not deliberate use `20241119000100000000000` |
| `20241119000030`, `000120`, `052200`, `052300`, `20241121200030`, `210000`, `220000`, `235959999999999`, `20241118030000`, `20241124030000`, `20241123044500`, `044611`, `20241215000000`, `20250601000000` | round times | keep |

- [ ] The three sub second values stay. They predate this work.
- [ ] `000061` stays. It is deliberate, 61 s after midnight so the 60 s buffer picks the
      previous file, and it predates this work.
- [ ] The one year later time `1763706780` in logFileRaw_test stays, same reason.
- [ ] `libMagAOX/file/tests/fileTimes_test.cpp` also uses `1732170780`. It is unchanged from
      `dev`, so it was left alone.

## 3. Event codes

| value | where | proposed |
|---|---|---|
| `60001`..`60014` | dummy log types and logMetaSpec in logMeta_test.cpp | keep, sequential and above every real code |
| `60999` | `unknownCode` in logTypes_test.cpp:63 | keep |
| `59999` | `verifyLogEntry` rejection in logMeta_test.cpp:1546 | keep |
| `9999` | unknown code in logMeta_test.cpp:405 | keep |

## 4. Everything else

| value | where | note | proposed |
|---|---|---|---|
| `12345` | logManager_test writePause, telnetConn_test port, netSerial_test port, logMeta_test expected value | already human | keep |
| `1234567` | usbDevice_test.cpp:106 baud | deliberately not a valid baud rate | keep |
| `2500`, `3700` | ioDevice_test.cpp:71,85,86 timeouts | arbitrary | `2500`, `3500` (optional) |
| `0.0000001`, `0.0001` | frameGrabber_test.cpp:970,655 | style | `1e-7`, `1e-4` |
| `1025`, `1023` | stdCamera_test | 1024 plus or minus one, deliberate | keep |
| `0xDEADBEEF` | ttyIOUtils_test.cpp:96 invalid `speed_t` | sentinel | keep |
| `1200`, `1300`, `1500` ms | sleeps in dm, dmPokeWFS, frameGrabber, shmimMonitor, indiDriver tests | timing margins over 1 s waits | keep |
| `5000000000`, `6000000000` | pixaccess_test | values above 2^32 for 64 bit paths | keep |
| `1000000000`, `2000000000` | semUtils_test tv_nsec | one second in ns and an overflow | keep |
| `"0403"`, `"6001"` | ttyUSB_test | real FTDI vendor and product ids | keep |
| `0555`, `0755`, `0660`, `0600` | chmod and mkfifo modes | octal modes | keep |

- [x] `2500/3700` to `2500/3500`
- [x] `0.0000001` to `1e-7` and `0.0001` to `1e-4`

## 5. Hard coded strings

Most string literals in the new tests are INDI property and element names, config keys,
or real MagAO-X device names (`fwfpm`, `lyotsm`, `65-35-open`, `stagesci1`). Those stay.
The arbitrary ones were handled as follows. Ticked rows are applied.

| old | new | where | reason |
|---|---|---|---|
| `"pdu9"`, `"thisch"`, `"thisel"`, `"thistgtel"` | `"pdu0"`, `"outlet1"`, `"state"`, `"target"` | MagAOXApp_test, MagAOXAppExecute_test | follow the real pdu device and element naming |
| `"a@b.c"`, `"obs"`, `"tgt"`, `"op@b.c"` | `"observer@example.com"`, `"observer"`, `"target"`, `"operator@example.com"` | logTypes_test telem_observer | valid placeholder field values |
| `"nope"` | `"no-such-app"` | logMeta_test | matches the name already used for the same purpose in the file |
| `"whatever"`, `"bad"` | `"member"` | logMeta_test logMetaSpec | the member name is not used by those sections |
| `"r"`, `"s"`, `"t"`, `"m"` | `"repo"`, `"sha"`, `"text"`, `"message"` | logMeta_test minimal payloads | say what the field is |
| `"zzz"` | `"bogus-level"` | logManager_test | the parser keys on the first letter, so it must start with a letter no level uses |
| `"foo"` | `"cmd"` | stdCamera_test serialCommand | any value, the section is skipped |

Kept as is, because they are an existing convention shared with tests that predate this
work (telemeter_test.cpp on dev uses both), now with a comment on the first use in each
new file:

| value | meaning |
|---|---|
| `"xx"` as the harness constructor argument | the placeholder git sha every test harness uses |
| `{"none"},{"nada"},{"0"}` | the placeholder entry every test uses so a config file is not empty |

Also kept, with the reason already in the test or added as a comment: `"a"`/`"b"` element
names, `"d"` and `"F"` (deliberately short, they exercise card() padding), `"f1"`..`"f6"`,
`"stest"` (stage test), `"some*"`, `"no-such-*"` and `"not_*"` names (deliberately
unregistered), `"KEYW"`/`"memb"`/`"cmt"` abbreviations, `"hello world"` and whitespace strings
(trimming tests), `"bogus1"`, `"garbage"`, `"corrupt"`, `"Weird"`.

- [x] all rows above applied

