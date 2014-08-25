#include "configdialog.h"
#include "ui_configdialog.h"
#include "downloaditem.h"

#include <QFileDialog>
#include <QSettings>

ConfigDialog::ConfigDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ConfigDialog)
{
    ui->setupUi(this);

    QSettings settings;

    QString path = settings.value("downloadPath", DownloadItem::getPackageDir()).toString();
    ui->downloadPathEdit->setText(path);

    int port = settings.value("proxyPort", 8888).toInt();
    ui->proxySpinBox->setValue(port);

    int maxTitles = settings.value("maxTitles", 10240).toInt();
    ui->maxTitlesSpinBox->setValue(maxTitles);

    int maxChecks = settings.value("maxChecks", 100).toInt();
    ui->maxChecksSpinBox->setValue(maxChecks);

    bool autostart = settings.value("autostartProxy", true).toBool();
    ui->autoStartCheckBox->setChecked(autostart);

    bool autocheck = settings.value("autoCheck", true).toBool();
    ui->downloadCheckBox->setChecked(autocheck);

    connect(ui->pushButton, SIGNAL(clicked()), this, SLOT(openFindDialog()));
}

void ConfigDialog::accept()
{
    QSettings settings;
    settings.setValue("downloadPath", ui->downloadPathEdit->text());
    settings.setValue("proxyPort", ui->proxySpinBox->value());
    settings.setValue("maxTitles", ui->maxTitlesSpinBox->value());
    settings.setValue("maxChecks", ui->maxChecksSpinBox->value());
    settings.setValue("autostartProxy", ui->autoStartCheckBox->isChecked());
    settings.setValue("autoCheck", ui->downloadCheckBox->isChecked());
    settings.sync();
    done(Accepted);
}

void ConfigDialog::openFindDialog()
{
    QString msg = tr("Select the folder to be used to store game packages");
    QString selected = QFileDialog::getExistingDirectory(this, msg, ui->downloadPathEdit->text(), QFileDialog::ShowDirsOnly);

    if(!selected.isEmpty())
        ui->downloadPathEdit->setText(QDir::toNativeSeparators((selected)));
}

ConfigDialog::~ConfigDialog()
{
    delete ui;
}
