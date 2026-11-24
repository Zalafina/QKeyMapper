"""Bounded public action tests; no UAC, product, input or driver use."""

from dataclasses import asdict, replace
import json
from pathlib import Path
from unittest.mock import patch
import unittest
import uuid

import cache_control as app
from cache_actions import REGISTRATION_LIMIT, digest, file_hash, helper_hashes, action_command, TICKS_TO_FILETIME
from cache_core import CacheError, validate_scope
from cache_history import FILE_BYTES
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
        return self.result(request)['registration_reference']

    def authorize(self, fingerprints=()):
        scope = {'test_actions': {'reason': 'explicit test scope', 'definitions': list(fingerprints)}} if fingerprints else {}
        result = self.result(self.request('authorize', scope=scope))
        self.assertTrue(result['ok'], result)

    def release_request(self, operation, target, error=None):
        result = self.result(self.request(operation, target=target, actor=self.actor))
        if error:
            self.assertFalse(result['ok'])
            self.assertEqual(result['error'], error)
        else:
            self.assertTrue(result['ok'], result)
        return result

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

    def action_rows(self, request):
        return [json.loads(line) for path in (self.controller.runtime / 'history').glob('events-*.jsonl')
            for line in path.read_text(encoding='utf-8').splitlines()
            if json.loads(line).get('request_id') == request['id']]

    def test_native_failure_keeps_obtained_diagnostics_without_private_response(self):
        request = self.action(self.start_request())
        self.controller.actions.pending['process'].pid = 701
        self.tick += 2.5
        self.finish_action({'Success': False, 'ProcessId': 602, 'Error': 'PRIVATE response'}, code=42)
        result = self.result(request)
        self.assertEqual(result['error'], 'test_action_failed')
        for key, value in {'native_exit_code': 42, 'native_pid': 701, 'reported_target_pid': 602,
                'action_seconds': 2.5, 'timed_out': False, 'controller_terminated': False,
                'failure_stage': 'action_result', 'response_state': 'valid', 'os_error_code': None, 'os_errno': None}.items():
            self.assertEqual(result[key], value, key)
        row = self.action_rows(request)[-1]
        self.assertEqual(row['native_exit_code'], 42)
        self.assertEqual(row['instance'], self.controller.instance)
        self.assertEqual(row['generation'], self.controller.window.generation)
        self.assertNotIn('PRIVATE', json.dumps(row))
        self.assertIn(request['id'], self.controller.actions.unknown_starts)
        self.assertEqual(len(self.backend.spawned), 1)

    def test_missing_response_keeps_native_exit_and_original_error(self):
        request = self.action(self.start_request())
        pending = self.controller.actions.pending
        pending['process'].code = 1
        self.controller.actions.advance()
        result = self.result(request)
        self.assertEqual(result['error'], 'invalid_record')
        self.assertEqual(result['native_exit_code'], 1)
        self.assertEqual(result['response_state'], 'missing')
        self.assertEqual(result['failure_stage'], 'response_read')
        self.assertIsNone(result['native_pid'])
        self.assertIsNone(result['os_error_code'])
        self.assertIn(request['id'], self.controller.actions.unknown_starts)
        self.assertFalse(pending['process'].terminated)

    def test_damaged_response_is_distinct_from_missing_and_valid(self):
        request = self.action(self.start_request())
        pending = self.controller.actions.pending
        pending['response'].write_text('{PRIVATE broken', encoding='utf-8')
        pending['process'].code = 7
        self.controller.actions.advance()
        result = self.result(request)
        self.assertEqual(result['error'], 'internal_error')
        self.assertEqual(result['native_exit_code'], 7)
        self.assertEqual(result['response_state'], 'invalid')
        self.assertEqual(result['failure_stage'], 'response_read')
        self.assertNotIn('PRIVATE', json.dumps(self.action_rows(request)))

    def test_obtained_os_error_codes_are_not_inferred_from_error_text(self):
        request = self.action(self.start_request())
        self.controller.actions.pending['process'].code = 3
        error = OSError(13, 'PRIVATE claims error 999')
        error.winerror = 5
        with patch.object(app, 'read_json', side_effect=error):
            self.controller.actions.advance()
        result = self.result(request)
        self.assertEqual(result['os_error_code'], 5)
        self.assertEqual(result['os_errno'], 13)
        self.assertEqual(result['error'], 'internal_error')
        self.assertEqual(result['response_state'], 'unavailable')
        rows = self.action_rows(request)
        record = next(row for row in rows if row['event'] == 'request_result')
        self.assertEqual((record['os_error_code'], record['os_errno']), (5, 13))
        self.assertNotIn('PRIVATE claims error 999', json.dumps(rows))

    def test_timeout_reports_missing_exit_without_terminating_or_replaying(self):
        request = self.action(self.start_request())
        pending = self.controller.actions.pending
        self.tick += 71
        self.controller.actions.advance()
        result = self.result(request)
        self.assertEqual(result['error'], 'result_unknown')
        self.assertIsNone(result['native_exit_code'])
        self.assertTrue(result['timed_out'])
        self.assertFalse(result['controller_terminated'])
        self.assertEqual(result['failure_stage'], 'await_exit')
        self.assertEqual(result['response_state'], 'not_observed')
        self.assertIs(self.controller.actions.pending, pending)
        self.assertFalse(pending['process'].terminated)
        self.controller.handle(request)
        self.assertEqual(len(self.backend.spawned), 1)
        self.finish_action()
        self.assertTrue(self.result(request)['ok'])
        self.assertTrue(self.result(request)['timed_out'])

    def test_diagnostic_write_failure_does_not_prevent_unknown_start_cleanup(self):
        request = self.action(self.start_request())
        self.controller.actions.pending['process'].code = 1
        with patch.object(self.controller.history, '_append', side_effect=OSError('PRIVATE disk full')):
            self.controller.actions.advance()
            self.controller.serve()
        result = self.result(request)
        self.assertEqual(result['error'], 'invalid_record')
        self.assertEqual(result['native_exit_code'], 1)
        self.assertFalse(self.backend.available)
        self.assertIn(request['id'], self.controller.actions.unknown_starts)
        closed = app.read_json(self.controller.runtime / 'closed.json')
        self.assertFalse(closed['history']['available'])
        self.assertGreater(closed['history']['missed_events'], 0)

    def test_dispatch_error_diagnostics_do_not_create_owned_or_pending_action(self):
        with patch.object(self.backend, 'start_action', side_effect=OSError(13, 'PRIVATE error 123')):
            request = self.action(self.start_request())
        self.assertEqual(self.result(request)['error'], 'internal_error')
        row = next(row for row in self.action_rows(request) if row['event'] == 'test_action_diagnostic')
        self.assertEqual(row['failure_stage'], 'dispatch')
        self.assertEqual(row['os_errno'], 13)
        self.assertIsNone(row['os_error_code'])
        self.assertIsNone(row['native_exit_code'])
        self.assertIsNone(row['native_pid'])
        self.assertFalse(self.controller.actions.owned)
        self.assertIsNone(self.controller.actions.pending)

    def test_full_registry_requires_explicit_unregistration_and_keeps_records_bounded(self):
        actions = self.controller.actions
        first = actions.registration_reference(self.target['id'])
        references = [first]
        for index in range(1, 128):
            name = f'target-{index:03d}' + 'x' * 54
            references.append(self.register(dict(self.target, id=name)))
        self.assertEqual(len(actions.definitions), 128)
        extra = dict(self.target, id='next-target')
        self.authorize([digest(extra)])
        denied = self.result(self.request('register', definition=extra, actor=self.actor))
        self.assertEqual(denied['error'], 'request_limit')
        self.assertIn('128', denied['message'])
        self.assertIn('没有可释放登记', denied['message'])
        self.assertEqual(actions.releasable_targets(), [])

        marks = [self.release_request('mark_releasable', reference)['release_reference'] for reference in references]
        unrelated = [f'{index:064x}' for index in range(32)]
        self.authorize(unrelated)
        status_request = self.request('status')
        status = self.result(status_request)
        self.assertEqual((status['registered_target_count'], status['registered_target_limit']), (128, REGISTRATION_LIMIT))
        self.assertEqual(status['registered_targets'], status['releasable_targets'])
        self.assertIs(status['release_reference_supported'], True)
        self.assertEqual(len(status['scope']['test_actions']['definitions']), 32)
        self.assertLessEqual(len(self.controller.results), 64)
        for file in (self.controller.runtime / 'state.json',
                     self.controller.runtime / 'results' / (status_request['id'] + '.json')):
            self.assertLessEqual(file.stat().st_size, 65536)
            self.assertEqual(app.read_json(file)['registered_target_count'], 128)

        self.authorize([digest(extra)])
        denied = self.result(self.request('register', definition=extra, actor=self.actor))
        self.assertEqual(denied['error'], 'request_limit')
        self.assertIn('显式注销', denied['message'])
        self.assertEqual(len(actions.definitions), 128)
        test_artifact = self.root / 'out/test/existing-result.json'
        test_artifact.write_text('{"preserved": true}', encoding='utf-8')
        history = {path: path.read_bytes() for path in (self.controller.runtime / 'history').glob('events-*.jsonl')}
        self.assertTrue(history)
        self.release_request('unregister', marks[0])
        self.assertEqual(len(actions.definitions), 127)
        self.assertNotIn(first['target_id'], actions.definitions)
        self.assertEqual(self.exe.read_bytes(), b'MZ fixed native fixture')
        self.assertEqual(test_artifact.read_text(encoding='utf-8'), '{"preserved": true}')
        for path, contents in history.items():
            current = path.read_bytes()
            self.assertTrue(current.startswith(contents))
            self.assertLessEqual(len(current), FILE_BYTES)
            self.assertNotIn(marks[0]['release_token'].encode(), current)
        rows = [json.loads(line) for path in history for line in path.read_text(encoding='utf-8').splitlines()]
        for event in ('target_release_marked', 'target_unregistered'):
            self.assertTrue(any(row['event'] == event and row.get('target_id') == first['target_id']
                and row.get('definition_hash') == first['definition_hash'] and len(row.get('request_id', '')) == 32 for row in rows))
        self.register(extra)
        self.assertEqual(len(actions.definitions), 128)
        self.assertIn(extra['id'], actions.definitions)
        updated = self.register(dict(extra, maximum_seconds=91))
        self.assertNotEqual(updated['definition_hash'], digest(extra))
        self.assertEqual(len(actions.definitions), 128)

    def test_release_reference_survives_original_result_eviction(self):
        actions = self.controller.actions
        target_id = self.target['id']
        self.authorize()
        mark = self.release_request('mark_releasable', actions.registration_reference(target_id))
        runtime = self.controller.runtime
        old_result = runtime / 'results' / (mark['request_id'] + '.json')
        self.assertTrue(old_result.is_file())
        for _ in range(65):
            status = self.result(self.request('status'))
        self.assertFalse(old_result.exists())
        self.assertEqual(len(self.controller.results), 64)
        self.assertEqual(status['releasable_targets'], [target_id])

        marks = actions.release_marks.copy()
        window = vars(self.controller.window).copy()
        state = (runtime / 'state.json').read_bytes()
        request = self.request('release_reference', target_id=target_id, actor=self.actor)
        result = app.read_json(runtime / 'results' / (request['id'] + '.json'))
        self.assertTrue(result['ok'])
        self.assertEqual(result['action'], 'target_release_reference')
        self.assertEqual(result['target_id'], target_id)
        reference = result['release_reference']
        self.assertEqual(reference, mark['release_reference'])
        self.assertEqual(result['instance'], status['instance'])
        self.assertEqual(actions.release_marks, marks)
        self.assertEqual(vars(self.controller.window), window)
        self.assertEqual(list(actions.definitions), [target_id])
        self.assertEqual((runtime / 'state.json').read_bytes(), state)
        history = b''.join(path.read_bytes() for path in (runtime / 'history').glob('events-*.jsonl'))
        for published in (state, history):
            self.assertNotIn(b'"release_token"', published)
            self.assertNotIn(reference['release_token'].encode(), published)
        self.release_request('unregister', reference)
        self.assertNotIn(target_id, actions.definitions)

    def test_release_reference_rejects_invalid_unmarked_or_unavailable_target(self):
        target_id = self.target['id']
        self.authorize()
        for selected, error in ((None, 'invalid_arguments'), ('bad/id', 'invalid_arguments'),
                                ('missing-target', 'stale_registration'), (target_id, 'not_releasable')):
            with self.subTest(selected=selected):
                result = self.result(self.request('release_reference', target_id=selected, actor=self.actor))
                self.assertEqual(result['error'], error)
                self.assertNotIn('release_reference', result)

        reference = self.controller.actions.registration_reference(target_id)
        self.release_request('mark_releasable', reference)
        with patch.object(self.controller, 'closing', True):
            result = self.result(self.request('release_reference', target_id=target_id, actor=self.actor))
            self.assertEqual(result['error'], 'busy')
        result = self.result(self.request('release_reference', target_id=target_id, actor={}))
        self.assertEqual(result['error'], 'identity_rejected')
        result = self.result(self.request('release_reference', target_id=target_id, actor=self.actor, instance='0' * 32))
        self.assertEqual(result['error'], 'stale_controller')

    def test_release_rechecks_authorization_and_live_dependencies(self):
        actions = self.controller.actions
        target_id = self.target['id']
        reference = actions.registration_reference(target_id)
        self.release_request('mark_releasable', reference, 'target_authorized')
        self.authorize()
        self.release_request('unregister', dict(reference, release_token='0' * 32), 'stale_release_reference')
        mark = self.release_request('mark_releasable', reference)['release_reference']
        with patch.object(self.controller, 'scope', {'test_actions': {'definitions': [reference['definition_hash']]}}):
            self.release_request('unregister', mark, 'target_authorized')
            result = self.result(self.request('release_reference', target_id=target_id, actor=self.actor))
            self.assertEqual(result['error'], 'target_authorized')
            self.assertNotIn(target_id, self.controller.snapshot()['releasable_targets'])

        self.backend.live[601] = True
        cases = (
            (actions, 'owned', {601: {'identity': {'pid': 601}, 'definition': self.target}}),
            (actions, 'pending', {'id': 'a' * 32, 'request': {'Action': 'Start'}, 'definition': self.target}),
            (self.controller, 'job', {'id': 'b' * 32, 'kind': 'product'}),
            (actions, 'unknown_starts', {'c' * 32: target_id}),
            (actions, 'unknown_starts', {'d' * 32: None}),
        )
        actions.job_definitions.add(target_id)
        for owner, field, dependency in cases:
            with self.subTest(field=field, dependency=dependency), patch.object(owner, field, dependency):
                self.release_request('mark_releasable', reference, 'target_in_use')
                self.release_request('unregister', mark, 'target_in_use')
                with patch.object(actions, 'refresh_owned') as refresh:
                    result = self.result(self.request('release_reference', target_id=target_id, actor=self.actor))
                    self.assertEqual(result['error'], 'target_in_use')
                    refresh.assert_not_called()
                self.assertIn(target_id, actions.definitions)
                self.assertNotIn(target_id, self.controller.snapshot()['releasable_targets'])
        with patch.object(actions, 'unknown_starts', {'e' * 32: 'known-unrelated-target'}):
            self.release_request('unregister', mark)
        self.assertNotIn(target_id, actions.definitions)

    def test_stale_release_reference_cannot_remove_reauthorized_or_reregistered_target(self):
        actions = self.controller.actions
        target_id = self.target['id']
        reference = actions.registration_reference(target_id)
        self.authorize()
        denied = self.result(self.request('mark_releasable', target=reference, actor={}))
        self.assertEqual(denied['error'], 'identity_rejected')
        self.release_request('mark_releasable', dict(reference, instance='0' * 32), 'stale_controller')
        self.release_request('mark_releasable', dict(reference, definition_hash='0' * 64), 'stale_registration')
        original = self.release_request('mark_releasable', reference)['release_reference']
        self.assertEqual(self.release_request('mark_releasable', reference)['release_reference'], original)
        self.authorize([reference['definition_hash']])
        self.assertEqual(actions.releasable_targets(), [])
        result = self.result(self.request('release_reference', target_id=target_id, actor=self.actor))
        self.assertEqual(result['error'], 'target_authorized')
        self.release_request('unregister', original, 'stale_release_reference')
        self.authorize()
        refreshed = self.release_request('mark_releasable', reference)['release_reference']
        self.assertNotEqual(original['release_token'], refreshed['release_token'])
        self.release_request('unregister', original, 'stale_release_reference')
        self.release_request('unregister', dict(refreshed, instance='0' * 32), 'stale_controller')
        self.release_request('unregister', dict(refreshed, definition_hash='0' * 64), 'stale_registration')
        actions.register(self.target, [reference['definition_hash']])
        self.assertEqual(actions.releasable_targets(), [])
        result = self.result(self.request('release_reference', target_id=target_id, actor=self.actor))
        self.assertEqual(result['error'], 'not_releasable')
        current = self.release_request('mark_releasable', reference)['release_reference']
        result = self.result(self.request('release_reference', target_id=target_id, actor=self.actor))
        self.assertEqual(result['release_reference'], current)
        self.release_request('unregister', refreshed, 'stale_release_reference')
        self.release_request('unregister', current)
        updated = self.register(dict(self.target, maximum_seconds=91))
        self.authorize()
        self.release_request('unregister', current, 'stale_registration')
        self.assertEqual(actions.registration_reference(self.target['id']), updated)

    def test_scope_and_owned_capacity_stay_at_32_and_same_target_can_run_repeatedly(self):
        with self.assertRaises(CacheError):
            validate_scope({'test_actions': {'reason': 'scope limit', 'definitions': [f'{n:064x}' for n in range(33)]}})
        actions = self.controller.actions
        for _ in range(2):
            self.start_owned()
            closing = self.action(self.close_request())
            self.finish_action({'Success': True, 'ExitCode': 0})
            self.assertTrue(self.result(closing)['ok'])
            self.assertFalse(actions.owned)
        for pid in range(601, 633):
            self.backend.live[pid] = True
            actions.owned[pid] = {'definition': self.target, 'identity': {'pid': pid}}
        request = self.action(self.start_request())
        self.assertEqual(self.result(request)['error'], 'request_limit')
        self.assertIn('自有测试进程', self.result(request)['message'])
        self.assertEqual(len(actions.owned), 32)

    def test_gui_diagnostics_use_the_actual_result_path(self):
        self.start_owned()
        for index, content in enumerate((None, '{PRIVATE broken')):
            path = self.root / f'out/test/gui-{index}.json'
            request = self.action(dict(self.close_request(), Action='Gui', Name='ReadMapping', ResultFile=str(path)))
            if content is not None:
                path.write_text(content, encoding='utf-8')
            self.controller.actions.pending['process'].code = 8
            self.controller.actions.advance()
            self.assertEqual(self.result(request)['response_state'], 'missing' if content is None else 'invalid')
            self.assertEqual(self.result(request)['native_exit_code'], 8)
            self.assertIn(601, self.controller.actions.owned)
            self.assertFalse(self.controller.closing)

    def test_final_summary_counts_unresolved_owned_and_pending_without_changing_cleanup(self):
        self.start_owned()
        request = self.action(self.start_request())
        pending = self.controller.actions.pending
        self.addCleanup(pending['log'].close)
        self.controller.actions.unknown_starts['a' * 32] = None
        with patch.object(self.controller, 'advance', side_effect=CacheError('test_interruption', 'PRIVATE')):
            with self.assertRaises(CacheError):
                self.controller.serve()
        rows = [json.loads(line) for path in (self.controller.runtime / 'history').glob('events-*.jsonl')
            for line in path.read_text(encoding='utf-8').splitlines()]
        row = next(row for row in rows if row['event'] == 'controller_finish')
        self.assertEqual((row['owned_count'], row['unresolved_count'], row['pending_count']), (1, 1, 1))
        self.assertEqual(row['pending_request'], request['id'])
        self.assertFalse(row['cleanup_complete'])
        self.assertFalse(self.backend.available)
        self.assertFalse(pending['process'].terminated)

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

    def test_release_cli_passes_explicit_reference_and_never_replaces_an_old_controller(self):
        target_file = self.root / 'release-reference.json'
        reference = {'instance': self.controller.instance, 'target_id': self.target['id'],
            'definition_hash': digest(self.target)}
        app.write_json(target_file, reference)
        state = self.controller.snapshot()
        with patch.object(app, 'token_info', return_value={'elevated': False, 'integrity': 8192}), \
                patch.object(app, 'runtime_safe'), patch.object(app, 'send') as send, \
                patch.object(app, 'ensure_controller') as create:
            with patch.object(app, 'verified_controller', return_value=state):
                app.client(['mark-releasable'], target_file=target_file)
                send.assert_called_once_with(state, {'op': 'mark_releasable', 'target': reference}, timeout=10, request_id=None)
                send.reset_mock()
                release = dict(reference, release_token='f' * 32)
                app.write_json(target_file, release)
                app.client(['unregister'], target_file=target_file)
                send.assert_called_once_with(state, {'op': 'unregister', 'target': release}, timeout=10, request_id=None)
                send.reset_mock()
            for old in (None, {'format_version': 2}):
                with patch.object(app, 'verified_controller', return_value=old):
                    with self.assertRaises(CacheError):
                        app.client(['unregister'], target_file=target_file)
                    send.assert_not_called()
            create.assert_not_called()

    def test_release_reference_cli_checks_capability_and_preserves_legacy_release_commands(self):
        target_id = self.target['id']
        state = self.controller.snapshot()
        legacy = {key: value for key, value in state.items() if key != 'release_reference_supported'}
        reference = self.controller.actions.registration_reference(target_id)
        target_file = self.root / 'release-reference.json'
        with patch.object(app, 'token_info', return_value={'elevated': False, 'integrity': 8192}), \
                patch.object(app, 'runtime_safe'), patch.object(app, 'verified_controller') as verified, \
                patch.object(app, 'send') as send, patch.object(app, 'ensure_controller') as create:
            verified.return_value = state
            request_id = 'e' * 32
            app.client(['release-ref', target_id], request_id=request_id)
            send.assert_called_once_with(state, {'op': 'release_reference', 'target_id': target_id},
                                         timeout=10, request_id=request_id)
            send.reset_mock()
            for old, error in ((legacy, 'controller_upgrade_required'), (None, 'controller_absent')):
                with self.subTest(error=error):
                    with self.assertRaises(CacheError) as rejected:
                        verified.return_value = old
                        app.client(['release-ref', target_id])
                    self.assertEqual(rejected.exception.code, error)
            send.assert_not_called()
            verified.return_value = legacy
            for command, operation, value in (
                    ('mark-releasable', 'mark_releasable', reference),
                    ('unregister', 'unregister', dict(reference, release_token='f' * 32))):
                with self.subTest(command=command):
                    app.write_json(target_file, value)
                    app.client([command], target_file=target_file)
                    send.assert_called_once_with(legacy, {'op': operation, 'target': value}, timeout=10, request_id=None)
                    send.reset_mock()
            create.assert_not_called()

    def test_release_cli_rejects_conflicting_files_and_invalid_query_arguments(self):
        target_id = self.target['id']
        target_file = self.root / 'release-reference.json'
        with patch.object(app, 'token_info', return_value={'elevated': False, 'integrity': 8192}), \
                patch.object(app, 'runtime_safe'), patch.object(app, 'verified_controller') as verified:
            for words, target in ((['release-ref', target_id], None), (['mark-releasable'], target_file),
                                  (['unregister'], target_file)):
                for option in ('scope_file', 'job_file', 'definition_file', 'action_file'):
                    with self.subTest(words=words, option=option):
                        with self.assertRaises(CacheError) as rejected:
                            app.client(words, target_file=target, **{option: target_file})
                        self.assertEqual(rejected.exception.code, 'invalid_arguments')
            for words, target in ((['release-ref'], None), (['release-ref', 'bad/id'], None),
                                  (['release-ref', target_id, target_id], None), (['release-ref', target_id], target_file)):
                with self.subTest(words=words, target=target):
                    with self.assertRaises(CacheError) as rejected:
                        app.client(words, target_file=target)
                    self.assertEqual(rejected.exception.code, 'invalid_arguments')
            verified.assert_not_called()

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
        with patch.object(app, 'verified_controller', return_value=old), \
                patch.object(app, 'send', return_value=dict(old, unresolved_starts=['a' * 32])) as send, \
                patch.object(app, 'ensure_controller') as create:
            with self.assertRaises(CacheError):
                app.upgrade_controller()
            send.assert_called_once_with(old, {'op': 'status'}, timeout=10)
            create.assert_not_called()


if __name__ == '__main__':
    unittest.main()
