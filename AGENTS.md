# Repository guidance

This repository contains a C/libusb command-line driver for the Redragon K1NG mouse.

## Read the documentation first

Read the relevant documents in [docs/](docs/) before changing driver behavior:

- [DPI target resolver](docs/dpi-target-resolver.md): active stage/profile reads,
  target selection, USB receive ordering, bounded read recovery, and hardware
  verification limits.
- [Sensitivity protocol](docs/sensitivity-protocol.md): report encoding and
  checksums, capture evidence, CLI/preset behavior, battery queries, and
  reproducible verification commands.

These documents include historical behavior and unresolved hypotheses. Check
their current runtime and verification sections against the implementation.

## Preserve the documented safeguards

- Resolve the DPI target from live stage/profile state. Do not choose a fixed
  address based on wired or wireless connection alone.
- Preserve response validation, matching write echoes, register readback, and
  profile/stage rechecks. An ACK alone does not prove active sensitivity changed.
- Queue interrupt IN before sending the control report. Keep retries bounded
  and limited to the documented read commands; never automatically retry writes.
- Keep DPI values within 50 through 26000 in steps of 50. `-p` selects a local
  TOML preset, not a hardware profile.
- Keep battery queries read-only and wired-only.

## Verification

- Build with `nix-shell --run 'make -B'`.
- Run `make test` for encoder, resolver, USB mock, and CLI checks. It requires
  Python 3 and a C compiler and does not access real USB devices.
- Run `make test-captures` for protocol/capture changes; it also requires tshark.
- Keep capture evidence, live-device fixtures, simulated tests, and actual
  hardware verification distinct when reporting results. Update the relevant
  documentation when protocol behavior or supporting evidence changes.
