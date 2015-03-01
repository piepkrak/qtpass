#include "newentrydialog.h"
#include "mainwindow.h"
#include <QDebug>

NewEntryDialog::NewEntryDialog( QWidget * parent) : QDialog(parent) {

    setupUi(this);

    MainWindow *mw = qobject_cast<MainWindow *>(parent);

    this->treeView->setModel(&mw->getProxyModel());
    this->treeView->setRootIndex(mw->getProxyModel().mapFromSource(mw->getModel().setRootPath(mw->getPassStore())));
    this->treeView->setColumnHidden(1, true);
    this->treeView->setColumnHidden(2, true);
    this->treeView->setColumnHidden(3, true);
    this->treeView->setHeaderHidden(true);
    this->treeView->setIndentation(15);
    this->treeView->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);

}

