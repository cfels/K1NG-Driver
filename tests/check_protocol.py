#!/usr/bin/env python3
import argparse
import collections
import csv
import ctypes
import io
import json
import os
from pathlib import Path
import shlex
import struct
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
VALID_DPI = set(range(50, 26001, 50))
FIELDS = [
    "frame.number", "frame.time_epoch", "usb.endpoint_address",
    "usbhid.setup.bRequest", "usbhid.setup.wValue", "usbhid.setup.wIndex",
    "usb.data_fragment", "usbhid.data", "usb.capdata",
]


class BatteryInfo(ctypes.Structure):
    _fields_ = [("percentage", ctypes.c_ubyte), ("charging", ctypes.c_bool)]


def run(command, **kwargs):
    result = subprocess.run(command, text=True, capture_output=True, **kwargs)
    if result.returncode != 0:
        raise RuntimeError(f"{shlex.join(map(str, command))}\n{result.stdout}{result.stderr}")
    return result.stdout


def extract(path):
    command = ["tshark", "-r", str(path), "-T", "fields"]
    for field in FIELDS:
        command.extend(["-e", field])
    return list(csv.reader(io.StringIO(run(command)), delimiter="\t"))


def audit_state_traffic(raw_packets, device, name, expected_out, expected_in, status_reports):
    vendor_out = collections.Counter()
    vendor_in = collections.Counter()
    replies = collections.defaultdict(set)
    targets = set()
    standard_requests = collections.Counter()
    descriptor_lengths = collections.Counter()
    motion_reports = 0
    pending_interrupts = set()
    pending_controls = {}
    queued_reads = collections.Counter()
    early_replies = 0
    for frame, raw in enumerate(raw_packets, 1):
        packet = bytes.fromhex(raw)
        header_length = struct.unpack_from("<H", packet)[0]
        if struct.unpack_from("<H", packet, 19)[0] != device:
            continue
        payload = packet[header_length:]
        endpoint, transfer_type = packet[21:23]
        irp = struct.unpack_from("<Q", packet, 2)[0]
        relevant = name == "new" and 662 <= frame <= 7305
        if transfer_type == 1 and endpoint == 0x82:
            if packet[16] & 1:
                pending_interrupts.discard(irp)
            else:
                pending_interrupts.add(irp)
        if transfer_type == 2:
            assert header_length == 28, (name, frame)
            if packet[27] == 3:
                pending_controls.pop(irp, None)
            if packet[27] == 0 and payload:
                setup = struct.unpack("<BBHHH", payload[:8])
                if setup == (0x21, 0x09, 0x0208, 1, 17):
                    report = payload[8:]
                    assert len(report) == 17 and report[0] == 8
                    vendor_out[report[1]] += 1
                    pending_controls[irp] = report[1]
                    if relevant:
                        assert pending_interrupts, (name, frame, "no interrupt IN queued")
                        queued_reads[report[1]] += 1
                    if report[1] == 7:
                        targets.add(report[4])
                else:
                    standard_requests[setup] += 1
            elif payload:
                assert packet[27] == 3 and endpoint == 0x80
                descriptor_lengths[len(payload)] += 1
        else:
            assert transfer_type == 1, (name, frame, transfer_type)
            if not payload:
                continue
            if endpoint == 0x82:
                assert len(payload) == 17 and payload[0] == 8
                vendor_in[payload[1]] += 1
                replies[payload[1]].add(payload.hex())
                if relevant and payload[1] in pending_controls.values():
                    early_replies += 1
            else:
                assert endpoint == 0x83 and len(payload) == 7
                assert payload[0] in [0, 1, 2, 3] and payload[5:] == bytes(2)
                motion_reports += 1
    assert standard_requests == {
        (0x80, 6, 0x0100, 0, 18): 1,
        (0x80, 6, 0x0200, 0, 84): 1,
        (0x00, 9, 1, 0, 0): 1,
    }
    assert descriptor_lengths == {18: 1, 84: 1}
    assert vendor_out == expected_out and vendor_in == expected_in
    assert replies[3] == {"08030000000101ac19660000000000001d"}
    assert replies[4] == status_reports
    assert targets == ({0x0C} if name == "old" else {0x10})
    if name == "new":
        assert queued_reads == {3: 570, 4: 52, 7: 523}
        assert early_replies > 0
        print(f"New frames 662-7305: all {sum(queued_reads.values())} commands have interrupt IN "
              f"queued before SET_REPORT; {early_replies} replies precede control completion")
    print(f"Full {name} audit: OUT {dict(vendor_out)}, IN {dict(vendor_in)}, "
          f"{motion_reports} seven-byte mouse reports; no additional read command or profile dump")


def capture_tests(encode, decode_battery):
    old_path = ROOT / "mouse_data/goldgoldgold2.pcap"
    ng_path = ROOT / "mouse_data/goldgoldgold.pcapng"
    new_path = ROOT / "mouse_data/new_table.pcap"
    old, ng, new = map(extract, [old_path, ng_path, new_path])
    assert old == ng, "older decoded USB records differ"
    raw = []
    for path in [old_path, ng_path, new_path]:
        packets = json.loads(run(["tshark", "-r", str(path), "-T", "jsonraw", "-j", "frame"]))
        raw.append([packet["_source"]["layers"]["frame_raw"][0] for packet in packets])
    assert raw[0] == raw[1], "older raw packets differ"
    print(f"Older captures: {len(old)} identical raw packets and decoded USB records")
    audit_state_traffic(raw[0], 1, "old", {3: 592, 4: 71, 7: 521},
                       {3: 592, 4: 71, 7: 521}, {
                           "0804000000026401102a000000000000a8",
                           "0804000000026401102f000000000000a3",
                       })
    audit_state_traffic(raw[2], 2, "new", {3: 574, 4: 55, 7: 524},
                       {3: 574, 4: 54, 7: 520}, {
                           "08040000000246000f870000000000006b",
                           "08040000000246000f9000000000000062",
                           "08040000000246000f940000000000005e",
                       })
    print("Captured writes match the working new baseline; active-stage reads are absent from these PCAPs.")
    batteries = 0
    for row in old + new:
        if row[2] == "0x82" and row[7].startswith("0804"):
            report = bytes.fromhex(row[7])
            assert decode_battery(report) == (report[6], bool(report[7]))
            batteries += 1
    assert batteries == 125
    print(f"Battery decoder: all {batteries} captured wired/wireless replies match")

    for name, rows, address, expected in [
        ("old wired", old, 0x0C, VALID_DPI),
        ("new wireless, frames 662-7305", [r for r in new if 662 <= int(r[0]) <= 7305],
         0x10, VALID_DPI - {50}),
    ]:
        seen = set()
        counts = collections.Counter()
        commands = collections.Counter()
        values = collections.Counter()
        for row in rows:
            for col in [6, 7, 8]:
                if not row[col]:
                    continue
                payload = bytes.fromhex(row[col])
                if len(payload) != 17 or payload[0] != 8:
                    continue
                assert sum(payload) % 256 == 0x55, (name, row[0], "report checksum")
                if col == 6:
                    assert row[2:6] == ["0x00", "0x09", "0x0208", "1"]
                    commands[payload[1]] += 1
                if payload[1] != 7:
                    continue
                assert payload[6] == payload[7] and payload[8] in [0, 0x44, 0x88]
                assert sum(payload[6:10]) % 256 == 0x55
                dpi = (payload[6] + (payload[8] // 0x44) * 256 + 1) * 50
                assert dpi in VALID_DPI and payload == encode(dpi, address), (name, row[0])
                counts["OUT" if col == 6 else "IN"] += 1
                if col == 6:
                    seen.add(dpi)
                    values[dpi] += 1
        assert seen == expected, (name, "missing", sorted(expected - seen))
        print(f"{name}: {len(seen)} DPI values, {dict(counts)} byte-exact C encoder matches")
        print(f"  commands: {dict(commands)}; repeated DPI writes: "
              f"{dict((dpi, count) for dpi, count in values.items() if count > 1)}")
    assert not any(r[6].startswith("0807") and bytes.fromhex(r[6])[6:9] == bytes(3) for r in new)
    print("50 DPI is absent from all new-capture sensitivity writes; verified in old frame 1897")


def code_tests(directory):
    cc = shlex.split(os.environ.get("CC", "cc"))
    flags = ["-Wall", "-Wextra", "-Werror", "-Iincludes", "-Isrc",
             "-Iincludes/tomlc17-R260618/src"]
    library_path = directory / "data.so"
    run(cc + flags + ["-shared", "-fPIC", "src/data.c", "-o", str(library_path)], cwd=ROOT)
    library = ctypes.CDLL(str(library_path))
    library.get_payload.argtypes = [ctypes.c_int, ctypes.c_ubyte, ctypes.POINTER(ctypes.c_ubyte)]
    library.get_payload.restype = ctypes.c_int
    library.resolve_dpi_target.argtypes = [ctypes.POINTER(ctypes.c_ubyte), ctypes.POINTER(ctypes.c_ubyte)]
    library.resolve_dpi_target.restype = ctypes.c_int
    library.get_read_payload.argtypes = [ctypes.c_ubyte, ctypes.c_ubyte, ctypes.POINTER(ctypes.c_ubyte)]
    library.get_read_payload.restype = ctypes.c_int
    library.get_profile_payload.argtypes = [ctypes.POINTER(ctypes.c_ubyte)]
    library.get_profile_payload.restype = None
    library.decode_battery_report.argtypes = [ctypes.POINTER(ctypes.c_ubyte), ctypes.POINTER(BatteryInfo)]
    library.decode_battery_report.restype = ctypes.c_int

    def encode(dpi, address):
        buffer = (ctypes.c_ubyte * 17)()
        assert library.get_payload(dpi, address, buffer) == 0
        return bytes(buffer)

    def decode_battery(report):
        assert len(report) == 17
        data = (ctypes.c_ubyte * 17)(*report)
        info = BatteryInfo()
        if library.decode_battery_report(data, ctypes.byref(info)) != 0:
            return None
        return info.percentage, info.charging

    for address in range(0x0C, 0x29, 4):
        reports = [encode(dpi, address) for dpi in sorted(VALID_DPI)]
        assert len(set(reports)) == 520
        if address in [0x0C, 0x10]:
            assert reports[0] == bytes.fromhex(
                "080700000c0400000055000000000000e1" if address == 0x0C
                else "08070000100400000055000000000000dd")
            assert reports[-1] == bytes.fromhex(
                "080700000c04070788bf000000000000e1" if address == 0x0C
                else "080700001004070788bf000000000000dd")
        for report in reports:
            assert sum(report) % 256 == 0x55
        for dpi in [-2147483648, -50, 0, 49, 51, 125, 26001, 26050, 2147483647]:
            buffer = (ctypes.c_ubyte * 17)(*[0xAA] * 17)
            assert library.get_payload(dpi, address, buffer) == -1
            assert bytes(buffer) == bytes([0xAA] * 17)
    assert library.get_payload(50, 0x0C, None) == -1
    for address in [0, 0x08, 0x0D, 0x2C, 0xFF]:
        assert library.get_payload(50, address, (ctypes.c_ubyte * 17)()) == -1

    for count in range(1, 9):
        for stage in range(count):
            config = (ctypes.c_ubyte * 6)(1, 0x54, count, (0x55 - count) & 255,
                                        stage, (0x55 - stage) & 255)
            address = ctypes.c_ubyte()
            assert library.resolve_dpi_target(config, ctypes.byref(address)) == 0
            assert address.value == 0x0C + 4 * stage
    for values in [[1, 0x54, 0, 0x55, 0, 0x55], [1, 0x54, 9, 0x4C, 0, 0x55],
                   [1, 0x54, 6, 0x4F, 6, 0x4F], [1, 0x54, 6, 0, 1, 0x54],
                   [3, 0x52, 6, 0x4F, 1, 0x54], [0, 0x55, 6, 0x4F, 1, 0x54]]:
        address = ctypes.c_ubyte(0xFF)
        assert library.resolve_dpi_target((ctypes.c_ubyte * 6)(*values), ctypes.byref(address)) == -1
        assert address.value == 0xFF
    assert library.resolve_dpi_target(None, ctypes.byref(ctypes.c_ubyte())) == -1

    request = (ctypes.c_ubyte * 17)()
    assert library.get_read_payload(0, 6, request) == 0
    assert bytes(request) == bytes.fromhex("080800000006000000000000000000003f")
    assert library.get_read_payload(0x10, 4, request) == 0
    assert bytes(request) == bytes.fromhex("0808000010040000000000000000000031")
    library.get_profile_payload(request)
    assert bytes(request) == bytes.fromhex("080e00000000000000000000000000003f")
    for address, length in [(0, 0), (0, 11), (255, 2)]:
        assert library.get_read_payload(address, length, request) == -1

    live = json.loads((ROOT / "tests/fixtures/k1ng-live-state.json").read_text())
    for key in ["online", "configuration", "dpi_register", "profile", "battery", "battery_wired"]:
        for direction in ["request", "response"]:
            packet = bytes.fromhex(live[key][direction])
            assert len(packet) == 17 and sum(packet) % 256 == 0x55
    configuration = bytes.fromhex(live["configuration"]["response"])[6:12]
    target = ctypes.c_ubyte()
    assert library.resolve_dpi_target((ctypes.c_ubyte * 6)(*configuration), ctypes.byref(target)) == 0
    assert configuration[2] == live["stage_count"] and configuration[4] == live["stage_index"]
    assert target.value == live["target"]
    assert bytes.fromhex(live["profile"]["response"])[6] == live["profile_index"]
    assert encode(live["dpi"], target.value)[6:10] == bytes.fromhex(live["dpi_register"]["response"])[6:10]
    for key, address, length in [("configuration", 0, 6), ("dpi_register", target.value, 4)]:
        assert library.get_read_payload(address, length, request) == 0
        assert bytes(request) == bytes.fromhex(live[key]["request"])
    library.get_profile_payload(request)
    assert bytes(request) == bytes.fromhex(live["profile"]["request"])
    battery_report = bytes.fromhex(live["battery"]["response"])
    for key in ["battery", "battery_wired"]:
        assert decode_battery(bytes.fromhex(live[key]["response"])) == (
            live[key]["percentage"], live[key]["charging"])
        assert decode_battery(bytes.fromhex(live[key]["request"])) is None
    for index, value in [(0, 9), (1, 3), (2, 1), (3, 1), (4, 1), (5, 1), (5, 11), (6, 101), (7, 2)]:
        invalid = bytearray(battery_report)
        invalid[index] = value
        invalid[16] = (0x55 - sum(invalid[:16])) & 255
        assert decode_battery(invalid) is None
    invalid = bytearray(battery_report)
    invalid[16] ^= 1
    assert decode_battery(invalid) is None
    for percentage in [0, 100]:
        for charging in [False, True]:
            report = bytearray(battery_report)
            report[6], report[7] = percentage, int(charging)
            report[16] = (0x55 - sum(report[:16])) & 255
            assert decode_battery(report) == (percentage, charging)
    assert library.decode_battery_report(None, ctypes.byref(BatteryInfo())) == -1
    assert library.decode_battery_report((ctypes.c_ubyte * 17)(*battery_report), None) == -1

    usb_test = directory / "usb-test"
    run(cc + flags + ["tests/usb_mock.c", "src/driver.c", "src/data.c", "-o", str(usb_test)], cwd=ROOT)
    run([str(usb_test)])
    cli = directory / "cli-test"
    run(cc + flags + ["-DTEST_USB_LIBRARY", "main.c", "src/driver.c", "src/data.c",
                     "src/config.c", "src/help.c", "tests/usb_mock.c",
                     "includes/tomlc17-R260618/src/tomlc17.c", "-o", str(cli)], cwd=ROOT)
    environment = dict(os.environ, HOME=str(directory))

    def check_cli(args, success, input_text=None, ack_without_apply=False, usb_environment=None):
        invocation_environment = dict(environment)
        invocation_environment.update(usb_environment or {})
        if ack_without_apply:
            invocation_environment["K1NG_TEST_ACK_WITHOUT_APPLY"] = "1"
        result = subprocess.run([str(cli)] + args, input=input_text, text=True,
                                capture_output=True, env=invocation_environment)
        assert (result.returncode == 0) == success, (args, result.stdout, result.stderr)
        if not success and not ack_without_apply:
            assert "mouse found" not in result.stdout
        if "ACK=yes" in result.stdout:
            assert "DPI changed" not in result.stdout
        if success and "mouse found" in result.stdout and not any(
                option in args for option in ["--battery", "-b", "-bat"]):
            assert "\nreadback=" in result.stdout
        return result

    for text in ["0", "49", "51", "125", "26001", "26050", "50junk", "", "4294967346",
                 "999999999999999999999999999999999999"]:
        check_cli([text], False)
    for dpi in [50, 800, 12800, 12850, 25600, 25650, 26000]:
        result = check_cli([str(dpi)], True)
        assert ("profile=1 index=0\nstage=2/6 index=1\ntarget=0x10\n"
                f"new=800\nACK=yes\nreadback={dpi}\n") in result.stdout
        result = check_cli(["--dpi-address", "0x10", str(dpi)], True)
        assert "target=0x10 debug=yes" in result.stdout
    result = check_cli(["--dpi-address", "0x0c", "400"], True)
    assert "debug write targeted an inactive stage" in result.stdout
    result = check_cli(["400"], True, usb_environment={"K1NG_TEST_ONLINE_ZERO": "1"})
    assert ("profile=1 index=0\nstage=2/6 index=1\ntarget=0x10\n"
            "new=800\nACK=yes\nreadback=400\n") in result.stdout
    assert "mouse is not reporting online" not in result.stderr
    result = check_cli(["400"], True, usb_environment={"K1NG_TEST_EARLY_REPLY": "1"})
    assert "ACK=yes\nreadback=400\n" in result.stdout
    result = check_cli(["600"], True, usb_environment={"K1NG_TEST_WAKE_READS": "1"})
    assert "ACK=yes\nreadback=600\n" in result.stdout
    assert result.stderr == "waiting for mouse response; retrying read automatically\n"
    result = check_cli(["400"], False, ack_without_apply=True)
    assert "ACK=yes" in result.stdout
    assert "readback=" not in result.stdout
    assert "DPI register readback mismatch" in result.stderr
    for args in [["--dpi-address"], ["--dpi-address", "0xff", "800"], ["800", "junk"], ["-p"]]:
        check_cli(args, False)
    check_cli([], True, "50\n")
    check_cli([], False, "125\n")
    check_cli(["-s"], True, "valid\n800\ny\n")
    check_cli(["-p", "valid"], True)
    with (directory / "mouse_presets.toml").open("a") as file:
        file.write("[overflow]\ndpi = 4294967346\n[nonstep]\ndpi = 125\n")
    for preset in ["overflow", "nonstep", "missing"]:
        check_cli(["-p", preset], False)
    check_cli(["-s"], False, "invalid\n50junk\n")
    for option in ["--battery", "-b", "-bat"]:
        result = check_cli([option], True)
        assert "battery: ⣿⣿⣿⣿⣿⣿⣤⠀⠀⠀  65% (not charging; wired)" in result.stdout
        assert "requested DPI" not in result.stdout and "target=" not in result.stdout
        for usb_environment in [{"K1NG_TEST_WIRELESS": "1"}, {"K1NG_TEST_NO_DEVICE": "1"},
                                {"K1NG_TEST_WIRELESS": "1", "K1NG_TEST_CHARGING": "1"}]:
            result = check_cli([option], False, usb_environment=usb_environment)
            assert result.stderr == "driver: u have to be plugged to use this feature\n"
            assert result.stdout == ""
        result = check_cli([option], True, usb_environment={"K1NG_TEST_CHARGING": "1"})
        assert "battery: ⣿⣿⣿⣿⣿⣿⣿⣿⣿⡀  90% (charging; wired; device estimate)" in result.stdout
        assert "battery: 90%" not in result.stdout
        for args in [[option, "800"], ["--dpi-address", "0x10", option]]:
            check_cli(args, False)
    print("Code: 4160 reports, resolver bounds, 64 profile/stage/transport cases, invalid inputs and readback/error paths passed")
    print("Live K1NG read fixtures pass; ACK-without-apply fails readback. Other stage/profile cases use simulated replies.")
    print("Online flag 0: responsive state proceeds; unreachable state or malformed online response prevents a DPI write (simulated replies).")
    print("USB ordering: interrupt IN is submitted before every report; early replies, cancellation, allocation/submit/event errors passed (simulated transport).")
    print("Read recovery: dropped online/configuration/profile reads recover within three attempts; exhausted reads stop before writes, and setting writes are never retried (simulated transport).")
    print("Battery: live replies, wired-only CLI, unplugged message, 0/100% boundaries, charging states, read-only queries and malformed-response handling passed")
    return encode, decode_battery


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--captures", action="store_true")
    args = parser.parse_args()
    with tempfile.TemporaryDirectory(prefix="k1ng-tests-") as directory:
        encode, decode_battery = code_tests(Path(directory))
        if args.captures:
            capture_tests(encode, decode_battery)


if __name__ == "__main__":
    main()
