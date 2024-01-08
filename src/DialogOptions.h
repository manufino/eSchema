#ifndef DIALOGOPTIONS_H
#define DIALOGOPTIONS_H

#include <QDialog>
#include <QMessageBox>

#include "SettingsManager.h"

namespace Ui {
class DialogOptions;
}

class DialogOptions : public QDialog
{
    Q_OBJECT

public:
    explicit DialogOptions(QWidget *parent = nullptr);
    ~DialogOptions();

protected:
    void loadSettings();
    void saveSettings();

public slots:
    void accept();
    void cancel();
    void apply();
    void restore();

private:
    Ui::DialogOptions *ui;
};

#endif // DIALOGOPTIONS_H
