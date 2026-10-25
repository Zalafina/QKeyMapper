"""Ordinary clients for the workspace's registered gsudo test actions."""

import argparse
import os
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / '.agents/skills/qkeymapper-gsudo-cache/scripts'))
import cache_control as control
from cache_core import require


def perform(request):
    require(control.token_info() == {'elevated': False, 'integrity': 8192},
        '测试控制器客户端须保持普通 Medium 权限。', 'integrity_rejected')
    control.runtime_safe()
    state = control.verified_controller()
    require(state is not None, '没有受管控制器。先设置工作区缓存；专门验证 UAC 时显式选择不缓存。', 'controller_absent')
    require(state['format_version'] == 2, '现有控制器需空闲升级；未另建控制器或直接申请 UAC。', 'controller_upgrade_required')
    result = control.send(state, {'op': 'test_action', 'request': request,
        'job_token': os.environ.get('QKM_CACHE_JOB')}, timeout=80)
    require(result.get('ok') is True, result.get('message', '固定动作失败，先查询原请求结果；不要盲目重试。')
        + ' request_id=' + str(result.get('request_id', '')), result.get('error', 'test_action_failed'))
    return result['response']


def reject_nested_cache(requested):
    require(not requested, '已改用工作区受管控制器；先通过缓存技能设置窗口，省略 --gsudo-cache。不会创建临时或嵌套缓存。',
        'nested_cache_rejected')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--request', type=Path, required=True)
    parser.add_argument('--response', type=Path, required=True)
    args = parser.parse_args()
    try:
        for path in (args.request, args.response):
            require(path.resolve().is_relative_to(ROOT / 'out'), '通信文件必须在本工作区 out。')
        result = perform(control.read_json(args.request))
    except Exception as error:
        result = {'Success': False, **control.error_result(error)}
    control.write_json(args.response, result)
    return 0 if result.get('Success') is True else 1


if __name__ == '__main__':
    sys.exit(main())
