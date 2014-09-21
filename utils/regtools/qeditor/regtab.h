#ifndef REGTAB_H
#define REGTAB_H

#include <QComboBox>
#include <QTreeWidget>
#include <QVBoxLayout>
#include <QTabWidget>
#include <QSplitter>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QListWidget>
#include <QGroupBox>
#include <QToolButton>
#include <QMenu>
#include <QCheckBox>
#include "backend.h"
#include "settings.h"
#include "mainwindow.h"
#include "utils.h"

class RegTabPanel
{
public:
    RegTabPanel() {}
    virtual ~RegTabPanel() {}
    virtual void AllowWrite(bool en) = 0;
    virtual QWidget *GetWidget() = 0;
};

class EmptyRegTabPanel : public QWidget, public RegTabPanel
{
public:
    EmptyRegTabPanel(QWidget *parent = 0);
    void AllowWrite(bool en);
    QWidget *GetWidget();
};

class RegTab : public QSplitter, public DocumentTab
{
    Q_OBJECT
public:
    RegTab(Backend *backend, QWidget *parent = 0);
    ~RegTab();
    virtual bool Quit();
    virtual QWidget *GetWidget();

protected:
    void FillDevSubTree(QTreeWidgetItem *item);
    void FillSocSubTree(QTreeWidgetItem *item);
    void FillRegTree();
    void FillAnalyserList();
    void UpdateSocList();
    void DisplayRegister(const SocRegRef& ref);
    void DisplayDevice(const SocDevRef& ref);
    void DisplaySoc(const SocRef& ref);
    void SetDataSocName(const QString& socname);
    void SetPanel(RegTabPanel *panel);
    void UpdateSocFilename();
    void UpdateTabName();

    QComboBox *m_soc_selector;
    BackendSelector *m_backend_selector;
    Backend *m_backend;
    QTreeWidget *m_reg_tree;
    SocRef m_cur_soc;
    QVBoxLayout *m_right_panel;
    RegTabPanel *m_right_content;
    QCheckBox *m_readonly_check;
    QLabel *m_data_soc_label;
    QPushButton *m_data_sel_reload;
    QPushButton *m_dump;
    IoBackend *m_io_backend;
    QTabWidget *m_type_selector;
    QListWidget *m_analysers_list;

private slots:
    void SetReadOnlyIndicator();
    void OnSocChanged(int index);
    void OnSocListChanged();
    void OnRegItemClicked(QTreeWidgetItem *clicked, int col);
    void OnBackendSelect(IoBackend *backend);
    void OnDataChanged();
    void OnDataSocActivated(const QString&);
    void OnAnalyserClicked(QListWidgetItem *clicked);
    void OnReadOnlyClicked(bool);
    void OnDumpRegs(bool);
    void OnBackendReload(bool);
    void OnTypeChanged(int index);
};

#endif /* REGTAB_H */
