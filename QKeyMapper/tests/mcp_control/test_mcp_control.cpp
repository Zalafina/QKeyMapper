#include "../../automation/qkm_control_dialog.h"
#include "../../qkeymapper_constants.h"

#include <QApplication>
#include <QCheckBox>
#include <QDir>
#include <QFileInfo>
#include <QSaveFile>
#include <QJsonDocument>
#include <QLabel>
#include <QListWidget>
#include <QLineEdit>
#include <QProcess>
#include <QPushButton>
#include <QStyleOptionButton>
#include <QTemporaryDir>
#include <QTimer>
#include <QtTest/QTest>

using namespace QkmAutomation;

namespace {

class Client : public QProcess {
public:
    bool launch(AutomationSessionServer &server, const QString &directory, const QString &mode)
    {
        const auto own = queryProcessIdentity(GetCurrentProcessId());
        if (!own.ok()) return false;
        start(qEnvironmentVariable("QKM_IPC_TEST_EXE"), {QStringLiteral("--session-client"), server.pipeName(),
            server.instanceId(), QString::number(own.identity.processId), QString::number(own.identity.creationTime), directory, mode});
        return waitForStarted(3000);
    }
    ~Client() override
    {
        if (state() != NotRunning) { kill(); waitForFinished(3000); }
    }
};

template<class T> T *widget(ControlDialog &dialog, const char *name)
{
    T *result = dialog.findChild<T *>(QLatin1String(name));
    Q_ASSERT(result);
    return result;
}

void fixture(AutomationSessionServer &server)
{
    server.setRequestHandler([&server](const AutomationRequest &request) {
        server.reply(request.connectionId, request.requestId, {{QStringLiteral("fixture"), true}});
    });
}

void clickCheckBox(ControlDialog &dialog, const char *name)
{
    auto *box = widget<QCheckBox>(dialog, name);
    QStyleOptionButton option;
    option.initFrom(box);
    option.text = box->text();
    const QRect indicator = box->style()->subElementRect(QStyle::SE_CheckBoxIndicator, &option, box);
    QTest::mouseClick(box, Qt::LeftButton, Qt::NoModifier, indicator.center());
}

} // namespace

class McpControlTest : public QObject {
    Q_OBJECT
private slots:
    void initTestCase();
    void defaultOffAndLanguage_data();
    void defaultOffAndLanguage();
    void pairingFromWidgets_data();
    void pairingFromWidgets();
    void rejectAndClose_data();
    void rejectAndClose();
    void enableFailure();
};

void McpControlTest::initTestCase()
{
    wchar_t desktop[256]{};
    DWORD needed = 0;
    QVERIFY(GetUserObjectInformationW(GetThreadDesktop(GetCurrentThreadId()), UOI_NAME, desktop, sizeof(desktop), &needed));
    const QString actualDesktop = QString::fromWCharArray(desktop);
    qInfo().noquote() << "Actual desktop:" << actualDesktop;
    if (!qEnvironmentVariableIsEmpty("QKM_TEST_DESKTOP")) QCOMPARE(actualDesktop, qEnvironmentVariable("QKM_TEST_DESKTOP"));
    QVERIFY2(QFileInfo::exists(qEnvironmentVariable("QKM_IPC_TEST_EXE")), "Set QKM_IPC_TEST_EXE to the matching built IPC test executable.");
    const auto own = queryProcessIdentity(GetCurrentProcessId());
    QVERIFY(own.ok());
    QCOMPARE(QFileInfo(queryProcessExecutable(own.identity)).canonicalFilePath(), QFileInfo(QCoreApplication::applicationFilePath()).canonicalFilePath());
    ProcessIdentity changed = own.identity;
    ++changed.creationTime;
    QVERIFY(queryProcessExecutable(changed).isEmpty());
    QVERIFY(queryProcessExecutable({}).isEmpty());
}

void McpControlTest::defaultOffAndLanguage_data()
{
    QTest::addColumn<int>("language");
    QTest::newRow("Chinese") << 0;
    QTest::newRow("English") << 1;
    QTest::newRow("Japanese") << 2;
}

void McpControlTest::defaultOffAndLanguage()
{
    QFETCH(int, language);
    QTemporaryDir root(QDir::currentPath() + QStringLiteral("/control-XXXXXX"));
    QVERIFY(root.isValid());
    ControlDialog dialog(root.path() + QStringLiteral("/private"), "test-control");
    dialog.setUILanguage(language);
    QVERIFY(!dialog.server().isEnabled());
    QVERIFY(!QFileInfo::exists(root.path() + QStringLiteral("/private")));
    QCOMPARE(dialog.windowTitle(), ControlDialog::title(language));
    QVERIFY(!widget<QCheckBox>(dialog, "mcpEnabled")->isChecked());
    QVERIFY(!widget<QPushButton>(dialog, "mcpApprove")->autoDefault());
    QVERIFY(!widget<QPushButton>(dialog, "mcpApprove")->isDefault());
    const auto labels = dialog.findChildren<QLabel *>();
    for (const QLabel *label : labels) QCOMPARE(label->textFormat(), Qt::PlainText);
}

void McpControlTest::pairingFromWidgets_data()
{
    QTest::addColumn<bool>("writable");
    QTest::newRow("read-only") << false;
    QTest::newRow("configure-run") << true;
}

void McpControlTest::pairingFromWidgets()
{
    QFETCH(bool, writable);
    QTemporaryDir root(QDir::currentPath() + QStringLiteral("/control-XXXXXX"));
    QVERIFY(root.isValid());
    ControlDialog dialog(root.path() + QStringLiteral("/server"), "test-control");
    dialog.show();
    QVERIFY(QTest::qWaitForWindowExposed(&dialog));
    fixture(dialog.server());
    clickCheckBox(dialog, "mcpEnabled");
    QVERIFY(dialog.server().isEnabled());
    QVERIFY(QFileInfo::exists(dialog.server().discoveryPath()));
    InstanceDescriptor descriptor;
    DiscoveryError discoveryError;
    QVERIFY(readInstanceDiscovery(QFileInfo(dialog.server().discoveryPath()).absolutePath(),
        QCoreApplication::applicationFilePath(), descriptor, discoveryError));
    QCOMPARE(descriptor.processId, quint32(GetCurrentProcessId()));
    Client child;
    QVERIFY(child.launch(dialog.server(), root.path() + QStringLiteral("/client"), QStringLiteral("pair")));
    QTRY_VERIFY_WITH_TIMEOUT(!widget<QLabel>(dialog, "mcpPairRequest")->text().isEmpty(), 3000);
    QVERIFY(widget<QLabel>(dialog, "mcpPairRequest")->text().contains(QString::number(child.processId())));
    const auto *executable = widget<QLineEdit>(dialog, "mcpClientExecutable");
    QVERIFY(executable->isReadOnly());
    QCOMPARE(QFileInfo(executable->text()).canonicalFilePath(), QFileInfo(qEnvironmentVariable("QKM_IPC_TEST_EXE")).canonicalFilePath());
    QVERIFY(!widget<QCheckBox>(dialog, "mcpConfigure")->isChecked());
    QVERIFY(!widget<QCheckBox>(dialog, "mcpRunMapping")->isChecked());
    if (writable) {
        clickCheckBox(dialog, "mcpConfigure");
        clickCheckBox(dialog, "mcpRunMapping");
    }
    QTest::mouseClick(widget<QPushButton>(dialog, "mcpApprove"), Qt::LeftButton);
    QVERIFY(!widget<QPushButton>(dialog, "mcpApprove")->isEnabled());
    QTRY_COMPARE_WITH_TIMEOUT(child.state(), QProcess::NotRunning, 3000);
    QCOMPARE(child.exitCode(), 0);
    QTRY_COMPARE(widget<QListWidget>(dialog, "mcpPairings")->count(), 1);
    const auto pairs = dialog.server().pairings();
    QCOMPARE(pairs.at(0).scopes, quint32(writable ? Read | Configure | RunMapping : Read));
    QVERIFY(dialog.close());
    QVERIFY(dialog.server().isEnabled());
    auto *list = widget<QListWidget>(dialog, "mcpPairings");
    list->setCurrentRow(0);
    QTest::mouseClick(widget<QPushButton>(dialog, "mcpRevoke"), Qt::LeftButton);
    QVERIFY(dialog.server().pairings().isEmpty());
    QCOMPARE(list->count(), 0);
    clickCheckBox(dialog, "mcpEnabled");
    QVERIFY(!dialog.server().isEnabled());
    QVERIFY(!QFileInfo::exists(dialog.server().discoveryPath()));
}

void McpControlTest::rejectAndClose_data()
{
    QTest::addColumn<int>("closeMethod");
    QTest::newRow("reject-button") << 0;
    QTest::newRow("close-window") << 1;
    QTest::newRow("escape") << 2;
}

void McpControlTest::rejectAndClose()
{
    QFETCH(int, closeMethod);
    QTemporaryDir root(QDir::currentPath() + QStringLiteral("/control-XXXXXX"));
    QVERIFY(root.isValid());
    ControlDialog dialog(root.path() + QStringLiteral("/server"), "test-control");
    dialog.show();
    QVERIFY(QTest::qWaitForWindowExposed(&dialog));
    clickCheckBox(dialog, "mcpEnabled");
    Client child;
    QVERIFY(child.launch(dialog.server(), root.path() + QStringLiteral("/client"), QStringLiteral("pair-rejected")));
    QTRY_VERIFY_WITH_TIMEOUT(!widget<QLabel>(dialog, "mcpPairRequest")->text().isEmpty(), 3000);
    if (closeMethod == 1) QVERIFY(dialog.close());
    else if (closeMethod == 2) QTest::keyClick(&dialog, Qt::Key_Escape);
    else QTest::mouseClick(widget<QPushButton>(dialog, "mcpReject"), Qt::LeftButton);
    QVERIFY(widget<QLabel>(dialog, "mcpPairRequest")->text().isEmpty());
    QTRY_COMPARE_WITH_TIMEOUT(child.state(), QProcess::NotRunning, 3000);
    QCOMPARE(child.exitCode(), 0);
    QVERIFY(dialog.server().isEnabled());
    QVERIFY(dialog.server().pairings().isEmpty());
    QVERIFY(widget<QLabel>(dialog, "mcpPairRequest")->text().isEmpty());
}

void McpControlTest::enableFailure()
{
    ControlDialog dialog(QString(), "test-control");
    clickCheckBox(dialog, "mcpEnabled");
    QVERIFY(!dialog.server().isEnabled());
    QVERIFY(!widget<QCheckBox>(dialog, "mcpEnabled")->isChecked());
    QVERIFY(widget<QLabel>(dialog, "mcpStatus")->text().contains(QStringLiteral("失败")));
}

int main(int argc, char **argv)
{
    // Match the product's baseline DPI, style and font without loading a personal INI.
    qputenv("QT_ENABLE_HIGHDPI_SCALING", "1");
    QGuiApplication::setHighDpiScaleFactorRoundingPolicy(Qt::HighDpiScaleFactorRoundingPolicy::PassThrough);
    QApplication app(argc, argv);
    QApplication::setStyle(QStringLiteral("Fusion"));
    QApplication::setFont(QFont(QString::fromLatin1(QKeyMapperConstants::FONTNAME_ENGLISH), 9));
    app.setQuitOnLastWindowClosed(false);
    if (app.arguments().value(1) == QStringLiteral("--preview") && app.arguments().size() == 5) {
        QTemporaryDir root(QDir::currentPath() + QStringLiteral("/preview-XXXXXX"));
        if (!root.isValid()) return 20;
        ControlDialog dialog(root.path() + QStringLiteral("/server"), "test-preview");
        dialog.setUILanguage(app.arguments().at(2).toInt());
        if (app.arguments().at(3) == QStringLiteral("pending")) {
            const auto own = queryProcessIdentity(GetCurrentProcessId());
            if (!own.ok()) return 21;
            // A UI-only fixture deliberately includes markup; no client is approved or authenticated.
            emit dialog.server().pairingRequested({QStringLiteral("preview"), QStringLiteral("preview"),
                QStringLiteral("<b>测试客户端</b> 日本語"), own.identity});
        }
        dialog.show();
        QObject::connect(&dialog, &QDialog::finished, &app, &QCoreApplication::quit);
        QTimer::singleShot(300, &dialog, [&]() {
            wchar_t desktop[256]{};
            DWORD needed = 0;
            if (!GetUserObjectInformationW(GetThreadDesktop(GetCurrentThreadId()), UOI_NAME, desktop, sizeof(desktop), &needed)) { app.exit(22); return; }
            const QJsonObject state{{QStringLiteral("pid"), int(GetCurrentProcessId())},
                {QStringLiteral("hwnd"), QString::number(quintptr(dialog.winId()))},
                {QStringLiteral("qt_dpr"), dialog.devicePixelRatioF()},
                {QStringLiteral("qt_logical_dpi"), dialog.logicalDpiX()},
                {QStringLiteral("native_dpi"), int(GetDpiForWindow(reinterpret_cast<HWND>(dialog.winId())))},
                {QStringLiteral("native_visible"), bool(IsWindowVisible(reinterpret_cast<HWND>(dialog.winId())))},
                {QStringLiteral("style"), dialog.style()->objectName()},
                {QStringLiteral("font"), dialog.font().family()},
                {QStringLiteral("font_points"), dialog.font().pointSizeF()},
                {QStringLiteral("desktop"), QString::fromWCharArray(desktop)}};
            QSaveFile output(app.arguments().at(4));
            output.setDirectWriteFallback(false);
            const QByteArray bytes = QJsonDocument(state).toJson();
            if (!output.open(QIODevice::WriteOnly) || output.write(bytes) != bytes.size() || !output.commit()) app.exit(23);
        });
        QTimer::singleShot(20000, &app, &QCoreApplication::quit);
        return app.exec();
    }
    McpControlTest test;
    return QTest::qExec(&test, argc, argv);
}

#include "test_mcp_control.moc"
