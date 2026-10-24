"""Reject missing releases, missing waits and late output before trusting real-input results."""

import configparser
import unittest

from features import CASES, check_effects, configuration
from run_acceptance import fixture


def events(keys):
    return [{"key": key, "down": down, "qpc_ns": index * 100_000_000} for index, (key, down) in enumerate(keys)]


class FeatureTests(unittest.TestCase):
    def test_fixture_is_per_process_and_stop_output_is_targeted(self):
        for case in CASES:
            text, rows = configuration(fixture(), case, "QkmEngine61Receiver_123")
            ini = configparser.ConfigParser(interpolation=None)
            ini.read_string(text)
            section = ini["Engine61"]
            self.assertEqual(section["ProcessInfo_ClassName"], "QkmEngine61Receiver_123")
            self.assertEqual(section["ProcessInfo_ClassNameMatchIndex"], "1")
            self.assertEqual(section["KeyMapData_OriginalKeys"].split("|")[0].split("#"), [r["original"] for r in rows])
            methods = section["KeyMapData_SendMappingKeyMethod"].split("|")[0].split("#")
            self.assertEqual(len(methods), len(rows))
            self.assertEqual(methods[-1], "1" if case == "stop_message" else "0")

    def test_return_and_restart_cannot_keep_the_previous_output(self):
        for case, keys in (("tab_return", [(0x43, True), (0x43, False), (0x42, True), (0x42, False)]),
                           ("profile_return", [(0x43, True), (0x43, False)]),
                           ("quick_restart", [(0x42, True), (0x42, False)] * 4)):
            check_effects(case, events(keys))
            for bad in (keys[:-1], [(0x42, down) for _, down in keys] if case != "quick_restart" else keys + keys[:2]):
                with self.assertRaises(AssertionError):
                    check_effects(case, events(bad))

    def test_held_output_requires_exact_release(self):
        good = events([(0x42, True), (0x42, False)])
        self.assertEqual(check_effects("hold_stop", good), [(0x42, True), (0x42, False)])
        for bad in (good[:1], good + good[1:], good[::-1]):
            with self.assertRaises(AssertionError):
                check_effects("hold_stop", bad)

    def test_cancel_rejects_late_tail_and_wrong_source(self):
        good = events([(0x43, True), (0x43, False)])
        for case in ("sequence_cancel", "exit_wait"):
            check_effects(case, good)
            for bad in (good + events([(0x45, True), (0x45, False)]), good + events([(0x41, True)]), good + good[1:]):
                with self.assertRaises(AssertionError):
                    check_effects(case, bad)

    def test_sequence_and_stop_wait_cannot_be_skipped(self):
        sequence = events([(0x43, True), (0x43, False), (0x44, True), (0x44, False)])
        check_effects("sequence_complete", sequence)
        sequence[2]["qpc_ns"] = 1
        with self.assertRaises(AssertionError):
            check_effects("sequence_complete", sequence)
        stop = events([(0x45, True), (0x45, False)])
        stop[1]["qpc_ns"] = 250_000_000
        check_effects("stop_message", stop)
        for bad in (stop * 2, [dict(stop[0]), dict(stop[1], qpc_ns=1)]):
            with self.assertRaises(AssertionError):
                check_effects("stop_message", bad)

    def test_burst_must_apply_its_configured_times(self):
        burst = events([(0x43, True), (0x43, False)] * 3)
        for event, milliseconds in zip(burst, (0, 400, 500, 900, 1000, 1020)):
            event["qpc_ns"] = milliseconds * 1_000_000
        check_effects("burst_stop", burst)
        burst[1]["qpc_ns"] = 40_000_000
        with self.assertRaises(AssertionError):
            check_effects("burst_stop", burst)


if __name__ == "__main__":
    unittest.main()
