#ifndef NEWENTRYDIALOG_H
#define NEWENTRYDIALOG_H

#include <QDialog>
#include "ui_newentrydialog.h"

class NewEntryDialog : public QDialog, public Ui::NewEntry {
    Q_OBJECT

public:
    NewEntryDialog( QWidget * parent = 0);
};

#endif // NEWENTRYDIALOG_H
