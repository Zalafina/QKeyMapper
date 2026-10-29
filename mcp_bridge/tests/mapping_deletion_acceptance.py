"""Formal stdio deletion checks for the bounded native settings_save_apply/delete_* fixture.

Only synthetic sources in an isolated owned runtime; no system input or mapping start.
GUI open/close and stale-source markers use the maintained native fixture. Inline draft
and popup markers are component injections, not independent GUI deletion baselines.
"""
import asyncio
import json
import time
from pathlib import Path
from jsonschema import Draft202012Validator
from test_sdk_interop import business_state, BUSINESS_CAPABILITIES

def fixture_bytes():
    source = Path(__file__).resolve().parents[2] / 'QKeyMapper/tests/sendinput_lifecycle/common_save_fixture.ini.example'
    optional = (b'KeyMapData_Note=', b'KeyMapData_Category=', b'KeyMapData_MappingKeys_Comments=',
                b'KeyMapData_MappingKeys_KeyUp_Comments=', b'CommonMappingData=')
    return b''.join(line for line in source.read_bytes().splitlines(keepends=True) if not line.startswith(optional))

def write_json(path, value):
    path.write_bytes(json.dumps(value, ensure_ascii=False, indent=2).encode('utf-8'))

def read_json(path):
    with path.open('rb') as stream:
        data = stream.read(2 * 1024 * 1024 + 1)
    assert len(data) <= 2 * 1024 * 1024
    return json.loads(data.decode('utf-8'))

async def exercise(client, request, variant, output, exe):
    assert variant in ('delete_normal', 'delete_common_first', 'delete_common_all', 'delete_stale', 'delete_read', 'delete_hidden')
    capabilities = (await request('qkm_get_capabilities'))['result']['business_capabilities']
    assert {c['id'] for c in capabilities} == BUSINESS_CAPABILITIES
    for id in ('mapping.delete.preview', 'mapping.delete.apply'):
        capability = next((c for c in capabilities if c['id'] == id))
        assert capability['required_scopes'] == 3 and capability['read_only'] == id.endswith('.preview')
        assert capability['arguments_schema']['additionalProperties'] is False
    initial = business_state((await request('qkm_get_state'))['result'])
    disk = (exe.parent / 'keymapdata.ini').read_bytes()
    assert disk == fixture_bytes(), 'Only the bounded synthetic deletion fixture is permitted'
    source = (await request('qkm_query', dict(entity='mapping_sources', limit=100)))['result']
    revision = source['source_snapshot']['revision']
    tab = next((t for t in source['items'] if t['attached_to_ui'] and t['ui_index'] == initial['current_tab']['index']))
    row_args = dict(entity='mappings', source_revision=revision, source_tab_ref=tab['source_tab_ref'], limit=100)
    rows = (await request('qkm_query', row_args))['result']['items']
    assert len(rows) == (2 if variant.startswith('delete_common') or variant == 'delete_stale' else 1)
    arguments = dict(action='mapping.delete', source_revision=revision, source_tab_ref=tab['source_tab_ref'], source_refs=[r['source_ref'] for r in rows])
    if variant == 'delete_common_first':
        arguments['source_refs'] = arguments['source_refs'][:1]
    elif variant == 'delete_common_all':
        arguments['source_refs'] = [rows[1]['source_ref'], rows[0]['source_ref'], rows[1]['source_ref']]
    tool = next((t for t in (await client.list_tools()).tools if t.name == 'qkm_preview_changes'))
    schema = tool.model_dump(by_alias=True)['inputSchema']
    validator = Draft202012Validator(schema)
    assert not list(validator.iter_errors(arguments))
    schema_errors = []
    for extra in ({'source_refs': []}, {'source_refs': rows[0]['source_ref']}, {'source_refs': [0]}, {'source_refs': [rows[0]['source_ref']] * 101}, {'original_key': 'A'}, {'pid': 123}, {'insert_row': 0}):
        reply = await client.call_tool('qkm_preview_changes', {**arguments, **extra})
        assert reply.is_error
        schema_errors.append(reply.model_dump(by_alias=True))
    write_json(output / 'delete-schema-errors.json', schema_errors)
    if variant == 'delete_read':
        await request('qkm_preview_changes', arguments, error='permission_denied')
        await request('qkm_apply_changes', dict(preview_id='a' * 32, confirm_delete=True), error='permission_denied')
        assert business_state((await request('qkm_get_state'))['result']) == initial
        assert (await request('qkm_query', row_args))['result']['items'] == rows
        assert (exe.parent / 'keymapdata.ini').read_bytes() == disk
        return dict(successful_saves=0, read_denied=True, schema_errors=len(schema_errors))

    async def preview():
        value = (await request('qkm_preview_changes', arguments))['result']
        description = value['description']
        assert description['action'] == 'mapping.delete' and description['target']['instance_id'] == initial['instance']
        assert description['target']['source_tab_ref'] == tab['source_tab_ref'] and description['confirmation_required'] is True
        assert description['modified'] == description['saved'] == description['started_by_operation'] == False
        indices = list(dict.fromkeys((int(ref.rsplit(':', 1)[1]) for ref in arguments['source_refs'])))
        assert [c['source_row'] for c in description['changes']] == indices
        for change in description['changes']:
            row = rows[change['source_row']]
            assert change['mapping'] == {k: v for k, v in row.items() if k not in ('source_ref', 'source_tab_ref', 'source_row')}
        assert business_state((await request('qkm_get_state'))['result']) == initial
        assert (await request('qkm_query', row_args))['result']['items'] == rows
        return value
    pending = await preview()
    cancelled = (await request('qkm_cancel_changes', dict(preview_id=pending['preview_id'])))['result']
    assert cancelled['status'] == 'cancelled' and cancelled['target']['instance_id'] == initial['instance']
    missing = await preview()
    await request('qkm_apply_changes', dict(preview_id=missing['preview_id']), error='confirmation_required')
    operation = (await request('qkm_get_operation', dict(operation_id=missing['operation_id'])))['result']
    assert operation['status'] == 'rejected' and operation['result']['modified'] is False
    wrong = await preview()
    await request('qkm_apply_changes', dict(preview_id=wrong['preview_id'], confirm_conflicts=True, confirm_delete=True), error='invalid_arguments')
    await request('qkm_preview_changes', {**arguments, 'source_refs': [revision + ':row:' + str((initial['current_tab']['index'] + 1) % 2) + ':0']}, error='invalid_arguments')
    await request('qkm_preview_changes', {**arguments, 'source_refs': [revision + ':row:' + str(initial['current_tab']['index']) + ':9999']}, error='invalid_target')

    async def marker(start, finish):
        write_json(output / ('case.json.' + start), {'fixed_gui': True})
        until = time.monotonic() + 5
        path = output / ('case.json.' + finish)
        while not path.exists():
            assert time.monotonic() < until, finish
            await asyncio.sleep(0.01)
        assert read_json(path)['ok']
    await marker('delete-open-editor', 'delete-editor-opened')
    await request('qkm_preview_changes', arguments, error='context_busy')
    await marker('delete-close-editor', 'delete-editor-closed')
    editing = await preview()
    await marker('delete-table-edit', 'delete-table-editing')
    await request('qkm_preview_changes', arguments, error='context_busy')
    await request('qkm_apply_changes', dict(preview_id=editing['preview_id'], confirm_delete=True), error='context_busy')
    await marker('delete-table-cancel', 'delete-table-cancelled')
    popup = await preview()
    await marker('delete-popup', 'delete-popup-opened')
    await request('qkm_preview_changes', arguments, error='context_busy')
    await request('qkm_apply_changes', dict(preview_id=popup['preview_id'], confirm_delete=True), error='context_busy')
    await marker('delete-popup-close', 'delete-popup-closed')
    if variant == 'delete_hidden':
        hidden = await preview()
        await marker('delete-hide-row', 'delete-row-hidden')
        await request('qkm_preview_changes', arguments, error='invalid_target')
        await request('qkm_apply_changes', dict(preview_id=hidden['preview_id'], confirm_delete=True), error='invalid_target')
        await marker('delete-show-row', 'delete-row-shown')
    pending = await preview()
    if variant == 'delete_stale':
        await marker('delete-gui-remove', 'delete-gui-removed')
        await request('qkm_apply_changes', dict(preview_id=pending['preview_id'], confirm_delete=True), error='stale_source')
        operation = (await request('qkm_get_operation', dict(operation_id=pending['operation_id'])))['result']
        assert operation['status'] == 'rejected' and operation['result']['modified'] is False
        assert operation['target']['instance_id'] == initial['instance']
        deleted = False
    else:
        applied = (await request('qkm_apply_changes', dict(preview_id=pending['preview_id'], confirm_delete=True)))['result']
        assert applied['status'] == 'applied' and applied['result']['action'] == 'mapping.delete'
        assert applied['result']['deleted_count'] == len(set(arguments['source_refs']))
        assert applied['result']['modified'] is True and applied['result']['saved'] == applied['result']['started_by_operation'] == False
        assert applied['target']['instance_id'] == initial['instance'] and applied['target']['source_tab_ref'] == tab['source_tab_ref']
        replay = (await request('qkm_apply_changes', dict(preview_id=pending['preview_id'], confirm_delete=True)))['result']
        assert replay['replayed'] is True and replay['result'] == applied['result'] and (replay['target'] == applied['target'])
        operation = (await request('qkm_get_operation', dict(operation_id=pending['operation_id'])))['result']
        assert operation['historical_result'] and operation['result'] == applied['result']
        deleted = True
    await request('qkm_query', row_args, error='stale_source')
    await request('qkm_preview_changes', arguments, error='stale_source')
    after = business_state((await request('qkm_get_state'))['result'])
    current = (await request('qkm_query', dict(entity='mapping_sources', limit=100)))['result']
    current_tab = next((t for t in current['items'] if t['attached_to_ui'] and t['ui_index'] == after['current_tab']['index']))
    remaining = (await request('qkm_query', dict(entity='mappings', source_revision=current['source_snapshot']['revision'], source_tab_ref=current_tab['source_tab_ref'], limit=100)))['result']['items']
    baseline_variant = 'common_first' if variant in ('delete_common_first', 'delete_stale') else 'common_all' if variant == 'delete_common_all' else 'normal_last'
    removed = {0} if variant == 'delete_stale' else {int(ref.rsplit(':', 1)[1]) for ref in arguments['source_refs']}
    expected_rows = [{k: v for k, v in row.items() if k not in ('source_ref', 'source_tab_ref', 'source_row')} for i, row in enumerate(rows) if i not in removed]
    actual_rows = [{k: v for k, v in row.items() if k not in ('source_ref', 'source_tab_ref', 'source_row')} for row in remaining]
    assert actual_rows == expected_rows
    write_json(output / 'delete-remaining-rows.json', actual_rows)
    assert after['mapping_load_context'] == initial['mapping_load_context'] and after['configuration']['has_unsaved_changes']
    assert (exe.parent / 'keymapdata.ini').read_bytes() == disk
    return dict(successful_saves=0, deleted=deleted, baseline=baseline_variant, remaining=len(remaining), schema_errors=len(schema_errors))
