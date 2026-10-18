# DPI target resolver: protocol evidence and implementation

## Hardware counterexample

The user tested the current Linux driver with requests for 50 and 400 DPI at
target `0c`. Both received a matching report echo; neither changed active DPI.
The connection, profile, stage, and previous active DPI were not reported.

This establishes:

```text
USB control transfer completed != device echoed the operation != active DPI changed
```

The old fixed-target driver and its mocks described protocol completion only.
The new driver requires state reads, resolves the active stage, and checks the
stored DPI word after writing. It never infers active-stage correctness from an
ACK. The user subsequently confirmed `new_table.pcap` frames 662–7305 as the
working official-software baseline; the old recording is a historical comparison.

## Additional primary sources

The captures alone do not include state reads. Public implementations supply the
missing register protocol, which was then checked on the actual K1NG:

- [Compx protocol notes](https://github.com/Brunovncs/compx-tray/blob/3a0d64d55b353ca8dd513398e9574f340078487f/docs/protocol.md),
  with its [EEPROM implementation](https://github.com/Brunovncs/compx-tray/blob/3a0d64d55b353ca8dd513398e9574f340078487f/crates/compx-proto/src/eeprom.rs).
- [ClickSync's Compx/ATK implementation](https://github.com/Nuitfanee/ClickSync/blob/907d79e92fe96d3ac3f1485e00e3ed6503bda7e4/src/protocols/protocol_api_atk.js),
  specifically `ProtocolCodec.read`, `REGS.SYS_CONFIG`, `dpiProfile`, and `requestConfig`.

Both describe report ID 08, command 08 register reads, command 07 writes, the
same outer and data checksums, a DPI table starting at 000c, and four-byte DPI
stages. Configuration pairs at addresses 0000, 0002, and 0004 hold polling rate,
stage count, and zero-based active stage. Each pair sums to 55 hex modulo 256.
The Compx implementation also documents command 0e for the active profile and
register addressing relative to that profile. The application does not select
another profile or alter the stage index.

```text
target = 0x000c + 4 * active_stage_index
```

Thus the captured 000c and 0010 writes fit stages 1 and 2, respectively. The
new recording confirms the second target works in that recorded state; it does
not imply wireless devices should always use 0010. Sensor-specific encoding in
the external projects is not copied: this driver retains the calculation matched
against every sensitivity write in the supplied captures.

## Read-only verification on the actual K1NG

On the connected wireless receiver `3554:f54f`, a read-only hidraw probe returned:

| Query | Request | Response |
| --- | --- | --- |
| Online | `080300000000000000000000000000004a` | `08030000000101ac19660000000000001d` |
| Configuration | `080800000006000000000000000000003f` | `0808000000060154035201540000000040` |
| Active DPI word | `0808000010040000000000000000000031` | `08080000100407070047000000000000dc` |
| Profile | `080e00000000000000000000000000003f` | `080e00000001020000000000000000003c` |

Configuration data `01 54 03 52 01 54` has three valid pairs: polling code 01,
stage count 3, active stage index 1. The derived target is 0010. Its DPI word
`07 07 00 47` decodes to X=400, Y=400. The profile reply reports index 2. These
are actual device replies, preserved in `tests/fixtures/k1ng-live-state.json`,
not constructed capture packets. The probe sent only commands 03, 08, and 0e;
no settings were written.

Earlier `/dev/bus/usb` and hidraw paths were hidden inside the sandbox. The actual
hidraw vendor interface is accessible outside it. Direct libusb access as the
session's unprivileged user was denied; `sudo -n` requires a password. Consequently
the agent could not run the root/libusb executable end to end. The user has since
confirmed that the automatic sensitivity implementation works on the actual mouse.

## Runtime behavior

The normal API is `run_driver(dpi)`. It performs:

1. The captured online query (03), validating the reply's framing, checksum,
   length, and Boolean value. For DPI changes its 0/1 flag is advisory; the
   subsequent configuration/profile reads determine whether usable state is
   available.
2. A six-byte configuration read at 0000 and an active-profile query (0e).
3. Validation of report framing, length, address, status, outer checksum, value
   pairs, stage count/index, and profile index, followed by target calculation.
4. A read of the selected stage's current DPI word, checking its checksum and
   the captured sensor encoding before any setting write.
5. A fresh profile/stage check, then one four-byte sensitivity write.
6. Exact write-echo matching, a fresh register readback matching the requested
   four bytes, and another profile/stage check.

Queued replies are discarded before each request because this protocol has no
transaction IDs. An asynchronous interrupt IN is then submitted before sending
the control report, allowing the reply to arrive while SET_REPORT is still in
progress. The transfer is completed or cancelled and drained before its buffer
is released. The queue drain and response waits are bounded. No address scan,
fixed-target fallback, profile switch, stage switch, automatic write retry, or
battery-setting change is performed. Unreadable or inconsistent state prevents
the setting write. A readback failure after a write returns failure without
claiming rollback or unchanged device state.

The user reported repeated valid command-03 replies with online value 0:
`08030000000100ac19660000000000001e`. The earlier driver stopped immediately,
without attempting a configuration read. DPI changes now try the required fresh
state reads after either valid Boolean response. This does not interpret 0 as
online or accept an ACK as proof of a changed sensor setting. Failed or invalid
state reads still prevent every settings write, and successful writes still
require register readback and unchanged profile/stage. Battery queries retain
their online requirement and remain wired-only.

The reported zero-flag failure has not been reproduced by the agent's live
read-only probe: the receiver currently returns 1 and valid configuration,
profile, and 400-DPI data. Tests replay the exact reported zero-flag packet with
both responsive and unreachable state reads; those scenarios are simulated and
do not establish why the receiver previously reported 0. Real-hardware
confirmation of this failure path remains required.

## Interrupt receive ordering

Direct USBPcap inspection establishes a transport difference from the original
synchronous Linux implementation. In `new_table.pcap` frames 662–7305, all 1,145
vendor commands (570 online queries, 52 battery queries, 523 writes) have a
vendor interrupt-IN request already pending when SET_REPORT is submitted.
For example, the read submitted at frame 666 receives the online reply at frame
676, after the command at frame 675 but before its control completion at frame
678. The earlier Linux implementation only submitted its reply read after the
control transfer returned.

The driver now reproduces the captured order by queuing interrupt IN before
the control transfer. This removes that receive-ordering difference without
changing register addresses, report encoding, or response validation.
The possibility that late queuing caused the reported intermittent timeouts is
a hypothesis, not an established hardware result. Read-only hidraw queries,
which use the kernel's active input reader, answer consistently in the agent's
checks; they do not test libusb's interface-detach path.

Reproduce the frame ordering with:

```sh
tshark -r mouse_data/new_table.pcap -Y 'frame.number >= 663 && frame.number <= 678' -V
make test-captures
```

The automated capture audit tracks pending interrupt requests by USBPcap IRP ID,
verifies every command in the specified interval, and counts replies arriving
before control completion. The USB mock additionally delivers replies from
inside the control-transfer call and checks allocation, submission, timeout,
cancellation, event-error handling, and release of pending transfers. These are
transport regression tests; confirmation on the failing real libusb connection
still requires the user's privileged run. The complete USB mock suite also
passes AddressSanitizer, LeakSanitizer, and UndefinedBehaviorSanitizer outside
the sandbox, where LeakSanitizer can inspect threads.

## Initial missed reads

The user's subsequent hardware log still shows initial configuration-read
timeouts, followed by successful commands completing immediately. This means
receive ordering alone did not resolve the intermittent failure. The four-second
pause in the failing logs was the read timeout, not a configured startup delay.
The records do not establish a fixed firmware initialization time or prove that
sleep is the cause.

The pinned [Compx request implementation](https://github.com/Brunovncs/compx-tray/blob/3a0d64d55b353ca8dd513398e9574f340078487f/crates/compx-proto/src/device.rs)
documents dropped initial packets from sleeping mice and retries requests three
times with a one-second reply timeout. This driver now applies that bounded
recovery to read commands 03, 08, and 0e only, and only after timeout. It prints
`waiting for mouse response; try moving or clicking your mouse` after the first
timeout of a retried read. Reads still retry automatically, with a final
move/click suggestion after all three attempts fail. There is no fixed startup
sleep. Valid replies proceed
immediately; malformed replies and non-timeout errors still fail immediately.
Every retry drains pending input and requeues interrupt IN before sending.
Battery command 04 and settings write 07 keep their single-attempt behavior.

Mock tests cover one or two dropped initial online/configuration/profile reads,
eventual success with exactly one setting write, and permanent read failures
stopping after three attempts with no setting write. Missing write ACKs are not
retried. Readback and profile/stage checks still run after the write. This is
read-recovery handling based on a matching protocol implementation; the reason
for the K1NG's initial missing replies and hardware effectiveness of the recovery
remain to be verified, including whether moving/clicking before the first
command prevents the failure.

## Host-triggered wake remains unproven

A later user run completed after the automatic retry, with a matching write ACK
and 400-DPI register readback. That demonstrates recovery, but does not establish
whether USB traffic woke the mouse, the wireless link became ready independently,
or physical input occurred during the wait. The captures contain no separately
identified wake command. Command 01 in the external Compx implementation is an
identity handshake; it is not demonstrated as a K1NG wake operation.

The [M916 K1NG Pro 4K manual](https://manuals.plus/redragon/m916-k1ng-pro-4k-wireless-gaming-mouse-manual.pdf)
describes sleep after one minute of inactivity and wake through movement or a
button. This does not prove that host wake is impossible. The connected
receiver's Linux runtime-power state was also `on`/`active` when checked, so
there is no evidence here for adding a USB autoresume workaround.

To investigate an actual wake sequence, make a separate full USBPcap recording:

1. Use the 4K wireless receiver, with the USB cable disconnected, profile 3,
   active DPI stage 2 of 3, and starting/ending DPI both 400. Close the official
   software before the experiment.
2. Start recording, then leave the mouse untouched for at least two minutes
   (or longer than the configured sleep timeout). Record the actual idle time.
3. Launch the official software using only the keyboard. Keep recording for
   ten seconds and note whether it reads live configuration while the mouse
   remains untouched. Do not move/click the mouse or change DPI, stage, profile,
   polling rate, sleep settings, buttons, or lighting.
4. If the software cannot connect untouched, repeat as a separate recording,
   moving the mouse once after software launch and recording that time. Keep
   every other condition identical.

The single tested action in the first capture is official-software startup;
there is no sensitivity change. This separates startup traffic from physical
wake and permits testing whether any observed exchange actually causes wake.

`--dpi-address` remains a debug override, restricted to aligned stage addresses
within the device's reported count. It does not bypass state validation or
readback. An inactive debug target is explicitly labeled. Normal users need only:

```sh
sudo ./k1ng_driver 800
```

A successful operation reports register readback and unchanged reported
profile/stage. It does not claim measured sensor DPI or power-cycle persistence.
Other applications or physical stage/profile changes can race these operations;
rechecks detect observable changes, but the protocol provides no atomic lock.

## Verification boundaries

- All new sensitivity writes in frames 662–7305 match the actual C encoder.
- Real K1NG read replies validate the configuration/profile format, active index
  1, derived target 0010, and decoding of its current DPI word.
- Mock tests cover every documented stage/profile/transport combination, all
  valid DPI values at every stage address, stale queues, malformed reads, state
  changes, and an ACK-without-apply operation that must fail readback.
- The read fixtures prove only the observed wireless state. Other stage/profile
  combinations are simulated, not independently observed on this K1NG.
- The user confirmed actual sensitivity changes with the rebuilt driver. Other
  stage/profile combinations, full-range motion measurements, and persistence
  still need independent verification. No live setting writes were made by the
  agent.

## Complete capture audit

The audit inspected every packet, including records outside the sensitivity sweep,
in both distinct recordings. The old PCAPNG was also re-extracted and checked for
raw-packet equivalence. No packets were selected by filename-based assumptions.

```sh
make test-captures
tshark -r mouse_data/new_table.pcap -T jsonraw -j frame > /tmp/k1ng-new-raw.json
tshark -r mouse_data/goldgoldgold2.pcap -T jsonraw -j frame > /tmp/k1ng-old-raw.json
tshark -r mouse_data/new_table.pcap \
  -Y 'frame.number == 451 || frame.number == 452 || frame.number == 455 || frame.number == 457 || frame.number == 12489 || frame.number == 12491 || frame.number == 12495 || frame.number == 12499' \
  -V -x
```

`tests/check_protocol.py` parses the complete USBPcap records emitted by tshark,
not just the previously extracted 17-byte reports. It checks all transfer types,
control setups, data-bearing control responses, and interrupt endpoints for the
mouse's device address. This detects hidden reads or larger structures that the
earlier narrow extraction could have missed.

| Whole-recording evidence | Old wired capture | New wireless capture |
| --- | --- | --- |
| Total frames, all devices | 11798 | 15567 |
| Mouse device address | 1 | 2 |
| Command 03 OUT / IN | 592 / 592 | 574 / 574 |
| Command 04 OUT / IN | 71 / 71 | 55 / 54 |
| Command 07 OUT / IN | 521 / 521 | 524 / 520 |
| Command 07 targets | only `0c` | only `10` |
| Other runtime control commands | none | none |
| Vendor interrupt reports | all 17 bytes on endpoint `82` | all 17 bytes on endpoint `82` |
| Other mouse input | 3528 seven-byte reports on `83` | 5477 seven-byte reports on `83` |
| Bulk or isochronous transfers | none | none |

The only standard control requests for the mouse are GET_DESCRIPTOR(Device),
GET_DESCRIPTOR(Configuration), and SET_CONFIGURATION. The only data-bearing
control responses are the 18-byte device and 84-byte configuration descriptors.
There is no captured HID GET_REPORT, HID report descriptor read, vendor control
IN request, additional register-read command, or larger profile/configuration dump.
Commands 03 and 04 are sent with SET_REPORT and answered through interrupt IN;
the absence of a control IN request does not exclude those commands as queries.

The initial six descriptor/configuration records for each mouse all have the same
timestamp and zero IRP IDs. They occur at old frames 1–6 and new frames 7–12.
The first vendor operation is command 03 at old frame 1453, about 3.66 seconds
later, or new frame 451, about 1.43 seconds later. These records do not establish
that the capture contains actual device power-on or a complete software launch.
Missing operations cannot be inferred from the descriptor records.

### Command 03

Every reply in both complete recordings is exactly:

```text
08 03 00 00 00 01 01 ac 19 66 00 00 00 00 00 00 1d
```

Examples: old frame 1454; new frames 452, 12491, and 13451. This same input cannot
by itself explain why the official software chose different write targets. Its
unknown fields must not be labeled as an active stage or profile without an
experiment that changes those states.

### Command 04

These are all distinct replies, not samples chosen from a larger set:

```text
old: 08 04 00 00 00 02 64 01 10 2a 00 00 00 00 00 00 a8
old: 08 04 00 00 00 02 64 01 10 2f 00 00 00 00 00 00 a3
new: 08 04 00 00 00 02 46 00 0f 87 00 00 00 00 00 00 6b
new: 08 04 00 00 00 02 46 00 0f 90 00 00 00 00 00 00 62
new: 08 04 00 00 00 02 46 00 0f 94 00 00 00 00 00 00 5e
```

Offsets 6–8 are constant within each recording and differ between recordings.
Offset 9 varies while the sensitivity-write target remains constant. Offset 16
is the already-established checksum. These fields may include useful state, but
there is no labeled stage/profile transition to determine which bits, if any,
identify it. Values 100/70 and 1/0 are compatible with battery/charging state,
but that interpretation is also not proven and is not used as resolver logic.

### Other input and existing code

Every endpoint-83 payload is seven bytes. Byte 0 ranges over 0–3 in the old
capture and 0–1 in the new capture; the final two bytes are always zero. The
middle four bytes have values consistent with two signed movement coordinates.
There is no separately identifiable stage-change notification in these records.
The HID report descriptor is absent, so no vendor-stage usage can be established
from it. These observations do not prove the firmware cannot expose stage state.

The current source has no hardware-profile or battery read function. `config.c`
reads only local TOML DPI presets. The deleted GUI's historical `DriverAPI` only
invoked the same executable with DPI/preset arguments; its preset manager also
handled local TOML. No existing state reader was found there to reuse.

## What the captures alone cannot establish

Neither recording supplies a labeled stage/profile transition, and every write
within a recording uses the same target. Device PID, command-04 state, selected
software state, and target all differ together between recordings. A resolver
could be made to fit those two sessions in many incompatible ways. There is no
evidence selecting one such relationship, nor evidence proving that a captured
write target was active instead of merely being edited in the software.

The public register layout and real K1NG queries above supply the missing state
reader. They are additional evidence, not a reinterpretation of bytes absent
from the PCAPs. A second accepted address or an echo alone would not have been
sufficient. Further captures remain useful for independently checking additional
stages, profiles, and the wired path.

## Optional independent capture validation

If additional PCAP validation is needed, use these three separate recordings.
The resolver no longer requires the user to make them before testing the build.
Number stages by their visible order starting at
1, and record their actual UI labels. If the software has no hardware profiles,
say so and omit C; do not rename DPI stages as profiles. Record action timestamps
and whether the software's active indicator and physical sensitivity agree.

For every recording, capture the complete USB bus/device traffic, including all
interfaces, both directions, and full packet lengths. Begin before reconnecting
the mouse/receiver and before launching the official software. Keep X/Y linked,
stage count, other DPI values, polling rate, button assignments, debounce, lighting,
and all unrelated settings unchanged. Preconfigure the baseline before recording;
do not drag sliders through intermediate values. Use exact value entry if offered.

### A: wireless, profile 1, two known DPI stages

Baseline: profile 1; stage 1 = 800 DPI; stage 2 = 1600 DPI; stage 1 active;
official software fully exited. Do not change the profile during this recording.

1. Start capture, reconnect the receiver, launch the official software, and wait
   five seconds without editing settings. Confirm stage 1 is shown active at 800.
2. Change only stage 1 DPI from 800 to 850. Wait five seconds.
3. Activate stage 2 without editing its 1600 DPI value. Wait five seconds.
   Use the official software's active-stage selector if supported; if it has none,
   press the physical DPI button the minimum number of times and record each press
   and the resulting active-stage label. Clicking an editor tab alone is not proof
   that the device's active stage changed.
4. Change only stage 2 DPI from 1600 to 1650. Wait five seconds.
5. Exit and relaunch the official software while stage 2 remains active. Do not
   edit or switch stages during this restart. Wait five seconds. This is the
   critical comparison with startup at stage 1.
6. Activate stage 1 again, without editing its now-850 DPI value. Wait five seconds.
7. If the UI permits editing an inactive stage without activating it, change only
   inactive stage 2 from 1650 to 1700 while stage 1 remains active at 850. Wait five
   seconds and record whether physical sensitivity remains unchanged. Otherwise
   skip this action and state that the UI cannot keep the edited stage inactive.
8. Stop capture. Record which edits actually changed physical sensitivity. Do not
   switch stages again inside this recording.

### B: wired, profile 1, same baseline and actions

Disconnect the wireless receiver. Restore profile 1 stage 1 = 800, stage 2 = 1600,
stage 1 active, and close the software before recording. Repeat A exactly with
the mouse connected by USB cable, including the software restart while stage 2
is active. This isolates transport from stage selection. Do not reuse A's final
850/1650/1700 values as B's baseline.

### C: wireless, explicit profile selection

Preconfigure profiles 1 and 2 with stage 1 = 800 and stage 2 = 1600, and select
stage 1 in both. Leave profile 1 active and exit the software before recording.

1. Start capture, reconnect the receiver, launch the software, and wait five seconds.
2. Select profile 2 only. Do not edit DPI or change stage. Wait five seconds;
   record whether stage 1 is still shown active and whether selecting a profile
   applies it to the mouse or merely opens a software editor.
3. With profile 2 stage 1 active, change its DPI from 800 to 850 only. Wait five seconds.
4. Exit/relaunch the software without changing profile 2 or stage 1. Wait five seconds.
5. Select profile 1 only, keeping stage 1 active at its original 800. Wait five seconds.
6. Change profile 1 stage 1 from 800 to 850 only. Wait five seconds and stop capture.

If applying a profile requires a distinct Apply/Save button, record selection and
that button press as separate timestamped actions with five seconds between them;
do not silently combine them. Keep its other stored settings identical between
profiles so profile selection does not intentionally change unrelated settings.

These experiments can reveal startup/read differences for two known stages,
stage-only transitions, edit-versus-activation behavior, profile selection and
transport differences. If the software never requests active state even here,
that is a useful negative result, not permission to infer an address formula.
