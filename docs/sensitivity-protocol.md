# Sensitivity protocol investigation

All byte offsets below are zero-based. Conclusions refer to the supplied captures,
not to measured sensor output. The captures alone do not identify active-stage
reads. A resolver is now implemented using matching public Compx implementations
and read-only replies from the actual K1NG; see [the resolver evidence](dpi-target-resolver.md).
The starting source revision was `7715fb2`, with pre-existing GUI removal changes.

Follow-up hardware testing: writes for 50 and 400 DPI at `0c` returned an exact
echo but did not change active sensitivity. USB completion, protocol response,
and active-setting change are three distinct outcomes. The fixed default has
been removed. The driver reads the active stage/profile, derives its target, and
requires fresh register readback. The user confirmed the new capture interval
as the working baseline and the older capture as historical behavior that no
longer works in the current device state.

## Reproduce

```sh
nix-shell --run 'make -B'
make test-captures
```

The test requires Python 3, a C compiler, and tshark. It compiles and calls the
actual C encoder, checks USB handling with a mock, tests CLI/preset validation,
and extracts the captures directly with tshark. It never accesses real USB devices.
`make test` runs the code tests without tshark or captures.

For manual examination:

```sh
tshark -r mouse_data/new_table.pcap \
  -Y 'frame.number >= 662 && frame.number <= 7305' \
  -T fields -e frame.number -e frame.time_epoch -e usb.endpoint_address \
  -e usbhid.setup.bRequest -e usbhid.setup.wValue -e usbhid.setup.wIndex \
  -e usb.data_fragment -e usbhid.data -e usb.capdata

tshark -r mouse_data/new_table.pcap \
  -Y 'frame.number >= 705 && frame.number <= 741' -V -x

tshark -r mouse_data/goldgoldgold2.pcap \
  -Y 'frame.number == 1897 || frame.number == 5657 || frame.number == 9075' -V -x

tshark -r mouse_data/new_table.pcap -Y 'usb.idVendor || usb.idProduct' \
  -T fields -e frame.number -e usb.idVendor -e usb.idProduct
```

SET_REPORT payloads are `usb.data_fragment`, not `usb.capdata`. Device interrupt
replies are `usbhid.data`. The SET_REPORT setup fields are under `usbhid.setup`,
not `usb.setup`. USBPcap's data length of 25 includes the eight-byte setup packet;
the report sent to the device is 17 bytes, not 25.

## Capture identity and coverage

| File | Device | Sensitivity writes | Distinct DPI | Target byte 4 |
| --- | --- | ---: | --- | --- |
| `goldgoldgold2.pcap` | wired `3554:f54d`, descriptor frame 2 | 521 | all 520, 50 through 26000 | `0c` |
| `goldgoldgold.pcapng` | identical packet contents | 521 | identical | `0c` |
| `new_table.pcap`, frames 662–7305 | wireless `3554:f54f`, descriptor frame 8 | 523 | 519, 100 through 26000 | `10` |

The older files have identical timestamps, decoded USB records, and raw bytes for
all 11,798 packets. Their different filenames/container formats do not supply
independent protocol evidence. The test compares both decoded records and raw
packets. This establishes equivalence of these particular files, not that either
name proves a known-good physical DPI result.

The new sweep's first sensitivity write is frame 669: **100 DPI**. It has four
repeated values: 250, 10600, 10700, and 17650. Assigning `50 * ordinal` to writes
would therefore produce a wrong mapping. The only sensitivity write outside the
requested interval is frame 12495, representing 1650 DPI. The entire new capture
contains no 50-DPI write. Inspecting outside the interval was limited to descriptors
and checking whether the missing boundary write existed elsewhere.
Starting the sweep with 50 already selected could explain why it was not sent;
the initial software-visible selection cannot be recovered from these reports.

The old capture has a preliminary 100-DPI write at frame 1459, then 50 DPI at
1897, then the uninterrupted 100–26000 sequence starting at 2251 and ending at
9159. This covers the missing new boundary with actual capture evidence.

Capture SHA-256 values:

```text
df2cfbde20d2b4682cb5be21f445ca6e6b73e99a92dbee9c7811f85dcdb819ed  new_table.pcap
d500c23ee8362387737babf5b46a369282ed512b5ff3c5d6c5980e51fb673efd  goldgoldgold2.pcap
5adde1d470b985d8859045fb09d2ceb7a0f3f0e786f10192e483728a3d18b012  goldgoldgold.pcapng
```

## Report format, proven for equal-axis sensitivity settings

Both devices use HID Output SET_REPORT: `bmRequestType=21`, `bRequest=09`,
`wValue=0208`, `wIndex=0001`, `wLength=17`. Interface 1 has interrupt endpoint
`82` in both configuration descriptors (old frame 4, new frame 10).

| Offset | Captured meaning or constraint |
| --- | --- |
| 0 | report ID `08` |
| 1 | command `07` for sensitivity writes |
| 2–3 | constant `00 00`; public protocol identifies status and address high byte |
| 4 | address low byte: old `0c`, new `10`; public layout identifies consecutive DPI stages |
| 5 | `04`, length of the four data bytes at 6–9; confirmed by the public read protocol and live K1NG queries |
| 6–7 | duplicated low byte of `n = DPI / 50 - 1` |
| 8 | `(n >> 8) * 44` hex for the captured equal-axis settings |
| 9 | `(55 - byte6 - byte7 - byte8) & ff` hex |
| 10–15 | constant zero padding |
| 16 | `(55 - sum(bytes 0 through 15)) & ff` hex |

All 17-byte vendor reports, including command 03/04 requests and replies, have
byte sum `55` hex modulo 256. The sensitivity data also has its own four-byte
sum of `55` hex. These are additive checksums. No CRC, timestamp, or sequence
counter is needed to reproduce any captured sensitivity report.

The inner checksum makes the final checksum constant for a given target/header:
`e1` for target `0c`, `dd` for target `10`. It is not an arbitrary fixed trailer.
Changing target without recomputing the checksum would invalidate the report.

| DPI | Bytes 6–9 | Old frame | New frame |
| ---: | --- | ---: | ---: |
| 50 | `00 00 00 55` | 1897 | absent |
| 100 | `01 01 00 53` | 2251 | 669 |
| 12800 | `ff ff 00 57` | 5645 | 3931 |
| 12850 | `00 00 44 11` | 5657 | 3943 |
| 25600 | `ff ff 44 13` | 9063 | 7199 |
| 25650 | `00 00 88 cd` | 9075 | 7211 |
| 26000 | `07 07 88 bf` | 9159 | 7295 |

The rollover values constrain the high-bit encoding. This is neither a direct
little-endian nor big-endian DPI integer. Duplicated low bytes and duplicated high
bit positions suggest packed X/Y sensitivity, but no asymmetric-axis settings
were captured. The implementation deliberately supports only the existing single
DPI argument and does not assign unproven individual X/Y bit meanings.

The calculation reproduces all 520 entries of the pre-change LUT exactly. Tests
compare the compiled C encoder against all 521 old OUT reports, 521 old echoes,
523 new OUT reports and 519 new echoes in the requested interval. There is no
evidence that the protocol needs a lookup table. Wireless 50 DPI is calculated
using the same encoding plus the observed target/checksum difference; it has not
been independently captured at target `10`.

## Transactions and acknowledgement

Command 07 produces a byte-for-byte echo on endpoint 82. Every old sensitivity
write has one. The new interval has 523 writes and 519 echoes. In particular:

| DPI | First OUT | USB completion | Repeated OUT | Echo |
| ---: | ---: | ---: | ---: | ---: |
| 250 | 705 | 706 | 731 | 733 |
| 10600 | 3359 | 3360 | 3385 | 3387 |
| 10700 | 3393 | 3394 | 3419 | 3421 |
| 17650 | 5161 | 5162 | 5187 | 5189 |

The first OUT completes without an echo before the same payload is sent again
about one second later. An echo then follows the repeated write by about 10 ms.
Without sequence IDs it is not possible to identify which identical request a
late echo acknowledges. This demonstrates why a successful control transfer
alone is insufficient. The driver now requires a matching echo within a bounded
four-second wait, rejects short transfers, and returns failure to the shell on
USB/acknowledgement errors. It does not automatically retry writes.

An echo confirms a protocol response, not the active stage, measured sensor DPI,
or persistence after power loss. A stale identical echo also cannot be
distinguished by this protocol. No independent DPI/profile readback has been
identified in these captures. The implemented readback comes from the matching
public Compx register protocol, verified by actual K1NG read replies. It uses
SET_REPORT command 08 followed by interrupt IN, not HID GET_REPORT.

Command 03 always requests the same packet and returns the same packet, independent
of sensitivity. Command 04 is normally polled about every five seconds and returns
changing status-like data. It is not observed after every sensitivity write.
The new sweep has nine adjacent pairs of command-07 writes with no intervening
OUT command, including frames 731, 735, and 739, each with an echo. Thus the
capture does not establish a mandatory per-setting select/save/apply transaction.
These observations do not exclude startup state, a side effect of command 03,
or an internal firmware persistence process. Those need controlled hardware tests.
The code leaves status/battery commands alone.

## Existing flow, changes, and remaining targeting problem

Before this change, `main.c` accepted direct/interactive DPI or loaded a local
TOML preset. `config.c` read only a `dpi` integer. `run_driver` opened wired first,
then wireless, claimed interface 1, copied a nearest-match table entry and sent
one report. All entries targeted `0c`. Any nonnegative libusb transfer result
printed success, and driver failures still returned shell status zero.

The old table was valid arithmetic encoding at a single target. It could work
when that target affected the desired stage, or appear to work when only transfer
success was observed. Its arithmetic does not explain a failure to affect a
different stage/profile. The evidence establishes hardcoded targeting and missing
acknowledgement as limitations, but cannot prove the target's semantics or the
physical cause of a particular reported failure.

There is no second/new LUT in this checkout. The new capture has the same
sensitivity data with a different target and corresponding outer checksum.
Choosing `10` solely because the connected device is wireless would conflate
transport with potentially different selected profiles/stages in the recordings.

The implementation replaces the table with a validated calculation and resolves
the target from live configuration instead of retaining a fixed default:

```sh
sudo ./k1ng_driver 800
sudo ./k1ng_driver -p saved_name
```

The normal API is `run_driver(dpi)`. The optional `--dpi-address` argument is for
debugging only and still requires valid state and readback. Strict numeric
parsing rejects malformed/overflowing CLI and preset DPI values. The build
script no longer stages the deleted GUI or invokes its deleted installer.

The original checkout had no hardware-profile/battery read implementation;
`-p` remains a local TOML preset. The new state reader queries the active profile
without selecting or modifying any profile. Battery commands are not changed.

## Additional controlled captures required

The [resolver investigation](dpi-target-resolver.md) now includes successful
live configuration/profile reads. The capture procedures below are optional
independent validation; no new capture is needed merely to try the rebuilt driver.

Record each experiment separately, starting before launching the official
software and ending a few seconds after the single indicated action. State the
exact software-visible profile and DPI-stage labels. Preconfigure the baseline
before starting each recording. Keep polling rate, buttons, lighting, debounce,
axis linkage, other stages, and all unrelated settings unchanged. Keep X and Y
linked except in the last experiment.

| Experiment | Connection | Profile | Active stage | Starting DPI | Ending DPI | Only recorded change |
| --- | --- | ---: | ---: | ---: | ---: | --- |
| A | wireless | 1 | 1 | 800 | 850 | stage 1 sensitivity |
| B | wired | 1 | 1 | 800 | 850 | stage 1 sensitivity; otherwise identical baseline to A |
| C | wireless | 1 | 2 | 800 | 850 | stage 2 sensitivity |
| D | wireless | 2 | 1 | 800 | 850 | profile 2, stage 1 sensitivity |
| E | wireless | 1 | stays 1 at 800 | inactive stage 2 at 1600 | inactive stage 2 at 1650 | edit inactive stage 2 only, if the UI permits it |
| F | wireless | stays 1 | 1 to 2 | stage 1 at 800 | stage 2 already at 1600 | select stage 2 only; no DPI edits |
| G | wireless | 1 to 2 | stage 1 in both | profile 1 already at 800 | profile 2 already at 1600 | select profile 2 only; no DPI edits |
| H | wireless | 1 | 1 | 100 | 50 | stage 1 sensitivity, to capture the missing boundary |
| I (optional) | wireless | 1 | 1 | X=800, Y=800 | X=850, Y=800 | X only, with axes already unlinked before capture; needed only for future independent-axis support |

A versus B isolates transport. A versus C isolates DPI stage. A versus D isolates
profile. E/F/G distinguish editing stored state from selecting active state and
reveal any separate select/apply operations. If the official software has no
numbered hardware profiles, report that explicitly rather than relabeling DPI
stages as profiles. After A, separately verify motion at 850 DPI and after a
disconnect/reconnect. Record an explicit Apply/Save action separately if the UI
offers one; do not mix it into the single-change captures.

## Verification status

- The user confirmed that the automatic sensitivity implementation works on the
  real mouse. This is separate from the agent's read-only protocol measurements.
- **PROVEN FROM CAPTURE:** packet encoding/checksums, old-file equivalence,
  coverage and duplicate corrections, two distinct targets, interface/endpoint,
  echoes and missing-echo/repeated-write examples.
- **PROVEN FROM CODE/BUILD:** generated report equality and input/error handling
  passed `make test-captures`. A forced Nix-shell build passed with `-Wall -Wextra
  -Werror`; all source files compiled and linked without warnings. Formatting,
  `git diff --check`, and `bash -n build.sh` checks passed. Mock USB tests do not
  constitute hardware tests.
- **PROVEN FROM LIVE READS:** the connected wireless K1NG reports profile index
  2, three stages, active index 1, and a 400-DPI word at the derived target 0010.
  Exact transactions are preserved in the live-state fixture.
- **REQUIRES FURTHER HARDWARE VERIFICATION:** other stage/profile combinations,
  wired state reads, systematic full-range motion measurements, and power-cycle
  persistence. No live setting writes were made by the agent. USB nodes were
  accessible outside the sandbox, but root execution requires the user's sudo
  password; hidraw permission allowed the read-only protocol check.

## Battery query

```sh
sudo ./k1ng_driver --battery
```

`-b` and `-bat` are equivalent aliases for `--battery`.

The command requires the wired mouse (`3554:f54d`). If it cannot open the wired
device, it prints `driver: u have to be plugged to use this feature` and exits
unsuccessfully without querying the wireless receiver. Sensitivity commands
continue to support both connections.

For the wired mouse, the driver checks online state, then sends the captured
command-04 request:

```text
08 04 00 00 00 00 00 00 00 00 00 00 00 00 00 00 49
```

The response uses byte 6 for percentage (0–100) and byte 7 for charging (0/1).
The decoder checks report ID, command, status, address, declared payload length,
percentage/flag bounds, and the complete report's additive checksum. An echo of
the empty request is not accepted as battery data. Remaining response bytes are
not needed for percentage or charging state.

A live wireless query returned:

```text
08 04 00 00 00 02 41 00 0f 83 00 00 00 00 00 00 74
```

This is 65%, not charging. A later read after the mouse enumerated as wired
(`3554:f54d`) returned:

```text
08 04 00 00 00 02 5f 01 10 11 00 00 00 00 00 00 c6
```

The device itself reports 95%, charging, in that reply. Both exact request/reply
pairs are in the live-state fixture. They establish that the changing percentage
and charging flag originate in device telemetry, rather than in percentage
conversion or formatting. They do not establish the battery's physical charge
level or explain the firmware's estimation algorithm.

The command shows a ten-cell braille bar with `⣿` for full cells, `⣷`, `⣤`,
or `⡀` for a partial cell, and `⠀` for empty cells. While charging it prints,
for example, `battery: ⣿⣿⣿⣿⣿⣿⣿⣿⣿⣤  95% (charging; wired; device estimate)`;
otherwise it prints `battery: ⣿⣿⣿⣿⣿⣿⣤⠀⠀⠀  65% (not charging; wired)`.
The percentage is not cached,
smoothed, or recalculated from voltage. A different estimate after connecting
USB must not be presented as a measured gain in stored charge.

`run_battery()` is independent of DPI/profile reads and performs no settings
writes. It shares the driver's existing USB open/claim/cleanup and response
validation. Tests cover all 125 battery replies in the two distinct recordings,
both charging states, 0% and 100%, invalid replies, timeouts, short transfers,
rejection of wireless-only and absent devices, and the requirement that battery
queries never write a sensitivity register.
