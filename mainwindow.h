#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTreeView>
#include <QFileSystemModel>
#include <QProcess>
#include <QSettings>
#include "storemodel.h"
#include "dialog.h"
#include "newentrydialog.h"
#include "singleapplication.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

enum actionType { GPG, GIT };

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();
    void setPassExecutable(QString);
    void setGitExecutable(QString);
    void setGpgExecutable(QString);
    void checkConfig();
    void setApp(SingleApplication* app);
    QFileSystemModel &getModel();
    StoreModel &getProxyModel();
    QString getPassStore();

private slots:
    void on_updateButton_clicked();
    void on_newEntryButton_clicked();
    void on_treeView_clicked(const QModelIndex &index);
    void on_configButton_clicked();
    void readyRead(bool finished);
    void processFinished(int, QProcess::ExitStatus);
    void processError(QProcess::ProcessError);
    void clearClipboard();
    void on_lineEdit_textChanged(const QString &arg1);
    void on_lineEdit_returnPressed();
    void on_clearButton_clicked();
    void messageAvailable(QString message);

private:
    Ui::MainWindow *ui;
    QFileSystemModel model;
    StoreModel proxyModel;
    QItemSelectionModel *selectionModel;
    QProcess *process;
    SingleApplication *a;
    Dialog* d;
    NewEntryDialog* newEntryDialog;
    bool usePass;
    bool useClipboard;
    bool useAutoclear;
    bool hidePassword;
    bool hideContent;
    int autoclearSeconds;
    QString passStore;
    QString passExecutable;
    QString gitExecutable;
    QString gpgExecutable;
    QString clippedPass;
    actionType currentAction;
    void updateText();
    void executePass(QString);
    void executeWrapper(QString, QString);
    void config();
    void newEntry();
    void enableUiElements(bool);
    void selectFirstFile();
    QModelIndex firstFile(QModelIndex parentIndex);
};

#endif // MAINWINDOW_H
