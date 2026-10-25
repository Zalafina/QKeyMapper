"""Bounded public action tests; no UAC, product, input or driver use."""

from dataclasses import asdict, replace
from pathlib import Path
from unittest.mock import patch
import unittest
import uuid

import cache_control as app
from cache_actions import digest, file_hash, helper_hashes, action_command, TICKS_TO_FILETIME
from cache_core import CacheError
from test_cache_control import ControllerFixture, FakeProcess


class ActionTests(ControllerFixture):
    def setUp(self):
        super().setUp()
        for name in ('scripts/test_process.ps1', 'QKeyMapper/tests/mcp_control/product_ui_actions.ps1'):
            path = self.root / name
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_text('fixed helper', encoding='utf-8')
        self.exe = self.root / 'out/test/Fixture.exe'
        self.exe.parent.mkdir(parents=True)
        self.exe.write_bytes(b'MZ fixed native fixture')
        self.backend.live = {}
        self.backend.verify_actor = lambda actor: app.require(actor == asdict(self.backend.owner), 'wrong actor', 'identity_rejected')
        self.backend.owned_alive = lambda value: self.backend.live.get(value['pid'], False)
        self.backend.started_identity = self.started_identity
        self.backend.start_action = self.start_action
        self.actor = asdict(self.backend.owner)
        self.set_window(300)
        self.finish_on()
        self.target = {'id': 'maintenance-one', 'executable': str(self.exe), 'sha256': file_hash(self.exe),
            'cases': [{'arguments': '--case alpha', 'environment': {}}], 'desktop': 'private',
            'output_root': str(self.root / 'out/test'), 'maximum_seconds': 90,
            'gui_actions': ['ReadMapping'], 'client_executable': None, 'helpers': helper_hashes(self.root)}
        self.register(self.target)

    def register(self, target):
        self.controller.scope['test_actions'] = {'reason': 'explicit test scope', 'definitions': [digest(target)]}
        request = self.request('register', definition=target, actor=self.actor)
        self.assertTrue(self.result(request)['ok'], self.result(request))

    def start_action(self, payload, path, log, pins):
        self.backend.spawned.append((payload, pins))
        return FakeProcess()

    def started_identity(self, response):
        return asdict(replace(self.backend.owner, pid=response['ProcessId'], created=int(response['StartTicks']) - TICKS_TO_FILETIME,
            executable=str(self.exe)))

    def start_request(self, **change):
        return {'Action': 'Start', 'Executable': str(self.exe), 'Arguments': '--case alpha',
            'Environment': {}, 'DesktopName': 'QkmMaintenance_123', **change}

    def action(self, request, **extra):
        return self.request('test_action', request=request, actor=self.actor, **extra)

    def finish_action(self, response=None, code=0):
        p = self.controller.actions.pending
        response = response or {'Success': True, 'ProcessId': 601, 'StartTicks': str(TICKS_TO_FILETIME + 2000)}
        app.write_json(p['response'], response)
        p['process'].code = code
        self.backend.live[601] = True
        self.controller.actions.advance()

    def start_owned(self):
        request = self.action(self.start_request())
        self.finish_action()
        self.assertTrue(self.result(request)['ok'])
        return request

    def close_request(self, **change):
        return {'Action': 'Close', 'Executable': str(self.exe), 'ProcessId': 601,
            'StartTicks': str(TICKS_TO_FILETIME + 2000), 'DesktopName': 'QkmMaintenance_123', **change}

    def test_new_case_and_target_registration_reuses_owner_and_window(self):
        original = self.controller.window.deadline
        self.target['cases'].append({'arguments': '--case beta', 'environment': {}})
        self.register(self.target)
        request = self.action(self.start_request(Arguments='--case beta'))
        self.finish_action()
        self.assertTrue(self.result(request)['ok'])
        self.assertEqual(original, self.controller.window.deadline)
        self.assertEqual(len(self.backend.opens), 1)
        self.exe = self.exe.with_name('SecondFixture.exe')
        self.exe.write_bytes(b'MZ independent native target')
        other = dict(self.target, id='maintenance-two', executable=str(self.exe), sha256=file_hash(self.exe),
            cases=[{'arguments': '--case gamma', 'environment': {}}])
        self.register(other)
        request = self.action(self.start_request(Arguments='--case gamma'))
        self.assertIsNotNone(self.controller.actions.pending)
        self.finish_action()
        self.assertTrue(self.result(request)['ok'])

    def test_metadata_does_not_grant_authority(self):
        self.controller.scope = {}
        result = self.request('register', definition=self.target, actor=self.actor)
        self.assertEqual(self.result(result)['error'], 'scope_not_authorized')
        result = self.action(self.start_request())
        self.assertEqual(self.result(result)['error'], 'target_not_registered')
        self.assertEqual(self.backend.spawned, [])

    def test_invalid_target_arguments_environment_and_desktop_rejected(self):
        for values in ({'Executable': str(self.root / 'elsewhere.exe')}, {'Arguments': '--shell arbitrary'},
                {'Environment': {'PATH': 'unapproved'}}, {'DesktopName': 'Default'}, {'Extra': 'value'}):
            with self.subTest(values=values):
                request = self.action(self.start_request(**values))
                self.assertFalse(self.result(request)['ok'])
        self.assertEqual(self.backend.spawned, [])

    def test_changed_artifact_and_helper_rejected(self):
        for path in (self.exe, self.root / 'scripts/test_process.ps1'):
            old = path.read_bytes()
            path.write_bytes(old + b' changed')
            request = self.action(self.start_request())
            self.assertEqual(self.result(request)['error'], 'artifact_changed' if path == self.exe else 'helper_changed')
            path.write_bytes(old)
        self.assertEqual(self.backend.spawned, [])

    def test_helper_update_can_be_reviewed_without_controller_restart(self):
        (self.root / 'scripts/test_process.ps1').write_text('reviewed fixed helper update', encoding='utf-8')
        self.target['helpers'] = helper_hashes(self.root)
        self.register(self.target)
        request = self.action(self.start_request())
        self.finish_action()
        self.assertTrue(self.result(request)['ok'])
        self.assertEqual(len(self.backend.opens), 1)

    def test_run_internal_action_does_not_wait_for_its_own_job(self):
        self.controller.job = {'kind': 'product'}
        self.controller.actions.job_token = 'a' * 32
        request = self.action(self.start_request(), job_token='a' * 32)
        self.assertNotIn(request['id'], self.controller.results)
        self.finish_action()
        self.assertTrue(self.result(request)['ok'])
        self.controller.job = None

    def test_run_gui_pairs_actual_process_image_instead_of_venv_launcher(self):
        client = self.root / 'out/test/ActualPython.exe'
        client.write_bytes(b'MZ actual process image')
        self.backend.owner = replace(self.backend.owner, executable=str(client))
        self.controller.actions.register_job('product', {'executable': str(self.exe)})
        self.assertEqual(self.controller.actions.definitions['run_executable']['client_executable'], str(client))

    def test_unrelated_client_rejected_while_run_owns_controller(self):
        self.controller.job = {'kind': 'product'}
        self.controller.actions.job_token = 'a' * 32
        request = self.action(self.start_request(), job_token='b' * 32)
        self.assertEqual(self.result(request)['error'], 'busy')
        self.controller.job = None
        self.assertEqual(self.backend.spawned, [])

    def test_action_is_serial_and_duplicate_does_not_spawn_twice(self):
        request = self.action(self.start_request())
        self.controller.handle(request)
        next_request = self.action(self.start_request())
        self.assertEqual(self.result(next_request)['error'], 'busy')
        self.assertEqual(len(self.backend.spawned), 1)
        self.finish_action()
        self.controller.handle(request)
        self.assertEqual(len(self.backend.spawned), 1)

    def test_cache_missing_or_deadline_does_not_fallback_to_uac(self):
        self.backend.available = False
        request = self.action(self.start_request())
        self.assertEqual(self.result(request)['error'], 'needs_uac')
        self.backend.available = True
        self.tick += 301
        request = self.action(self.start_request())
        self.assertFalse(self.result(request)['ok'])
        self.assertEqual(self.backend.spawned, [])
        self.assertEqual(len(self.backend.opens), 1)

    def test_shortened_window_schedules_cleanup_before_its_new_deadline(self):
        self.start_owned()
        request = self.set_window(25)
        self.assertTrue(self.result(request)['ok'])
        self.tick += 6
        self.controller.advance()
        self.assertEqual(self.controller.actions.pending['request']['Action'], 'Close')
        self.finish_action({'Success': True, 'ExitCode': 0})

    def test_too_short_window_does_not_strand_owned_process(self):
        self.start_owned()
        old = self.controller.window.deadline
        request = self.set_window(5)
        self.assertEqual(self.result(request)['error'], 'insufficient_window')
        self.assertEqual(old, self.controller.window.deadline)

    def test_pid_reuse_or_changed_creation_refuses_close(self):
        self.start_owned()
        request = self.action(self.close_request(StartTicks=str(TICKS_TO_FILETIME + 2001)))
        self.assertEqual(self.result(request)['error'], 'ownership_rejected')
        self.backend.live[601] = False
        request = self.action(self.close_request())
        self.assertEqual(self.result(request)['error'], 'ownership_rejected')
        self.assertEqual(len(self.backend.spawned), 1)

    def test_unknown_liveness_is_not_recorded_as_clean_exit(self):
        self.start_owned()
        self.backend.live[601] = None
        self.controller.actions.refresh_owned()
        self.assertIn(601, self.controller.actions.owned)

    def test_only_owned_gui_and_authorized_action_can_run(self):
        request = self.action(dict(self.close_request(), Action='Gui', Name='ReadMapping',
            ResultFile=str(self.root / 'out/test/read.json')))
        self.assertEqual(self.result(request)['error'], 'ownership_rejected')
        self.start_owned()
        request = self.action(dict(self.close_request(), Action='Gui', Name='StartMapping',
            ResultFile=str(self.root / 'out/test/read.json')))
        self.assertEqual(self.result(request)['error'], 'scope_not_authorized')
        self.assertEqual(len(self.backend.spawned), 1)

    def test_off_cleans_owned_before_cache_close(self):
        self.start_owned()
        self.request('off')
        self.assertTrue(self.controller.advance())
        self.assertEqual(self.backend.closes, 0)
        self.assertEqual(self.controller.actions.pending['request']['Action'], 'Close')
        self.finish_action({'Success': True, 'ExitCode': 0})
        self.assertFalse(self.controller.advance())
        self.assertFalse(self.controller.actions.owned)
        self.assertFalse(self.backend.available)

    def test_timeout_retains_unknown_result_without_replay(self):
        request = self.action(self.start_request())
        self.tick += 71
        self.controller.actions.advance()
        self.assertEqual(self.result(request)['error'], 'result_unknown')
        self.assertIn(request['id'], self.controller.actions.unknown_starts)
        self.controller.handle(request)
        self.assertEqual(len(self.backend.spawned), 1)
        self.finish_action()
        self.assertNotIn(request['id'], self.controller.actions.unknown_starts)

    def test_fixed_command_does_not_accept_shell_or_extra_executable(self):
        command = action_command(self.root, 'pwsh.exe', self.start_request(), Path('fixed.json'))
        self.assertEqual(command, ['pwsh.exe', '-NoProfile', '-File', str(self.root / 'scripts/test_process.ps1'),
            '-LaunchRequest', 'fixed.json'])

    def test_old_controller_cannot_be_silently_replaced_by_action_client(self):
        with patch.object(app, 'token_info', return_value={'elevated': False, 'integrity': 8192}), \
                patch.object(app, 'verified_controller', return_value={'format_version': 1}), \
                patch.object(app, 'send') as send, patch.object(app, 'ensure_controller') as create:
            with self.assertRaises(CacheError):
                app.client(['test-action'], action_file=Path('request.json'))
            send.assert_not_called()
            create.assert_not_called()

    def test_upgrade_keeps_previous_cutoff_and_refuses_active_job(self):
        old = {'ok': True, 'status': 'active', 'identity': {'pid': 42}, 'instance': 'a' * 32,
            'deadline': '2026-10-25T21:07:50+08:00', 'remaining_seconds': 300, 'scope': {}}
        new = {'identity': {'pid': 43}, 'instance': 'b' * 32}
        with patch.object(app, 'verified_controller', side_effect=[old, None]), \
                patch.object(app, 'send', side_effect=[old, {}, {}, {'ok': True}]) as send, \
                patch.object(app, 'ensure_controller', return_value=(new, True)), \
                patch.object(app, 'read_json', return_value={'instance': old['instance'], 'cache_available': False}):
            result = app.upgrade_controller()
        applied = send.call_args_list[-1].args[1]
        self.assertEqual(applied['deadline'], old['deadline'])
        self.assertLessEqual(applied['remaining_cap'], 300)
        self.assertEqual(result['previous_controller_pid'], 42)
        with patch.object(app, 'verified_controller', return_value=old), \
                patch.object(app, 'send', return_value=dict(old, current_job='source')), \
                patch.object(app, 'ensure_controller') as create:
            with self.assertRaises(CacheError):
                app.upgrade_controller()
            create.assert_not_called()


if __name__ == '__main__':
    unittest.main()
