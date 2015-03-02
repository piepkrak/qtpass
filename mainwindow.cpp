#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "util.h"
#include <QClipboard>
#include <QTimer>

/**
 * @brief MainWindow::MainWindow
 * @param parent
 */
MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    process = new QProcess(this);
//    connect(process, SIGNAL(readyReadStandardOutput()), this, SLOT(readyRead()));
    connect(process, SIGNAL(error(QProcess::ProcessError)), this, SLOT(processError(QProcess::ProcessError)));
    connect(process, SIGNAL(finished(int, QProcess::ExitStatus)), this, SLOT(processFinished(int, QProcess::ExitStatus)));
    ui->setupUi(this);
}

/**
 * @brief MainWindow::~MainWindow
 */
MainWindow::~MainWindow()
{
    delete ui;
}

/**
 * @brief MainWindow::checkConfig
 */
void MainWindow::checkConfig() {

    QSettings settings("IJHack", "QtPass");

    usePass = (settings.value("usePass") == "true");

    useClipboard = (settings.value("useClipboard") == "true");
    useAutoclear = (settings.value("useAutoclear") == "true");
    autoclearSeconds = settings.value("autoclearSeconds").toInt();
    hidePassword = (settings.value("hidePassword") == "true");
    hideContent = (settings.value("hideContent") == "true");

    passStore = settings.value("passStore").toString();
    if (passStore == "") {
        passStore = Util::findPasswordStore();
        settings.setValue("passStore", passStore);
    }

    passExecutable = settings.value("passExecutable").toString();
    if (passExecutable == "") {
        passExecutable = Util::findBinaryInPath("pass");
    }

    gitExecutable = settings.value("gitExecutable").toString();
    if (gitExecutable == "") {
        gitExecutable = Util::findBinaryInPath("git");
    }

    gpgExecutable = settings.value("gpgExecutable").toString();
    if (gpgExecutable == "") {
        gpgExecutable = Util::findBinaryInPath("gpg");
    }

    if (passExecutable == "" && (gitExecutable == "" || gpgExecutable == "")) {
        config();
    }

    model.setNameFilters(QStringList() << "*.gpg");
    model.setNameFilterDisables(false);

    proxyModel.setSourceModel(&model);
    proxyModel.setModelAndStore(&model, passStore);
    selectionModel = new QItemSelectionModel(&proxyModel);
    model.fetchMore(model.setRootPath(passStore));
    model.sort(0, Qt::AscendingOrder);

    ui->treeView->setModel(&proxyModel);
    ui->treeView->setRootIndex(proxyModel.mapFromSource(model.setRootPath(passStore)));
    ui->treeView->setColumnHidden(1, true);
    ui->treeView->setColumnHidden(2, true);
    ui->treeView->setColumnHidden(3, true);
    ui->treeView->setHeaderHidden(true);
    ui->treeView->setIndentation(15);
    ui->treeView->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);

    ui->textBrowser->setOpenExternalLinks(true);

    ui->lineEdit->setFocus();
}

/**
 * @brief MainWindow::config
 */
void MainWindow::config() {
    d = new Dialog();
    d->setModal(true);

    d->setPassPath(passExecutable);
    d->setGitPath(gitExecutable);
    d->setGpgPath(gpgExecutable);
    d->setStorePath(passStore);
    d->usePass(usePass);
    d->useClipboard(useClipboard);
    d->useAutoclear(useAutoclear);
    d->setAutoclear(autoclearSeconds);
    d->hidePassword(hidePassword);
    d->hideContent(hideContent);

    if (d->exec()) {
        if (d->result() == QDialog::Accepted) {
            passExecutable = d->getPassPath();
            gitExecutable = d->getGitPath();
            gpgExecutable = d->getGpgPath();
            passStore = d->getStorePath();
            usePass = d->usePass();
            useClipboard = d->useClipboard();
            useAutoclear = d->useAutoclear();
            autoclearSeconds = d->getAutoclear();
            hidePassword = d->hidePassword();
            hideContent = d->hideContent();

            QSettings settings("IJHack", "QtPass");

            settings.setValue("passExecutable", passExecutable);
            settings.setValue("gitExecutable", gitExecutable);
            settings.setValue("gpgExecutable", gpgExecutable);
            settings.setValue("passStore", passStore);
            settings.setValue("usePass", usePass ? "true" : "false");
            settings.setValue("useClipboard", useClipboard ? "true" : "false");
            settings.setValue("useAutoclear", useAutoclear ? "true" : "false");
            settings.setValue("autoclearSeconds", autoclearSeconds);
            settings.setValue("hidePassword", hidePassword ? "true" : "false");
            settings.setValue("hideContent", hideContent ? "true" : "false");

            ui->treeView->setRootIndex(model.setRootPath(passStore));
        }
    }
}

/**
 * @brief MainWindow::newEntry
 */
void MainWindow::newEntry() {
    newEntryDialog = new NewEntryDialog(this);
    newEntryDialog->show();
}

/**
 * @brief MainWindow::on_updateButton_clicked
 */
void MainWindow::on_updateButton_clicked()
{
    ui->statusBar->showMessage(tr("Updating password-store"), 2000);
    currentAction = GIT;
    if (usePass) {
        executePass("git pull");
    } else {
        executeWrapper(gitExecutable, "pull");
    }
}

/**
 * @brief MainWindow::on_newEntryButton_clicked
 */
void MainWindow::on_newEntryButton_clicked()
{
    newEntry();
}


/**
 * @brief MainWindow::on_treeView_clicked
 * @param index
 */
void MainWindow::on_treeView_clicked(const QModelIndex &index)
{
    currentAction = GPG;
    QString filePath = model.filePath(proxyModel.mapToSource(index));
    QString passFile = filePath;
    passFile.replace(QRegExp("\\.gpg$"), "");
    passFile.replace(QRegExp("^" + passStore), "");
    if (model.fileInfo(proxyModel.mapToSource(index)).isFile()){
        if (usePass) {
            executePass(passFile);
        } else {
            executeWrapper(gpgExecutable , "--no-tty -dq " + filePath);
        }
    }
}

/**
 * @brief MainWindow::executePass
 * @param args
 */
void MainWindow::executePass(QString args) {
    executeWrapper(passExecutable, args);
}

/**
 * @brief MainWindow::executeWrapper
 * @param app
 * @param args
 */
void MainWindow::executeWrapper(QString app, QString args) {
    process->setWorkingDirectory(passStore);
    process->start("sh", QStringList() << "-c" << app + " " + args);
    ui->textBrowser->clear();
    ui->textBrowser->setTextColor(Qt::black);
    enableUiElements(false);
}

/**
 * @brief MainWindow::readyRead
 */
void MainWindow::readyRead(bool finished = false) {
    QString output = ui->textBrowser->document()->toPlainText();
    QString error = process->readAllStandardError();
    if (error.size() > 0) {
        ui->textBrowser->setTextColor(Qt::red);
        output += error;
    } else {
        output += process->readAllStandardOutput();
        if (finished && currentAction == GPG) {
            if (useClipboard) {
                QClipboard *clip = QApplication::clipboard();
                QStringList tokens =  output.split("\n");
                clip->setText(tokens[0]);
                ui->statusBar->showMessage(tr("Password copied to clipboard"), 3000);
                if (useAutoclear) {
                      clippedPass = tokens[0];
                      QTimer::singleShot(1000*autoclearSeconds, this, SLOT(clearClipboard()));
                }
                if (hidePassword) {
                    tokens.pop_front();
                    output = tokens.join("\n");
                }
                if (hideContent) {
                    output = tr("Content hidden");
                }
            }
        }
    }
    output.replace(QRegExp("((http|https|ftp)\\://[a-zA-Z0-9\\-\\.]+\\.[a-zA-Z]{2,3}(:[a-zA-Z0-9]*)?/?([a-zA-Z0-9\\-\\._\\?\\,\\'/\\\\+&amp;%\\$#\\=~])*)"), "<a href=\"\\1\">\\1</a>");
    output.replace(QRegExp("\n"), "<br />");
    ui->textBrowser->setHtml(output);
}

/**
 * @brief MainWindow::clearClipboard
 * @TODO check clipboard content (only clear if contains the password)
 */
void MainWindow::clearClipboard()
{
    QClipboard *clipboard = QApplication::clipboard();
    if (clipboard->text() == clippedPass) {
        clipboard->clear();
        clippedPass = "";
        ui->statusBar->showMessage(tr("Clipboard cleared"), 3000);
    } else {
        ui->statusBar->showMessage(tr("Clipboard not cleared"), 3000);
    }
}

/**
 * @brief MainWindow::processFinished
 * @param exitCode
 * @param exitStatus
 */
void MainWindow::processFinished(int exitCode, QProcess::ExitStatus exitStatus) {
    if (exitStatus != QProcess::NormalExit || exitCode > 0) {
         ui->textBrowser->setTextColor(Qt::red);
    }
    readyRead(true);
    enableUiElements(true);

}

/**
 * @brief MainWindow::enableUiElements
 * @param state
 */
void MainWindow::enableUiElements(bool state) {
    ui->updateButton->setEnabled(state);
    ui->treeView->setEnabled(state);
    ui->lineEdit->setEnabled(state);
}

/**
 * @brief MainWindow::processError
 * @param error
 */
void MainWindow::processError(QProcess::ProcessError error)
{
    QString errorString;
    switch (error) {
        case QProcess::FailedToStart:
            errorString = tr("QProcess::FailedToStart");
            break;
        case QProcess::Crashed:
            errorString = tr("QProcess::Crashed");
            break;
        case QProcess::Timedout:
            errorString = tr("QProcess::Timedout");
            break;
        case QProcess::ReadError:
            errorString = tr("QProcess::ReadError");
            break;
        case QProcess::WriteError:
            errorString = tr("QProcess::WriteError");
            break;
        case QProcess::UnknownError:
            errorString = tr("QProcess::UnknownError");
            break;
    }
    ui->textBrowser->setTextColor(Qt::red);
    ui->textBrowser->setText(errorString);
}

/**
 * @brief MainWindow::setPassExecutable
 * @param path
 */
void MainWindow::setPassExecutable(QString path) {
    passExecutable = path;
}

/**
 * @brief MainWindow::setGitExecutable
 * @param path
 */
void MainWindow::setGitExecutable(QString path) {
    gitExecutable = path;
}

/**
 * @brief MainWindow::setGpgExecutable
 * @param path
 */
void MainWindow::setGpgExecutable(QString path) {
    gpgExecutable = path;
}

/**
 * @brief MainWindow::on_configButton_clicked
 */
void MainWindow::on_configButton_clicked()
{
    config();
}

/**
 * @brief MainWindow::on_lineEdit_textChanged
 * @param arg1
 */
void MainWindow::on_lineEdit_textChanged(const QString &arg1)
{
    ui->treeView->expandAll();
    ui->statusBar->showMessage(tr("Looking for: ") + arg1, 1000);
    QString query = arg1;
    query.replace(QRegExp(" "), ".*");
    QRegExp regExp(query, Qt::CaseInsensitive);
    proxyModel.setFilterRegExp(regExp);
    ui->treeView->setRootIndex(proxyModel.mapFromSource(model.setRootPath(passStore)));
    selectFirstFile();
}

/**
 * @brief MainWindow::on_lineEdit_returnPressed
 */
void MainWindow::on_lineEdit_returnPressed()
{
    selectFirstFile();
    on_treeView_clicked(ui->treeView->currentIndex());
}

/**
 * @brief MainWindow::selectFirstFile
 */
void MainWindow::selectFirstFile()
{
    QModelIndex index = proxyModel.mapFromSource(model.setRootPath(passStore));
    index = firstFile(index);
    ui->treeView->setCurrentIndex(index);
}

/**
 * @brief MainWindow::firstFile
 * @param parentIndex
 * @return QModelIndex
 */
QModelIndex MainWindow::firstFile(QModelIndex parentIndex) {
    QModelIndex index = parentIndex;
    int numRows = proxyModel.rowCount(parentIndex);
    for (int row = 0; row < numRows; ++row) {
        index = proxyModel.index(row, 0, parentIndex);
        if (model.fileInfo(proxyModel.mapToSource(index)).isFile()) {
            return index;
        }
        if (proxyModel.hasChildren(index)) {
            return firstFile(index);
        }
    }
    return index;
}

/**
 * @brief MainWindow::on_clearButton_clicked
 */
void MainWindow::on_clearButton_clicked()
{
    ui->lineEdit->clear();
}

/**
 * @brief MainWindow::setApp
 * @param app
 */
void MainWindow::setApp(SingleApplication *app)
{
    a = app;
    connect(a, SIGNAL(messageAvailable(QString)), this, SLOT(messageAvailable(QString)));
}

/**
 * @brief MainWindow::messageAvailable
 * @param message
 */
void MainWindow::messageAvailable(QString message)
{
    if (message == "show") {
        ui->lineEdit->selectAll();
        ui->lineEdit->setFocus();
    } else {
        ui->treeView->expandAll();
        ui->lineEdit->setText(message);
        on_lineEdit_returnPressed();
    }
    show();
    raise();
}

QFileSystemModel& MainWindow::getModel()
{
    return model;
}

StoreModel& MainWindow::getProxyModel()
{
    return proxyModel;
}

QString MainWindow::getPassStore()
{
    return passStore;
}
