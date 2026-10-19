"""Derive the UI-only entry from the current product; never edit product files."""
import argparse
from pathlib import Path

REPO = Path(__file__).resolve().parents[4]

def replace_once(text, old, new):
    if text.count(old) != 1:
        raise ValueError('Product entry changed; review this anchor before adapting: ' + old)
    return text.replace(old, new, 1)

def derive(text):
    text = replace_once(text, '#include <QApplication>', '#include "manual_probe.h"\n#include <QApplication>')
    text = replace_once(text, 'int main(int argc, char *argv[])\n{', '''int main(int argc, char *argv[])
{
    wchar_t observerDesktop[256] = {};
    DWORD observerNeeded = 0;
    if (qEnvironmentVariableIsEmpty("QKM_MANUAL_SESSION") ||
        !GetUserObjectInformationW(GetThreadDesktop(GetCurrentThreadId()), UOI_NAME,
                                  observerDesktop, sizeof(observerDesktop), &observerNeeded) ||
        !QString::fromWCharArray(observerDesktop).startsWith(QStringLiteral("QkmManual_"))) {
        return 2;
    }''')
    text = replace_once(text, '    SingleApplication app(argc, argv);',
                        '    QApplication::setApplicationName(QStringLiteral("QkmManual_") + qEnvironmentVariable("QKM_MANUAL_SESSION"));\n    SingleApplication app(argc, argv);')
    for line in [
        '    QObject::connect(interceptionThread, &QThread::started, interception_worker, &Interception_Worker::InterceptionThreadStarted);',
        '    QObject::connect(hookprocThread, &QThread::started, keymapper_hook_proc, &QKeyMapper_Hook_Proc::HookProcThreadStarted);',
    ]:
        text = replace_once(text, line, '    // UI observer: do not start input interception or hooks.')
    text = replace_once(text, '    int ret = app.exec();',
                        '    w.hide();\n    w.show();\n    ManualProbe::install(&w);\n    int ret = app.exec();')
    return text

def prepare(output):
    text = derive((REPO / 'QKeyMapper/main.cpp').read_text(encoding='utf-8'))
    output = output.resolve()
    if not output.is_relative_to(REPO / 'out/user_manual'):
        raise ValueError('Observer output must stay in out/user_manual')
    output.parent.mkdir(parents=True, exist_ok=True)
    with output.open('w', encoding='utf-8', newline='\n') as target:
        target.write(text)
    print(output)

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, default=REPO / 'out/user_manual/observer/main_manual_probe.cpp')
    parser.add_argument('--self-test', action='store_true')
    args = parser.parse_args()
    if args.self_test:
        source = (REPO / 'QKeyMapper/main.cpp').read_text(encoding='utf-8')
        result = derive(source)
        assert 'ManualProbe::install(&w)' in result
        assert '&QKeyMapper_Hook_Proc::HookProcThreadStarted' not in result
        for bad in [source.replace('    int ret = app.exec();', ''), source + '\n    int ret = app.exec();']:
            try:
                derive(bad)
            except ValueError:
                pass
            else:
                raise AssertionError('Missing/duplicate anchor must fail')
        print('Observer entry self-test passed')
    else:
        prepare(args.output)
