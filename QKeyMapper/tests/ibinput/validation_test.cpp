#include "qkm_ibinput_validation.h"
#include <QCoreApplication>
#include <iostream>
#include <stdexcept>
#include <utility>

namespace {
void check(bool value,const char *message) { if (!value) throw std::runtime_error(message); }
MAP_KEYDATA movement(const QString &original,int method,const QString &mapping=QStringLiteral("Key2Mouse-Right")) {
    MAP_KEYDATA row;
    row.Original_Key=original; row.Pure_MappingKeys=QStringList{mapping};
    row.SendMappingKeyMethod=method; return row;
}
}
int main(int argc,char **argv) {
    QCoreApplication app(argc,argv);
    using namespace QKeyMapperConstants;
    using QkmIbInputValidation::mixedMovementError;
    try {
        check(QkmIbInputValidation::keyboardSourceCode(V_KEYCODE(VK_RETURN,false))==VK_RETURN
            && QkmIbInputValidation::keyboardSourceCode(V_KEYCODE(VK_RETURN,true))!=(VK_RETURN),
            "Physical keypad Enter was collapsed into main Enter as an original key");
        const std::pair<WORD,WORD> keypad[]={{VK_INSERT,VK_NUMPAD0},{VK_END,VK_NUMPAD1},{VK_DOWN,VK_NUMPAD2},
            {VK_NEXT,VK_NUMPAD3},{VK_LEFT,VK_NUMPAD4},{VK_CLEAR,VK_NUMPAD5},{VK_RIGHT,VK_NUMPAD6},
            {VK_HOME,VK_NUMPAD7},{VK_UP,VK_NUMPAD8},{VK_PRIOR,VK_NUMPAD9},{VK_DELETE,VK_DECIMAL}};
        for (const auto &pair:keypad) {
            check(QkmIbInputValidation::keyboardSourceCode(V_KEYCODE(pair.first,false))==pair.second,
                "NumLock-off source alias lost physical identity");
            check(QkmIbInputValidation::keyboardSourceCode(V_KEYCODE(pair.first,true))==pair.first,
                "Main navigation source became a keypad alias");
        }
        check(QkmIbInputValidation::keyboardSourceCode(V_KEYCODE(VK_RSHIFT,true))==VK_RSHIFT,
            "Right modifier source identity changed");
        QList<MAP_KEYDATA> rows;
        QStringList locations;
        for (int i=0;i<8;++i) {
            rows.append(movement(QStringLiteral("trigger-%1").arg(i),
                i<4 ? SENDMAPPINGKEY_METHOD_IBINPUT : SENDMAPPINGKEY_METHOD_SENDINPUT));
            locations.append(QStringLiteral("source-%1").arg(i));
        }
        const QString error=mixedMovementError(rows,locations);
        check(!error.isEmpty() && error.contains(QStringLiteral("source-0")) && error.contains(QStringLiteral("source-7")),
            "Distinct original keys bypassed one shared movement group or lost source locations");
        for (int i=4;i<8;++i) rows[i].Disabled=true;
        check(mixedMovementError(rows,locations).isEmpty(),"Disabled rows blocked the group");
        for (auto &row:rows) { row.Disabled=false; row.SendMappingKeyMethod=SENDMAPPINGKEY_METHOD_IBINPUT; }
        check(mixedMovementError(rows,locations).isEmpty(),"Uniform IbInput group was rejected");
        rows[0].SendMappingKeyMethod=SENDMAPPINGKEY_METHOD_SENDINPUT;
        for (int i=1;i<8;++i) rows[i].SendMappingKeyMethod=SENDMAPPINGKEY_METHOD_FAKERINPUT;
        check(mixedMovementError(rows,locations).isEmpty(),"Non-IbInput legacy combinations changed");
        const QStringList originals{QStringLiteral("F8"),QStringLiteral("Joy-LS2Mouse"),
            QLatin1String(JOY_GYRO2MOUSE_STR),QLatin1String(JOY_TOUCHPAD2MOUSE_STR)};
        for (int group=0;group<4;++group) {
            auto ib=movement(originals.at(group),SENDMAPPINGKEY_METHOD_IBINPUT,
                group==0 ? QStringLiteral("Key2Mouse-Up") : QString());
            auto other=ib; other.SendMappingKeyMethod=SENDMAPPINGKEY_METHOD_SENDINPUT;
            check(QkmIbInputValidation::movementGroup(ib)==group,"Movement classification changed");
            check(!mixedMovementError({ib,other},{}).isEmpty(),"One of the four movement groups bypassed validation");
        }
        auto ib=movement(QStringLiteral("F8"),SENDMAPPINGKEY_METHOD_IBINPUT);
        auto keyUp=movement(QStringLiteral("F9"),SENDMAPPINGKEY_METHOD_SENDINPUT,QString());
        keyUp.Pure_MappingKeys_KeyUp=QStringList{QStringLiteral("Key2Mouse-Left")};
        check(!mixedMovementError({ib,keyUp},{}).isEmpty(),"Key-up-only movement bypassed the shared group");
        auto stick=movement(QStringLiteral("Joy-LS2Mouse"),SENDMAPPINGKEY_METHOD_SENDINPUT,QString());
        check(mixedMovementError({ib,stick},{}).isEmpty(),"Independent movement groups were combined");
        auto keyboard=movement(QStringLiteral("A"),SENDMAPPINGKEY_METHOD_SENDINPUT,QStringLiteral("B"));
        check(mixedMovementError({ib,keyboard},{}).isEmpty(),"Unrelated keyboard mapping was rejected");
        keyboard.SendMappingKeyMethod=SENDMAPPINGKEY_METHOD_IBINPUT;
        check(mixedMovementError({keyboard},{}).isEmpty(),"Runtime capability policy leaked into structural validation");
        std::cout<<"PASS mixed movement validation: sources, disabled, uniform, legacy, four groups and independent outputs.\n"
            <<"No vendor DLL, device initialization, application window or input.\n";
        return 0;
    } catch (const std::exception &error) { std::cerr<<error.what()<<'\n'; return 2; }
}
