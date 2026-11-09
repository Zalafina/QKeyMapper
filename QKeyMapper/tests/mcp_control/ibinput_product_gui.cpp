#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSaveFile>
#include <QThread>
#include <Windows.h>
#include <ole2.h>
#include <UIAutomation.h>
#include <wrl/client.h>
#include <iostream>
#include <algorithm>
#include <stdexcept>
#include <vector>

using Microsoft::WRL::ComPtr;
namespace {
void require(bool value, const char *message)
{
    if (!value) throw std::runtime_error(message);
}
void checked(HRESULT result, const char *operation)
{
    if (FAILED(result)) throw std::runtime_error(std::string(operation) + " HRESULT=" + std::to_string(static_cast<unsigned long>(result)));
}
QString desktopName(DWORD thread)
{
    wchar_t name[256]{};
    DWORD needed = 0;
    require(GetUserObjectInformationW(GetThreadDesktop(thread), UOI_NAME, name, sizeof(name), &needed), "Cannot read desktop identity.");
    return QString::fromWCharArray(name);
}
QString nameOf(IUIAutomationElement *element)
{
    BSTR text = nullptr;
    checked(element->get_CurrentName(&text), "CurrentName");
    const QString result = QString::fromWCharArray(text ? text : L"");
    SysFreeString(text);
    return result;
}
class Target {
public:
    Target(DWORD pid, quint64 created, const QString &executable) : m_pid(pid)
    {
        m_process = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION | SYNCHRONIZE, FALSE, pid);
        require(m_process != nullptr, "Cannot open owned product.");
        FILETIME start{}, end{}, kernel{}, user{};
        require(GetProcessTimes(m_process, &start, &end, &kernel, &user), "Cannot read process creation time.");
        require(((quint64(start.dwHighDateTime) << 32) | start.dwLowDateTime) == created, "Product PID identity changed.");
        wchar_t image[32768]{};
        DWORD length = 32768;
        require(QueryFullProcessImageNameW(m_process, 0, image, &length), "Cannot read product image.");
        require(QFileInfo(QString::fromWCharArray(image, int(length))).canonicalFilePath().compare(
            QFileInfo(executable).canonicalFilePath(), Qt::CaseInsensitive) == 0, "Product EXE identity changed.");
        require(QFileInfo(executable).fileName() == QStringLiteral("QKeyMapperMcpAcceptance_ibinput_qt6.exe"), "Select the dedicated fixture EXE.");
        m_desktop = desktopName(GetCurrentThreadId());
        require(m_desktop.startsWith(QStringLiteral("QkmProduct_")) && m_desktop.size() == 43, "A dedicated private product desktop is required.");
        checked(CoCreateInstance(__uuidof(CUIAutomation), nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&m_automation)), "CUIAutomation");
    }
    ~Target() { if (m_process) CloseHandle(m_process); }
    ComPtr<IUIAutomationElement> find(const wchar_t *id)
    {
        for (int attempt = 0; attempt < 100; ++attempt) {
            require(WaitForSingleObject(m_process, 0) == WAIT_TIMEOUT, "Product exited.");
            m_windows.clear();
            require(EnumDesktopWindows(GetThreadDesktop(GetCurrentThreadId()), [](HWND window, LPARAM context) -> BOOL {
                auto *self = reinterpret_cast<Target *>(context);
                DWORD pid = 0;
                GetWindowThreadProcessId(window, &pid);
                if (pid == self->m_pid && IsWindowVisible(window)) {
                    self->m_windows.push_back(window);
                }
                return TRUE;
            }, reinterpret_cast<LPARAM>(this)), "Cannot enumerate owned desktop windows.");
            ComPtr<IUIAutomationCondition> condition;
            checked(m_automation->CreateTrueCondition(&condition), "Owned descendants condition");
            for (HWND window : m_windows) {
                require(desktopName(GetWindowThreadProcessId(window, nullptr)) == m_desktop, "Target desktop changed.");
                ComPtr<IUIAutomationElement> root, found;
                checked(m_automation->ElementFromHandle(window, &root), "Owned window element");
                ComPtr<IUIAutomationElementArray> elements;
                checked(root->FindAll(TreeScope_Descendants, condition.Get(), &elements), "Owned control lookup");
                int length = 0; checked(elements->get_Length(&length), "Owned control count");
                require(length <= 4096, "Owned UI tree exceeded the fixture bound.");
                for (int index = 0; index < length; ++index) {
                    ComPtr<IUIAutomationElement> element;
                    checked(elements->GetElement(index, &element), "Owned descendant");
                    BSTR raw = nullptr;
                    checked(element->get_CurrentAutomationId(&raw), "Owned AutomationId");
                    const QString actual = QString::fromWCharArray(raw ? raw : L"");
                    SysFreeString(raw);
                    const QString wanted = QString::fromWCharArray(id);
                    if (actual == wanted || actual.endsWith(QLatin1Char('.') + wanted)) {
                        int pid = 0; checked(element->get_CurrentProcessId(&pid), "Control owner");
                        require(pid == int(m_pid) && !found, "Ambiguous fixed owned control.");
                        found = element;
                    }
                }
                if (found) return found;
            }
            QThread::msleep(30);
        }
        throw std::runtime_error("The fixed owned control did not appear.");
    }
    void invoke(const wchar_t *id)
    {
        ComPtr<IUIAutomationInvokePattern> pattern;
        checked(find(id)->GetCurrentPatternAs(UIA_InvokePatternId, IID_PPV_ARGS(&pattern)), "Invoke pattern");
        require(bool(pattern), "The fixed button has no Invoke pattern.");
        checked(pattern->Invoke(), "Invoke fixed button");
    }
    HWND ownedWindow(IUIAutomationElement *element)
    {
        ComPtr<IUIAutomationElement> parent = element;
        ComPtr<IUIAutomationTreeWalker> walker;
        checked(m_automation->get_ControlViewWalker(&walker), "Owned parent walker");
        UIA_HWND raw = nullptr;
        for (int depth = 0; depth < 20 && !raw; ++depth) {
            checked(parent->get_CurrentNativeWindowHandle(&raw), "Owned window handle");
            if (!raw) { ComPtr<IUIAutomationElement> next; checked(walker->GetParentElement(parent.Get(), &next), "Owned parent"); require(bool(next), "No owned native parent."); parent = next; }
        }
        const HWND window = static_cast<HWND>(raw);
        DWORD owner = 0;
        const DWORD thread = GetWindowThreadProcessId(window, &owner);
        require(window && owner == m_pid && desktopName(thread) == m_desktop, "Owned window identity changed.");
        return window;
    }
    void click(IUIAutomationElement *element)
    {
        const HWND window = ownedWindow(element);
        RECT bounds{}; checked(element->get_CurrentBoundingRectangle(&bounds), "Fixed control bounds");
        require(bounds.right > bounds.left && bounds.bottom > bounds.top, "Empty fixed control bounds.");
        POINT point{(bounds.left + bounds.right) / 2, (bounds.top + bounds.bottom) / 2};
        require(ScreenToClient(window, &point), "Cannot locate the owned GUI control.");
        const LPARAM position = MAKELPARAM(point.x, point.y);
        require(PostMessageW(window, WM_LBUTTONDOWN, MK_LBUTTON, position) && PostMessageW(window, WM_LBUTTONUP, 0, position), "Cannot click the owned GUI control.");
    }
    QString choose(const wchar_t *id, int index, int count)
    {
        auto combo = find(id);
        const auto before = m_windows;
        ComPtr<IUIAutomationExpandCollapsePattern> expand;
        checked(combo->GetCurrentPatternAs(UIA_ExpandCollapsePatternId, IID_PPV_ARGS(&expand)), "Expand pattern");
        if (expand) checked(expand->Expand(), "Open fixed combo");
        else click(combo.Get());
        ComPtr<IUIAutomationCondition> condition;
        VARIANT value{}; value.vt = VT_I4; value.lVal = UIA_ListItemControlTypeId;
        checked(m_automation->CreatePropertyCondition(UIA_ControlTypePropertyId, value, &condition), "List condition");
        ComPtr<IUIAutomationElementArray> items;
        int length = 0;
        for (int attempt = 0; attempt < 100 && length != count; ++attempt) {
            std::vector<HWND> popups;
            require(EnumDesktopWindows(GetThreadDesktop(GetCurrentThreadId()), [](HWND window, LPARAM data) -> BOOL {
                auto *list = reinterpret_cast<std::vector<HWND> *>(data);
                if (IsWindowVisible(window)) list->push_back(window);
                return TRUE;
            }, reinterpret_cast<LPARAM>(&popups)), "Cannot enumerate combo windows.");
            for (const HWND window : popups) {
                DWORD owner = 0; GetWindowThreadProcessId(window, &owner);
                if (owner != m_pid || std::find(before.begin(), before.end(), window) != before.end()) continue;
                ComPtr<IUIAutomationElement> root;
                checked(m_automation->ElementFromHandle(window, &root), "Owned combo window");
                checked(root->FindAll(TreeScope_Descendants, condition.Get(), &items), "Fixed combo items");
                checked(items->get_Length(&length), "Combo count");
                if (length == count) break;
            }
            if (length != count) QThread::msleep(30);
        }
        require(length == count && index >= 0 && index < length, "Unexpected fixed combo choices.");
        ComPtr<IUIAutomationElement> item;
        checked(items->GetElement(index, &item), "Fixed item");
        const QString label = nameOf(item.Get());
        click(item.Get());
        ComPtr<IUIAutomationValuePattern> selection;
        checked(combo->GetCurrentPatternAs(UIA_ValuePatternId, IID_PPV_ARGS(&selection)), "Current combo value pattern");
        require(bool(selection), "The fixed combo has no readable Value pattern.");
        bool observed = false;
        for (int attempt = 0; attempt < 100 && !observed; ++attempt) {
            BSTR current = nullptr;
            checked(selection->get_CurrentValue(&current), "Current combo selection");
            observed = QString::fromWCharArray(current ? current : L"") == label;
            SysFreeString(current);
            if (!observed) QThread::msleep(30);
        }
        require(observed, "The requested GUI option was not observed selected.");
        return label;
    }
    void escape(IUIAutomationElement *element)
    {
        ComPtr<IUIAutomationElement> parent = element;
        ComPtr<IUIAutomationTreeWalker> walker;
        checked(m_automation->get_ControlViewWalker(&walker), "Popup parent walker");
        UIA_HWND raw = nullptr;
        for (int depth = 0; depth < 20 && !raw; ++depth) {
            checked(parent->get_CurrentNativeWindowHandle(&raw), "Window handle");
            if (!raw) { ComPtr<IUIAutomationElement> next; checked(walker->GetParentElement(parent.Get(), &next), "Popup parent"); require(bool(next), "No owned popup parent."); parent = next; }
        }
        const HWND window = static_cast<HWND>(raw);
        DWORD pid = 0; GetWindowThreadProcessId(window, &pid);
        require(window && pid == m_pid, "Only an owned popup may receive Escape.");
        require(PostMessageW(window, WM_KEYDOWN, VK_ESCAPE, 1) && PostMessageW(window, WM_KEYUP, VK_ESCAPE, 1), "Cannot post owned GUI Escape.");
    }
    void closeGeneral()
    {
        (void)find(L"keymapButton");
        int matches = 0;
        for (const HWND window : m_windows) {
            wchar_t title[256]{};
            GetWindowTextW(window, title, 256);
            if (!QStringList{QStringLiteral("通用高级设定"), QStringLiteral("General Advanced Setting"), QStringLiteral("一般高度設定")}.contains(QString::fromWCharArray(title))) continue;
            require(++matches == 1, "Ambiguous owned general settings window.");
            ComPtr<IUIAutomationElement> root;
            checked(m_automation->ElementFromHandle(window, &root), "General settings window");
            ComPtr<IUIAutomationWindowPattern> pattern;
            checked(root->GetCurrentPatternAs(UIA_WindowPatternId, IID_PPV_ARGS(&pattern)), "General close pattern");
            checked(pattern->Close(), "Close owned general settings");
        }
    }
    void enterFixedSettingName()
    {
        auto field = find(L"settingNameLineEdit");
        ComPtr<IUIAutomationValuePattern> value;
        checked(field->GetCurrentPatternAs(UIA_ValuePatternId, IID_PPV_ARGS(&value)), "Setting name observation");
        require(bool(value), "The setting name has no readable Value pattern.");
        BSTR text = nullptr;
        checked(value->get_CurrentValue(&text), "Setting name value");
        const QString before = QString::fromWCharArray(text ? text : L"");
        SysFreeString(text);
        if (before == QStringLiteral("Persist136A")) return;
        require(before.isEmpty(), "Only the empty synthetic setting name may be typed.");
        RECT bounds{}; checked(field->get_CurrentBoundingRectangle(&bounds), "Setting name bounds");
        auto parent = field;
        ComPtr<IUIAutomationTreeWalker> walker; checked(m_automation->get_ControlViewWalker(&walker), "Name parent walker");
        UIA_HWND raw = nullptr;
        for (int depth = 0; depth < 20 && !raw; ++depth) {
            checked(parent->get_CurrentNativeWindowHandle(&raw), "Name window handle");
            if (!raw) { ComPtr<IUIAutomationElement> next; checked(walker->GetParentElement(parent.Get(), &next), "Name parent"); require(bool(next), "No owned name parent."); parent = next; }
        }
        const HWND window = static_cast<HWND>(raw);
        DWORD pid = 0; GetWindowThreadProcessId(window, &pid);
        require(window && pid == m_pid, "Name input must remain in the owned product.");
        POINT point{(bounds.left + bounds.right) / 2, (bounds.top + bounds.bottom) / 2};
        require(ScreenToClient(window, &point), "Cannot locate owned name field.");
        const LPARAM position = MAKELPARAM(point.x, point.y);
        require(PostMessageW(window, WM_LBUTTONDOWN, MK_LBUTTON, position) && PostMessageW(window, WM_LBUTTONUP, 0, position), "Cannot focus owned name field.");
        for (const auto character : QStringLiteral("Persist136A"))
            require(PostMessageW(window, WM_CHAR, character.unicode(), 1), "Cannot type the fixed setting name.");
        bool observed = false;
        for (int attempt = 0; attempt < 100 && !observed; ++attempt) {
            checked(value->get_CurrentValue(&text), "Typed setting observation");
            observed = QString::fromWCharArray(text ? text : L"") == QStringLiteral("Persist136A");
            SysFreeString(text); text = nullptr;
            if (!observed) QThread::msleep(30);
        }
        require(observed, "Fixed GUI name input was not observed.");
    }
    void openFirstMapping()
    {
        (void)find(L"keymapButton");
        ComPtr<IUIAutomationElement> grid;
        ComPtr<IUIAutomationCondition> tableType, gridType, condition;
        VARIANT type{}; type.vt = VT_I4; type.lVal = UIA_TableControlTypeId;
        checked(m_automation->CreatePropertyCondition(UIA_ControlTypePropertyId, type, &tableType), "Table condition");
        type.lVal = UIA_DataGridControlTypeId;
        checked(m_automation->CreatePropertyCondition(UIA_ControlTypePropertyId, type, &gridType), "Grid condition");
        checked(m_automation->CreateOrCondition(tableType.Get(), gridType.Get(), &condition), "Mapping table condition");
        for (HWND window : m_windows) {
            ComPtr<IUIAutomationElement> root;
            checked(m_automation->ElementFromHandle(window, &root), "Mapping window");
            ComPtr<IUIAutomationElementArray> candidates;
            checked(root->FindAll(TreeScope_Descendants, condition.Get(), &candidates), "Mapping grids");
            int length = 0; checked(candidates->get_Length(&length), "Grid count");
            for (int index = 0; index < length; ++index) {
                ComPtr<IUIAutomationElement> candidate;
                checked(candidates->GetElement(index, &candidate), "Grid candidate");
                BOOL offscreen = TRUE; checked(candidate->get_CurrentIsOffscreen(&offscreen), "Grid visibility");
                ComPtr<IUIAutomationGridPattern> candidatePattern;
                if (offscreen || FAILED(candidate->GetCurrentPatternAs(UIA_GridPatternId, IID_PPV_ARGS(&candidatePattern)))) continue;
                int columns = 0; checked(candidatePattern->get_CurrentColumnCount(&columns), "Grid columns");
                if (columns == 7) { require(!grid, "Expected one visible product mapping grid."); grid = candidate; }
            }
        }
        require(bool(grid), "No visible product mapping grid.");
        ComPtr<IUIAutomationGridPattern> pattern;
        checked(grid->GetCurrentPatternAs(UIA_GridPatternId, IID_PPV_ARGS(&pattern)), "Mapping grid");
        int rows = 0, columns = 0;
        checked(pattern->get_CurrentRowCount(&rows), "Mapping rows");
        checked(pattern->get_CurrentColumnCount(&columns), "Mapping columns");
        require(rows == 1 && columns == 7, "Only the one-row synthetic mapping may be edited.");
        ComPtr<IUIAutomationElement> original, mapping;
        checked(pattern->GetItem(0, 0, &original), "Original cell");
        checked(pattern->GetItem(0, 1, &mapping), "Mapping cell");
        require(nameOf(original.Get()) == QStringLiteral("A") && nameOf(mapping.Get()) == QStringLiteral("B"), "Expected the GUI A-to-B baseline.");
        RECT bounds{}; checked(original->get_CurrentBoundingRectangle(&bounds), "Cell bounds");
        auto main = find(L"keymapButton");
        ComPtr<IUIAutomationTreeWalker> walker; checked(m_automation->get_ControlViewWalker(&walker), "Control walker");
        UIA_HWND raw = nullptr;
        for (int parent = 0; parent < 20 && !raw; ++parent) {
            checked(main->get_CurrentNativeWindowHandle(&raw), "Main handle");
            if (!raw) { ComPtr<IUIAutomationElement> next; checked(walker->GetParentElement(main.Get(), &next), "Main parent"); require(bool(next), "No main parent."); main = next; }
        }
        const HWND window = static_cast<HWND>(raw);
        DWORD pid = 0; GetWindowThreadProcessId(window, &pid);
        require(window && pid == m_pid && nameOf(find(L"keymapButton").Get()) == QStringLiteral("映射开始"), "Product must remain idle.");
        POINT point{(bounds.left + bounds.right) / 2, (bounds.top + bounds.bottom) / 2};
        require(ScreenToClient(window, &point), "Cannot locate owned cell.");
        const LPARAM position = MAKELPARAM(point.x, point.y);
        for (UINT message : {WM_LBUTTONDOWN, WM_LBUTTONUP, WM_LBUTTONDBLCLK, WM_LBUTTONUP})
            require(PostMessageW(window, message, message == WM_LBUTTONUP ? 0 : MK_LBUTTON, position), "Cannot post owned GUI cell click.");
    }
    QString desktop() const { return m_desktop; }
private:
    DWORD m_pid;
    HANDLE m_process = nullptr;
    QString m_desktop;
    std::vector<HWND> m_windows;
    ComPtr<IUIAutomation> m_automation;
};
}

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    const auto arguments = app.arguments();
    if (arguments.size() != 6) return 2;
    const QString action = arguments.at(1), executable = arguments.at(4);
    const QFileInfo output(arguments.at(5));
    if ((action != QStringLiteral("row_ibinput") && action != QStringLiteral("preferences_razer_fallback")
         && action != QStringLiteral("return_to_mapping") && action != QStringLiteral("save"))
        || output.exists() || output.suffix() != QStringLiteral("json")
        || output.absolutePath() != QFileInfo(executable).absolutePath()) return 2;
    bool pidOk = false, createdOk = false;
    const DWORD pid = arguments.at(2).toUInt(&pidOk);
    const quint64 created = arguments.at(3).toULongLong(&createdOk);
    if (!pidOk || !createdOk || !pid || !created) return 2;
    const HRESULT initialized = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    if (FAILED(initialized)) return 2;
    QJsonObject result{{QStringLiteral("action"), action}, {QStringLiteral("target_pid"), int(pid)}};
    int code = 0;
    try {
        Target target(pid, created, executable);
        result.insert(QStringLiteral("desktop"), target.desktop());
        require(nameOf(target.find(L"keymapButton").Get()) == QStringLiteral("映射开始"), "Mapping must remain idle.");
        if (action == QStringLiteral("return_to_mapping")) {
            target.closeGeneral();
        } else if (action == QStringLiteral("row_ibinput")) {
            target.openFirstMapping();
            result.insert(QStringLiteral("send_method_label"), target.choose(L"sendMappingKeyMethodComboBox", 3, 4));
            target.escape(target.find(L"sendMappingKeyMethodComboBox").Get());
        } else if (action == QStringLiteral("preferences_razer_fallback")) {
            target.invoke(L"ibInputSettingsButton");
            result.insert(QStringLiteral("backend_label"), target.choose(L"ibInputBackendComboBox", 3, 4));
            result.insert(QStringLiteral("policy_label"), target.choose(L"ibInputPolicyComboBox", 1, 2));
            target.escape(target.find(L"ibInputPolicyComboBox").Get());
        } else {
            target.enterFixedSettingName();
            target.invoke(L"savemaplistButton");
        }
        result.insert(QStringLiteral("ok"), true);
    } catch (const std::exception &error) {
        result.insert(QStringLiteral("ok"), false);
        result.insert(QStringLiteral("error"), QString::fromLocal8Bit(error.what()));
        code = 2;
    }
    QSaveFile file(output.absoluteFilePath());
    if (!file.open(QIODevice::WriteOnly) || file.write(QJsonDocument(result).toJson()) < 0 || !file.commit()) code = 2;
    if (file.error() == QFileDevice::NoError) {
        int attempts = 0;
        while (!QFileInfo::exists(output.absoluteFilePath() + QStringLiteral(".done")) && attempts++ < 1000) QThread::msleep(20);
        if (attempts >= 1000) code = 2;
    }
    CoUninitialize();
    std::cout << "fixed GUI action exit=" << code << '\n';
    return code;
}
