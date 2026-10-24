"""Fixed original-function expectations; all keyboard output is observed at the receiver."""

import asyncio
import json
import time

import win32api
import win32gui


CASES = ("hold_stop", "burst_release", "burst_stop", "lock_toggle_stop", "sequence_complete",
         "sequence_cancel", "stop_message", "tab_return", "profile_return", "quick_restart", "exit_wait")


def configuration(base, case, window_class):
    assert case in CASES and window_class.startswith("QkmEngine61Receiver_")
    row = {"hold_stop": None, "burst_release": ("F13", "C"), "burst_stop": ("F13", "C"),
           "lock_toggle_stop": ("F14", "D"), "sequence_complete": ("F15", "C⏱200»D"),
           "sequence_cancel": ("F16", "C⏱2000»E"), "stop_message": ("SendOnMappingStop", "E⏱250"),
           "tab_return": None, "profile_return": None, "quick_restart": None,
           "exit_wait": ("F16", "C⏱2000»E")}[case]
    rows = [{"original": "A", "mapping": "B"}]
    if row:
        rows.append({"original": row[0], "mapping": row[1]})
        base = base.replace("KeyMapData_OriginalKeys=A\n", "KeyMapData_OriginalKeys=A#" + row[0] + "\n", 1)
        base = base.replace("KeyMapData_MappingKeys=B\n", "KeyMapData_MappingKeys=B#" + row[1] + "\n", 1)
        method = "1" if case == "stop_message" else "0"
        base = base.replace("KeyMapData_SendMappingKeyMethod=0\n", "KeyMapData_SendMappingKeyMethod=0#" + method + "\n", 1)
        base += "KeyMapData_KeySeqHoldDown=OFF#OFF\nKeyMapData_Unbreakable=OFF#OFF\nKeyMapData_RepeatMode=0#0\n"
    if case.startswith("burst_"):
        base += "KeyMapData_Burst=OFF#ON\nKeyMapData_BurstPressTime=40#400\nKeyMapData_BurstReleaseTime=20#100\n"
    if case == "lock_toggle_stop":
        base += "KeyMapData_Lock=OFF#ON\n"
    # Exact per-process class prevents stop SendMessage from selecting a lookalike window.
    base = base.replace("ProcessInfo_ClassNameMatchIndex=0\n", "ProcessInfo_ClassName=" + window_class +
                        "\nProcessInfo_ClassNameMatchIndex=1\n", 1)
    if case == "tab_return":
        base = base.replace("MappingTable_TabNameList=Tab1\n", "MappingTable_TabNameList=Tab1, Tab2\nMappingTable_TabHotkeyList=F17, F18\n", 1)
        for name, value in (("OriginalKeys", "A|A"), ("MappingKeys", "B|C"), ("SendMappingKeyMethod", "0|0")):
            old = {"OriginalKeys": "A", "MappingKeys": "B", "SendMappingKeyMethod": "0"}[name]
            base = base.replace("KeyMapData_" + name + "=" + old + "\n", "KeyMapData_" + name + "=" + value + "\n", 1)
    if case in ("quick_restart", "profile_return"):
        base = base.replace("MappingStartKey=F23\n", "MappingStartKey=F17\n", 1)
    if case == "profile_return":
        base = base.replace("SettingSelect=Engine61\n", "GlobalSettingSwitchTimeout=100\nSettingSelect=Engine61\n", 1)
        base = base.replace("AutoStartMappingChecked=0\n", "AutoStartMappingChecked=2\n", 1)
        base += ("\n[QKeyMapperGlobalSetting]\nMappingTable_TabNameList=Global\nMappingTable_LastTabName=Global\n"
                 "KeyMapData_OriginalKeys=A\nKeyMapData_MappingKeys=C\nKeyMapData_SendMappingKeyMethod=0\n"
                 "MappingStartKey=F17\nMappingStopKey=F24\nAutoStartMappingChecked=2\n")
    return base, rows


def mapped(events):
    return [(event["key"], event["down"]) for event in events if 0x42 <= event["key"] <= 0x45]


def check_effects(case, events):
    effect = mapped(events)
    if case == "hold_stop":
        assert effect == [(0x42, True), (0x42, False)], "Held B was lost, duplicated or not released"
    elif case.startswith("burst_"):
        assert len(effect) >= 6 and len(effect) % 2 == 0, "Burst did not repeat and finish released"
        assert all(key == 0x43 and down == (index % 2 == 0) for index, (key, down) in enumerate(effect)), \
            "Burst output changed key or lost press/release order"
        output = [event for event in events if event["key"] == 0x43]
        for index in range(0, len(output) - 2, 2):
            duration = (output[index + 1]["qpc_ns"] - output[index]["qpc_ns"]) / 1_000_000
            gap = (output[index + 2]["qpc_ns"] - output[index + 1]["qpc_ns"]) / 1_000_000
            assert 350 <= duration < 1500 and 50 <= gap < 1000, "Configured burst press/release times were not applied"
    elif case == "lock_toggle_stop":
        assert effect == [(0x44, True), (0x44, False)] * 2, "Lock toggle/stop did not release D exactly once"
    elif case == "sequence_complete":
        assert effect == [(0x43, True), (0x43, False), (0x44, True), (0x44, False)], "Wrong sequence order"
        output = [event for event in events if event["key"] in (0x43, 0x44)]
        wait_ms = (output[2]["qpc_ns"] - output[0]["qpc_ns"]) / 1_000_000
        assert 180 <= wait_ms < 1500, "Configured 200 ms wait was skipped or stalled"
    elif case in ("sequence_cancel", "exit_wait"):
        assert effect == [(0x43, True), (0x43, False)], "Cancelled sequence released incorrectly or sent late E"
    elif case == "tab_return":
        assert effect == [(0x43, True), (0x43, False), (0x42, True), (0x42, False)], "Tab switch did not change and restore actual output"
    elif case == "profile_return":
        assert effect == [(0x43, True), (0x43, False)], "Global fallback did not send C"
    elif case == "quick_restart":
        assert effect == [(0x42, True), (0x42, False)] * 4, "Restart lost output or released an old B into the next session"
    else:
        assert case == "stop_message" and effect == [(0x45, True), (0x45, False)], "Stop action missing or duplicated"
        output = [event for event in events if event["key"] == 0x45]
        assert 200 <= (output[1]["qpc_ns"] - output[0]["qpc_ns"]) / 1_000_000 < 1500
    original = {"hold_stop": 0x41, "burst_release": 0x7C, "burst_stop": 0x7C, "lock_toggle_stop": 0x7D,
                "sequence_complete": 0x7E, "sequence_cancel": 0x7F, "stop_message": None,
                "tab_return": 0x41, "profile_return": 0x41, "quick_restart": 0x41, "exit_wait": 0x7F}[case]
    extra = (0x80, 0x81) if case == "tab_return" else (0x80,) if case == "quick_restart" else ()
    assert all(0x42 <= event["key"] <= 0x45 or event["key"] in (original, 0x87) + extra for event in events), \
        "Unexpected source or output key appeared"
    return effect


async def wait_for(receiver, sender, predicate, timeout=3):
    deadline = time.monotonic() + timeout
    while time.monotonic() < deadline:
        sender.check_target()
        if predicate():
            return
        await asyncio.sleep(.01)
    raise AssertionError("Expected real key state or receiver event did not arrive")


async def tap(sender, sent, key):
    sent.append(sender.send(key, True))
    await asyncio.sleep(.02)
    sent.append(sender.send(key, False))


def pressed(key):
    return bool(win32api.GetAsyncKeyState(key) & 0x8000)


async def exercise(case, product, receiver, sender):
    sent, completed = [], False
    receiver.reset()
    stop_during_case = case not in ("burst_release", "sequence_complete", "tab_return")
    try:
        if case == "hold_stop":
            sent.append(sender.send(0x41, True))
            await wait_for(receiver, sender, lambda: receiver.count(0x42) == 1 and pressed(0x42))
            await tap(sender, sent, 0x87)
            await wait_for(receiver, sender, lambda: receiver.count(0x42) >= 2 and not pressed(0x42))
            await asyncio.sleep(.3)
        elif case.startswith("burst_"):
            sent.append(sender.send(0x7C, True))
            # Stop during a 400 ms down phase, after three real burst presses.
            await wait_for(receiver, sender, lambda: receiver.count(0x43) >= 5 and receiver.count(0x43) % 2 == 1)
            if case == "burst_stop":
                await tap(sender, sent, 0x87)
            else:
                sent.append(sender.send(0x7C, False))
            await wait_for(receiver, sender, lambda: receiver.count(0x43) >= 6 and not pressed(0x43))
            await asyncio.sleep(.55)
        elif case == "lock_toggle_stop":
            await tap(sender, sent, 0x7D)
            await wait_for(receiver, sender, lambda: receiver.count(0x44) == 1 and pressed(0x44))
            await asyncio.sleep(.15)
            assert receiver.count(0x44) == 1 and pressed(0x44), "Lock released with its source key"
            await tap(sender, sent, 0x7D)
            await wait_for(receiver, sender, lambda: receiver.count(0x44) == 2 and not pressed(0x44))
            await tap(sender, sent, 0x7D)
            await wait_for(receiver, sender, lambda: receiver.count(0x44) == 3 and pressed(0x44))
            await tap(sender, sent, 0x87)
            await wait_for(receiver, sender, lambda: receiver.count(0x44) >= 4 and not pressed(0x44))
            await asyncio.sleep(.3)
        elif case in ("sequence_complete", "sequence_cancel", "exit_wait"):
            key = 0x7E if case == "sequence_complete" else 0x7F
            sent.append(sender.send(key, True))
            await wait_for(receiver, sender, lambda: receiver.count(0x43) >= 1 and pressed(0x43))
            if case == "sequence_complete":
                await wait_for(receiver, sender, lambda: receiver.count(0x44) >= 2)
                sent.append(sender.send(key, False))
            else:
                if case == "exit_wait":
                    await product.close()
                else:
                    await tap(sender, sent, 0x87)
                await wait_for(receiver, sender, lambda: not pressed(0x43))
                sent.append(sender.send(key, False))
                # Watch beyond the cancelled 2 s wait, not just until GUI says idle.
                await asyncio.sleep(2.25)
        elif case == "tab_return":
            for hotkey, output in ((0x81, 0x43), (0x80, 0x42)):
                await tap(sender, sent, hotkey)
                await asyncio.sleep(.2)
                await tap(sender, sent, 0x41)
                await wait_for(receiver, sender, lambda: receiver.count(output) == 2)
            await asyncio.sleep(.3)
        elif case == "profile_return":
            title = win32gui.GetWindowText(receiver.window)
            try:
                win32gui.SetWindowText(receiver.window, title + " [global fallback]")
                await asyncio.sleep(1.5)
                await tap(sender, sent, 0x41)
                await wait_for(receiver, sender, lambda: receiver.count(0x43) == 2)
                await tap(sender, sent, 0x87)
                await asyncio.sleep(.3)
            finally:
                win32gui.SetWindowText(receiver.window, title)
        elif case == "quick_restart":
            for cycle in range(4):
                if cycle:
                    await tap(sender, sent, 0x80)
                    await asyncio.sleep(.15)
                sent.append(sender.send(0x41, True))
                await wait_for(receiver, sender, lambda: receiver.count(0x42) == cycle * 2 + 1 and pressed(0x42))
                await tap(sender, sent, 0x87)
                await wait_for(receiver, sender, lambda: receiver.count(0x42) == cycle * 2 + 2 and not pressed(0x42))
                sent.append(sender.send(0x41, False))
                await asyncio.sleep(.1)
        else:
            assert case == "stop_message"
            await tap(sender, sent, 0x87)
            await wait_for(receiver, sender, lambda: receiver.count(0x45) >= 1)
            assert not pressed(0x45), "SendMessage stop action altered global E key state"
            await asyncio.sleep(.5)
            await tap(sender, sent, 0x87)
            await asyncio.sleep(.3)
        completed = True
    finally:
        cleanup_source_keys = sorted(sender.held)
        sender.release()
        await asyncio.sleep(.1)
        events = await receiver.flush()
        (product.output / "feature-events.json").write_text(json.dumps({"case": case, "actions_completed": completed,
            "sent": sent, "released_in_cleanup": cleanup_source_keys, "received": events}, ensure_ascii=False, indent=2), encoding="utf-8")
    effect = check_effects(case, events)
    assert not any(pressed(key) for key in range(0x41, 0x46)), "A mapped or restored key remained held"
    if not stop_during_case:
        receiver.reset()
        await tap(sender, sent, 0x87)
        await asyncio.sleep(.5)
        tail = await receiver.flush()
        (product.output / "stop-tail.json").write_text(json.dumps(tail, ensure_ascii=False, indent=2), encoding="utf-8")
        assert not any(down for _, down in mapped(tail)), "New mapped key-down after the case completed"
    return {"case": case, "mapped_effect": effect, "original_events": [e for e in events if not 0x42 <= e["key"] <= 0x45],
            "stop_during_case": stop_during_case}
