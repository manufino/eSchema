#ifndef OPTIONSDIALOG_H
#define OPTIONSDIALOG_H

#include <QDialog>

#include "SettingsManager.h"

namespace Ui {
class OptionsDialog;
}

class OptionsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit OptionsDialog(QWidget *parent = nullptr);
    ~OptionsDialog();

protected:
    void loadSettings();
    void saveSettings();

public slots:
    void accept();
    void cancel();
    void apply();
    void restore();

private:
    Ui::OptionsDialog *ui;
};

#endif // OPTIONSDIALOG_H
