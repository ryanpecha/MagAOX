shmimDelta
==========

[TOC]

------------------------------------------------------------------------

# NAME

shmimDelta - measure semaphore timing deltas between two ImageStreamIO streams.

# SYNOPSIS

```
shmimDelta [options]
shmimDelta --shmimName1 camwfs --shmimName2 aol1_imWFS2 -N 1000 -t 1
```

# DESCRIPTION

`shmimDelta` attaches to two ImageStreamIO shared memory image streams
(`shmims`) and measures the timing delta between their semaphore updates.  It is
intended for quick latency and jitter checks between a source stream and a
downstream stream derived from it.

The reported delta is:

```
shmimName2 time - shmimName1 time
```

At startup, `shmimDelta` opens both streams, selects a wait semaphore for each
stream, and flushes both semaphores before collecting samples.  During
collection, each stream is watched by its own waiter thread so that waiting on
one stream does not serialize waiting on the other.

After each semaphore wake, `shmimDelta` drains any queued semaphore posts and
records only the latest observed frame.  This avoids slowly accumulating stale
semaphore posts when the reader falls behind.  Duplicate frame counters are
ignored.

When possible, samples are paired by matching ImageStreamIO frame counter
(`cnt0`).  If no usable common `cnt0` values are found, samples are paired by
arrival order and the output reports `pairing: order`.

# OPTIONS

| Short | Long | Config-file key | Type | Description |
| --- | --- | --- | --- | --- |
| `-1` | `--shmimName1` | `shmimName1` | string | First stream name.  This is the reference stream in the delta calculation. |
| `-2` | `--shmimName2` | `shmimName2` | string | Second stream name.  Reported delta is this stream's time minus stream 1's time. |
| `-N` | `--nFrames` | `nFrames` | int | Number of paired frame arrivals to measure.  Default is `100`. |
| `-t` | `--timeout` | `timeout` | float | Per-frame semaphore wait timeout in seconds.  Default is `1.0`. |

# OUTPUT

`shmimDelta` first prints the attached stream names and dimensions:

```
shmim1: camwfs
size1: 50 50 1
shmim2: aol2_imWFS2
size2: 50 50 1
```

It then prints the timing statistics:

```
timed_pairs: 1000
pairing: cnt0
delta_mean_usec: 123.456
delta_rms_usec: 7.890
```

The timing values are in microseconds.  `delta_rms_usec` is the rms scatter of
the paired deltas about `delta_mean_usec`.

The `pairing` field reports how frames were paired:

- `cnt0`: samples were paired by matching ImageStreamIO frame counter values;
- `order`: no usable common counters were found, so samples were paired by
  collection order.

# TIMING SOURCE

For each semaphore wake, `shmimDelta` samples the latest stream metadata.  If
the stream has a valid ImageStreamIO `writetime`, that metadata timestamp is
used for the delta calculation.  If no valid metadata timestamp is available,
the local `CLOCK_MONOTONIC` timestamp taken immediately after the semaphore wake
is used as a fallback.

The local fallback includes scheduler wake latency.  For lowest jitter, run
`shmimDelta` on an isolated CPU or cpuset.

# EXAMPLES

Measure 1000 paired updates:

```
shmimDelta \
  --shmimName1 camwfs \
  --shmimName2 aol2_imWFS2 \
  -N 1000 \
  -t 1
```

Run on the `mlat` cpuset as user `xsup`, preserving the caller's environment:

```
sudo -E bash -lc '
  echo $$ > /opt/MagAOX/cpuset/mlat/tasks
  exec sudo -E -u xsup -- shmimDelta \
    --shmimName1 camwfs \
    --shmimName2 aol2_imWFS2 \
    -N 1000 \
    -t 1
'
```

This pattern enters the cpuset before dropping privileges to `xsup`.  If the
cpuset contains CPU 48, for example:

```
cat /opt/MagAOX/cpuset/mlat/cpuset.cpus
48
```

then no additional `taskset -c 48` is needed.  Running `taskset` before entering
the cpuset can fail with:

```
taskset: failed to set pid <pid>'s affinity: Invalid argument
```

because the launcher process may not yet be allowed to run on the cpuset CPU.

# TROUBLESHOOTING

## Negative Deltas

A negative delta means the sampled time for `shmimName2` was earlier than the
sampled time for `shmimName1`.  This can be real if the selected streams do not
represent a causal source-to-output relationship, or if they are paired by
arrival order and the physical delay crosses a frame boundary.

Prefer runs with:

```
pairing: cnt0
```

If the output reports `pairing: order`, the two streams did not expose matching
frame counters during the run.  In that case, long runs are more sensitive to
frame drops, startup phase, and scheduler effects.

## Timeouts

Increase `-t` if either stream updates slower than the timeout:

```
shmimDelta --shmimName1 camwfs --shmimName2 aol2_imWFS2 -N 1000 -t 5
```

Timeouts can also indicate that the stream was replaced or deleted while the
tool was waiting.

## Environment

When running through `sudo`, preserve the runtime environment if stream lookup
depends on variables such as `MILK_SHM_DIR`:

```
sudo -E ...
```

# EXIT STATUS

`shmimDelta` returns:

| Code | Meaning |
| --- | --- |
| `0` | Measurement completed and statistics were printed. |
| non-zero | Stream open, semaphore wait, timeout, or configuration error. |

# SEE ALSO

`shmimInfo`, `shmimLat`, `ImageStreamIO`
