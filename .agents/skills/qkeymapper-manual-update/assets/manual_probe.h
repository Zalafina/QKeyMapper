#pragma once
#include "qkeymapper.h"
#include <QApplication>
#include <QAbstractButton>
#include <QComboBox>
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QLineEdit>
#include <QSpinBox>
#include <QTabWidget>
#include <QTimer>
#include <QWindow>
#include <QScreen>
#include <QContextMenuEvent>
#include <QDialogButtonBox>
#include <QMessageBox>
#include <QMenuBar>
#include <QSettings>
#include <QLabel>
#include <QListWidget>
#include <QRegularExpression>
#include <QHeaderView>
#include <QCheckBox>
#include "qignorewindowinfolistdialog.h"
#include <windows.h>

namespace ManualProbe {
inline QString directory() { return qEnvironmentVariable("QKM_MANUAL_SESSION"); }
inline QJsonObject state(QWidget *w) {
    const HWND hwnd=reinterpret_cast<HWND>(w->winId());
    wchar_t desk[256]={};DWORD needed=0;
    GetUserObjectInformationW(GetThreadDesktop(GetWindowThreadProcessId(hwnd,nullptr)),UOI_NAME,desk,sizeof(desk),&needed);
    return {{"hwnd",QString::number(quintptr(hwnd))},{"pid",int(GetCurrentProcessId())},
        {"desktop",QString::fromWCharArray(desk)},{"visible",w->isVisible()},
        {"nativeVisible",bool(IsWindowVisible(hwnd))},{"width",w->width()},{"height",w->height()},
        {"dpr",w->devicePixelRatioF()},{"screen",w->screen()->name()},{"screenWidth",w->screen()->geometry().width()},{"screenHeight",w->screen()->geometry().height()},{"title",w->windowTitle()},
        {"x",w->frameGeometry().x()},{"y",w->frameGeometry().y()}};
}
class Controller final:public QObject {
    QKeyMapper *main;
    QTimer timer;
    QWidget demo;
    QJsonArray regexResults;
    QJsonArray mappingResults;
    bool busy=false;
    QWidget *root(const QJsonObject &c) {
        const QString name=c["root"].toString();
        if(name.isEmpty() || name=="main")return main;
        if(name=="editor")return QItemSetupDialog::getInstance();
        if(name=="rules")return QIgnoreWindowInfoListDialog::getInstance();
        if(name=="dialog")return QApplication::activeModalWidget();
        if(name=="popup")return QApplication::activePopupWidget();
        if(auto *named=main->findChild<QWidget *>(name))return named;
        QWidget *found=nullptr;
        for(auto *w:QApplication::topLevelWidgets())if(w->isVisible() && name==w->metaObject()->className()){
            if(found)return nullptr;
            found=w;
        }
        return found;
    }
    void record(const QString &id,QString error={}) {
        QJsonArray rows;
        const auto *data=QKeyMapper::KeyMappingDataList;
        auto *table=QKeyMapper::s_KeyMappingTabInfoList.at(QKeyMapper::s_KeyMappingTabWidgetCurrentIndex).KeyMappingDataTable;
        if(data)for(int i=0;i<data->size();++i){const auto &row=data->at(i);rows.append(QJsonObject{{"original",row.Original_Key},{"mapping",row.Mapping_Keys.join(" + ")},{"category",row.Category},{"disabled",row.Disabled},{"visible",!table->isRowHidden(i)},{"selected",table->item(i,0)->isSelected()},{"burst",row.Burst},{"lock",row.Lock}});}
        QJsonObject roots{{"main",state(main)},{"demo",state(&demo)}};
        if(auto *e=QItemSetupDialog::getInstance())roots["editor"]=state(e);
        QJsonArray tabs;
        for(const auto &tab:QKeyMapper::s_KeyMappingTabInfoList)tabs.append(QJsonObject{{"name",tab.TabName},{"rows",tab.KeyMappingData?tab.KeyMappingData->size():0}});
        QJsonArray widgets;
        for(auto *w:QApplication::topLevelWidgets())if(w->isVisible()){
            widgets.append(QJsonObject{{"class",w->metaObject()->className()},{"name",w->objectName()},{"title",w->windowTitle()}});
            if(qobject_cast<QDialog *>(w)){roots["dialog"]=state(w);roots[w->metaObject()->className()]=state(w);}
            if(auto *menu=qobject_cast<QMenu *>(w)){roots["popup"]=state(w);for(auto *a:menu->actions())if(a->text()==QString::fromUtf8("输入分类"))roots["category-context"]=state(w);}
            if(w!=main && !qobject_cast<QDialog *>(w) && !qobject_cast<QMenu *>(w) && w->windowType()==Qt::Popup)roots["popup"]=state(w);
        }
        if(auto *popup=QApplication::activePopupWidget())roots["popup"]=state(popup);
        if(auto *rules=QIgnoreWindowInfoListDialog::getInstance())roots["rules"]=state(rules);
        QJsonArray processes;auto *processTable=main->findChild<QTableWidget *>("processinfoTable");
        for(int i=0;i<processTable->rowCount();++i){QJsonArray cells;for(int j=0;j<processTable->columnCount();++j)cells.append(processTable->item(i,j)->text());processes.append(cells);}
        QJsonObject fields;for(auto *line:main->findChildren<QLineEdit *>())fields[line->objectName()]=line->text();
        QJsonObject labels;
        for(auto *w:main->findChildren<QWidget *>()) {
            if(auto *b=qobject_cast<QAbstractButton *>(w))labels[w->objectName()]=b->text();
            if(auto *c=qobject_cast<QComboBox *>(w)) {QJsonArray options;for(int i=0;i<c->count();++i)options.append(c->itemText(i));labels[w->objectName()]=options;}
        }
        QJsonObject result{{"id",id},{"roots",roots},{"extra",QJsonObject{{"error",error},{"rows",rows},{"labels",labels},{"tabs",tabs},{"widgets",widgets},{"processes",processes},{"fields",fields},{"regexResults",regexResults},{"mappingResults",mappingResults}}}};
        QFile f(directory()+"/response_"+id+".json");if(!f.open(QIODevice::WriteOnly))qFatal("Cannot write manual observer state");f.write(QJsonDocument(result).toJson());
    }
    void command() {
        // Keep non-demo process titles out of captures even after a refresh.
        if(auto *table=main->findChild<QTableWidget *>("processinfoTable"))
            for(int row=0;row<table->rowCount();++row)
                table->setRowHidden(row, !table->item(row,2) || table->item(row,2)->text()!=demo.windowTitle());
        if(busy)return;
        QFile f(directory()+"/request.json");if(!f.open(QIODevice::ReadOnly))return;
        const auto c=QJsonDocument::fromJson(f.readAll()).object();f.close();f.remove();
        const QString id=c["id"].toString(),action=c["action"].toString();QString error;
        if(action=="quit") {if(auto *e=QItemSetupDialog::getInstance())e->hide();record(id);QTimer::singleShot(200,qApp,&QCoreApplication::quit);return;}
        QWidget *r=root(c);const QString name=c["name"].toString();
        if(!r){record(id,"Requested root is missing or ambiguous");return;}
        // The observer is for editing/capture, never mapping, drivers or OS settings.
        const QRegularExpression unsafe(QStringLiteral("install|uninstall|multiinputenable|virtualgamepad|virtualjoystick|autostart|filterkey|keymapbutton|lockcursor"), QRegularExpression::CaseInsensitiveOption);
        if ((action=="click" || action=="set") && unsafe.match(name).hasMatch()) {record(id,"Control is outside UI-only scope");return;}
        if (action=="click" && r->metaObject()->className()==QByteArray("QInputDeviceListWindow") && !name.contains("cancel",Qt::CaseInsensitive)) {record(id,"Device list is view/cancel only");return;}
        if (action=="menu-item") {record(id,"Inspect menus only; activation requires a scoped helper adaptation");return;}
        if (action=="transfer-path" || action=="table-export" || action=="table-import") {
            const QString path=QDir::cleanPath(QFileInfo(c["text"].toString()).absoluteFilePath());
            if(!path.startsWith(QDir(directory()).absolutePath()+"/",Qt::CaseInsensitive)){record(id,"File operations must stay in the dedicated session");return;}
        }

        if(action=="validate-mapping") {
            mappingResults={};for(const auto &v:c["cases"].toArray()){const auto sample=v.toObject();QString text=sample["text"].toString();const auto result=QKeyMapper::validateMappingKeyString(text);mappingResults.append(QJsonObject{{"text",sample["text"]},{"valid",result.isValid},{"error",result.errorMessage},{"expected",sample["expected"]}});if(result.isValid!=sample["expected"].toBool())error="Mapping sample mismatch";}
        } else if(action=="point") {main->showPointPickerDialog(true);}
        else if(action=="category-view") {main->setCategoryColumnVisible(true);}
        else if(action=="category-layout") {main->setProcessListVisible(false);QTimer::singleShot(30,this,[this]{auto *table=QKeyMapper::s_KeyMappingTabInfoList.at(QKeyMapper::s_KeyMappingTabWidgetCurrentIndex).KeyMappingDataTable;table->horizontalHeader()->setStretchLastSection(false);table->horizontalHeader()->setSectionResizeMode(1,QHeaderView::Interactive);table->setColumnWidth(0,100);table->setColumnWidth(1,220);table->setColumnWidth(6,140);});}
        else if(action=="hide-disabled") {auto *table=QKeyMapper::s_KeyMappingTabInfoList.at(QKeyMapper::s_KeyMappingTabWidgetCurrentIndex).KeyMappingDataTable;table->setHideDisabledFilter(c["checked"].toBool());}
        else if(action=="category-header") {auto *table=QKeyMapper::s_KeyMappingTabInfoList.at(QKeyMapper::s_KeyMappingTabWidgetCurrentIndex).KeyMappingDataTable;QMetaObject::invokeMethod(table->horizontalHeader(),"sectionPressed",Qt::DirectConnection,Q_ARG(int,6));}
        else if(action=="submenu" || action=="menu-item") {
            auto *menu=qobject_cast<QMenu *>(QApplication::activePopupWidget());QAction *found=nullptr;
            if(menu)for(auto *a:menu->actions())if(a->text().remove('&')==c["text"].toString())found=a;
            if(!found)error="Menu item missing: "+c["text"].toString();
            else if(action=="submenu" && found->menu()){menu->setActiveAction(found);found->menu()->popup(menu->mapToGlobal(menu->actionGeometry(found).topRight()));}
            else if(action=="menu-item"){found->trigger();menu->close();}
            else error="Submenu missing";
        } else if(action=="category-input") {
            auto *popup=QApplication::activePopupWidget();auto *line=popup?popup->findChild<QLineEdit *>():nullptr;
            if(line)line->setText(c["text"].toString());else error="Category input missing";
        } else if(action=="category-apply") {
            auto *popup=QApplication::activePopupWidget();QPushButton *apply=nullptr;
            if(popup)for(auto *b:popup->findChildren<QPushButton *>())if(b->text()==QString::fromUtf8("应用"))apply=b;
            if(apply){if(c["requireDisabled"].toBool() && apply->isEnabled())error="Empty category apply unexpectedly enabled";else if(!c["requireDisabled"].toBool())apply->click();}else error="Category apply missing";
        } else if(action=="check-label") {
            auto *popup=QApplication::activePopupWidget();QCheckBox *found=nullptr;
            if(popup)for(auto *b:popup->findChildren<QCheckBox *>())if(b->text()==c["text"].toString())found=b;
            if(found)found->setChecked(c["checked"].toBool());else error="Checkbox missing: "+c["text"].toString();
        } else if(action=="hide") {r->hide();}
        else if(action=="tabs") {auto *tabs=r->findChild<QTabWidget *>(c["name"].toString());if(tabs)tabs->setCurrentIndex(c["index"].toInt());else error="Missing tabs";}
        else if(action=="process-select") {
            auto *table=main->findChild<QTableWidget *>("processinfoTable");int row=-1;
            for(int i=0;i<table->rowCount();++i)if(table->item(i,2)->text()==demo.windowTitle())row=i;
            if(row<0)error="Controlled demo missing from real process enumeration";
            else{table->selectRow(row);QMetaObject::invokeMethod(main,"on_processinfoTable_doubleClicked",Qt::DirectConnection,Q_ARG(QModelIndex,table->model()->index(row,0)));}
        } else if(action=="regex-check") {
            regexResults={};for(const auto &v:c["cases"].toArray()){const auto item=v.toObject();QRegularExpression re(item["pattern"].toString());const bool actual=re.isValid() && re.match(item["text"].toString()).hasMatch();regexResults.append(QJsonObject{{"pattern",item["pattern"]},{"text",item["text"]},{"valid",re.isValid()},{"actual",actual},{"expected",item["expected"]}});if(actual!=item["expected"].toBool())error="Regex sample mismatch";}
        } else if(action=="rule-select") {
            auto *rules=QIgnoreWindowInfoListDialog::getInstance();auto *list=rules->findChild<QListWidget *>("ruleListWidget");auto items=list->findItems(c["text"].toString(),Qt::MatchExactly);
            if(items.isEmpty())error="Rule not found";else{list->setCurrentItem(items.first());QMetaObject::invokeMethod(rules,"on_ruleListWidget_itemDoubleClicked",Qt::DirectConnection,Q_ARG(QListWidgetItem *,items.first()));}
        } else if(action=="hide-rules") {QIgnoreWindowInfoListDialog::getInstance()->hide();}
        else if(action=="stage-data") {
            auto *data=QKeyMapper::KeyMappingDataList;
            for(int row=0;data && row<data->size();++row){(*data)[row].Category=row<2?QString::fromUtf8("基础练习"):QString::fromUtf8("常用操作");}
            main->setCategoryColumnVisible(true);main->setShowNotes(true);
            main->refreshKeyMappingDataTableByTabIndex(QKeyMapper::s_KeyMappingTabWidgetCurrentIndex);
        } else if(action=="select") {
            auto *table=QKeyMapper::s_KeyMappingTabInfoList.at(QKeyMapper::s_KeyMappingTabWidgetCurrentIndex).KeyMappingDataTable;
            const int first=c["first"].toInt(),last=c.contains("last")?c["last"].toInt():first;
            if(first<0 || last<first || last>=table->rowCount() || c["column"].toInt()<0 || c["column"].toInt()>=table->columnCount()){record(id,"Selection outside table");return;}
            table->clearSelection();
            table->setRangeSelected(QTableWidgetSelectionRange(first,0,last,table->columnCount()-1),true);table->setCurrentCell(first,c["column"].toInt(),QItemSelectionModel::NoUpdate);
        } else if(action=="copy") {main->copySelectedKeyMappingDataToCopiedList();}
        else if(action=="paste") {main->insertKeyMappingDataFromCopiedList(QKeyMapper::getTableInsertModeIndex());}
        else if(action=="move-down") {main->selectedItemsMoveDown();}
        else if(action=="delete-rows") {main->deleteSelectedMappingData();}
        else if(action=="new-tab") {main->addTabToKeyMappingTabWidget(c["text"].toString(),true);}
        else if(action=="mapping-tab") {main->findChild<KeyMappingTabWidget *>()->setCurrentIndex(c["index"].toInt());}
        else if(action=="setup") {QMetaObject::invokeMethod(main,"openCurrentMappingTableSetupDialog",Qt::DirectConnection);}
        else if(action=="filter") {auto *table=QKeyMapper::s_KeyMappingTabInfoList.at(QKeyMapper::s_KeyMappingTabWidgetCurrentIndex).KeyMappingDataTable;QSet<QString> categories;if(!c["text"].toString().isEmpty())categories.insert(c["text"].toString());table->setCategoryFilters(categories);}
        else if(action=="operations") {main->menuBar()->actions().first()->menu()->popup(main->mapToGlobal(QPoint(15,25)));}
        else if(action=="context") {QTimer::singleShot(0,this,[this]{auto *table=QKeyMapper::s_KeyMappingTabInfoList.at(QKeyMapper::s_KeyMappingTabWidgetCurrentIndex).KeyMappingDataTable;QPoint pos=table->visualItemRect(table->currentItem()).center();QContextMenuEvent event(QContextMenuEvent::Mouse,pos,table->viewport()->mapToGlobal(pos));QApplication::sendEvent(table->viewport(),&event);});}
        else if(action=="close-popup") {if(auto *popup=QApplication::activePopupWidget())popup->close();}
        else if(action=="setting-popup") {main->findChild<QComboBox *>("settingselectComboBox")->showPopup();}
        else if(action=="activate-setting") {auto *combo=main->findChild<QComboBox *>("settingselectComboBox");const int index=c["index"].toInt();combo->setCurrentIndex(index);QMetaObject::invokeMethod(main,"on_settingselectComboBox_textActivated",Qt::DirectConnection,Q_ARG(QString,combo->currentText()));}
        else if(action=="transfer") {const QString text=c["text"].toString();QTimer::singleShot(0,this,[this,text]{QMetaObject::invokeMethod(main,"settingBackupActionTriggered",Qt::DirectConnection,Q_ARG(QString,text));});}
        else if(action=="transfer-path") {
            auto *dlg=qobject_cast<SettingTransferDialog *>(QApplication::activeModalWidget());
            if(!dlg)error="No transfer dialog";
            else{
                const QString path=c["text"].toString();dlg->findChild<QLineEdit *>()->setText(path);
                if(c["load"].toBool()){
                    auto *groups=dlg->findChild<GroupSelectionWidget *>();const auto names=SettingTransferDialog::readGroupsFromIni(path);groups->setGroups(names,path);
                    QSettings current("keymapdata.ini",QSettings::IniFormat);QSet<QString> duplicates;QStringList selected;
                    for(const auto &name:names)if(current.childGroups().contains(name)||name=="General")duplicates.insert(name);else selected.append(name);
                    groups->setSelectedGroups(selected);groups->highlightDuplicates(duplicates,QColor(255,0,0));
                }
            }
        } else if(action=="transfer-select") {
            auto *dlg=qobject_cast<SettingTransferDialog *>(QApplication::activeModalWidget());QStringList selected;for(const auto &v:c["groups"].toArray())selected.append(v.toString());if(dlg)dlg->findChild<GroupSelectionWidget *>()->setSelectedGroups(selected);else error="No transfer dialog";
        } else if(action=="dialog-response") {
            const QString response=c["text"].toString();QTimer::singleShot(0,this,[response]{auto *dlg=QApplication::activeModalWidget();if(auto *box=qobject_cast<QMessageBox *>(dlg)){auto button=response=="yes"?QMessageBox::Yes:response=="no"?QMessageBox::No:QMessageBox::Ok;if(auto *b=box->button(button))b->click();}else if(auto *dialog=qobject_cast<QDialog *>(dlg)){if(response=="cancel")dialog->reject();else if(auto *buttons=dialog->findChild<QDialogButtonBox *>())buttons->button(QDialogButtonBox::Ok)->click();}});
        } else if(action=="table-export") {if(!QKeyMapper::exportKeyMappingDataToFile(QKeyMapper::s_KeyMappingTabWidgetCurrentIndex,c["text"].toString()))error="Table export failed";}
        else if(action=="table-import") {if(!QKeyMapper::importKeyMappingDataFromFile(QKeyMapper::s_KeyMappingTabWidgetCurrentIndex,c["text"].toString()))error="Table import failed";main->refreshKeyMappingDataTableByTabIndex(QKeyMapper::s_KeyMappingTabWidgetCurrentIndex);}
        else if(action=="set") {
            QWidget *w=r->findChild<QWidget *>(name);
            if(auto *combo=qobject_cast<QComboBox *>(w)) {const int i=combo->findText(c["text"].toString());if(i<0)error="Missing combo option: "+name;else combo->setCurrentIndex(i);}
            else if(auto *line=qobject_cast<QLineEdit *>(w))line->setText(c["text"].toString());
            else if(auto *button=qobject_cast<QAbstractButton *>(w))button->setChecked(c["checked"].toBool());
            else if(auto *spin=qobject_cast<QSpinBox *>(w))spin->setValue(c["value"].toInt());
            else error="Unsupported control: "+name;
        } else if(action=="click") {
            auto *button=r->findChild<QAbstractButton *>(name);
            if(button)QTimer::singleShot(0,button,[button]{button->click();});else error="Missing button: "+name;
        } else if(action=="presentation") {
            auto *table=main->findChild<QTableWidget *>("processinfoTable");
            if(table){table->setColumnWidth(0,140);table->setColumnWidth(1,52);table->setColumnWidth(2,235);table->setColumnWidth(3,110);for(int i=0;i<table->rowCount();++i)table->setRowHidden(i,true);}
        } else if(action=="tab") {
            auto *tabs=main->findChild<QTabWidget *>("settingTabWidget");
            if(tabs)tabs->setCurrentIndex(c["index"].toInt());else error="Missing settings tabs";
        } else if(action=="editor") {
            auto *t=QKeyMapper::s_KeyMappingTabInfoList.at(QKeyMapper::s_KeyMappingTabWidgetCurrentIndex).KeyMappingDataTable;
            if(t->rowCount()==0){record(id,"No mapping item to edit");return;}
            t->selectionModel()->select(QItemSelection(t->model()->index(0,0),t->model()->index(0,t->columnCount()-1)),QItemSelectionModel::ClearAndSelect|QItemSelectionModel::Rows);
            t->setCurrentCell(0,0,QItemSelectionModel::NoUpdate);
            if(!QMetaObject::invokeMethod(main,"openCurrentMappingItemSetupDialog",Qt::DirectConnection))error="Editor opener unavailable";
        } else if(action=="hide-editor")QItemSetupDialog::getInstance()->hide();
        else if(action=="menu") {auto *button=main->findChild<QToolButton *>("keymapButton");if(button && button->menu())button->menu()->popup(button->mapToGlobal(QPoint(0,0)));}
        else if(action!="state")error="Unknown observer action: "+action;
        busy=true;QTimer::singleShot(250,this,[this,id,error]{record(id,error);busy=false;});
    }
public:
    explicit Controller(QKeyMapper *w):QObject(w),main(w) {
        main->move(180,100);
        demo.setWindowTitle(QString::fromUtf8("星际练习场（演示）"));demo.setWindowIcon(main->windowIcon());demo.resize(440,180);demo.move(1420,160);
        auto *label=new QLabel(QString::fromUtf8("受控演示窗口：仅用于读取窗口信息，不运行游戏。"),&demo);label->setGeometry(18,35,410,70);label->setWordWrap(true);demo.show();
        connect(&timer,&QTimer::timeout,this,[this]{command();});timer.start(40);
        QTimer::singleShot(1400,this,[this]{record("ready");});
        QTimer::singleShot(3600000,qApp,&QCoreApplication::quit);
    }
};
inline void install(QKeyMapper *w){new Controller(w);}
}
