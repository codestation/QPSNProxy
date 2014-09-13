/**
 * QPSNProxy is an open source software for downloading PSN packages
 * on a PC, then transfer them to a game console via a HTTP proxy.
 *
 *  Copyright (C) 2014 codestation
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "authdialog.h"
#include "ui_authdialog.h"

#include <QMessageBox>
#include <QSettings>

AuthDialog::AuthDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::AuthDialog)
{
    ui->setupUi(this);

    ui->usernameEdit->setText(QSettings().value("lastUsername", "").toString());

    if(!ui->usernameEdit->text().isEmpty())
        ui->passwordEdit->setFocus();

    connect(ui->buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
}

void AuthDialog::accept()
{
    if(ui->usernameEdit->text().isEmpty() || ui->passwordEdit->text().isEmpty())
    {
        QMessageBox msg;
        msg.setText(tr("Username/password shouldn't be empty"));
    }
    else
    {
        QSettings().setValue("lastUsername", ui->usernameEdit->text());
        QDialog::accept();
    }
}

QPair<QString, QString> AuthDialog::getAuth()
{
    return qMakePair(ui->usernameEdit->text(), ui->passwordEdit->text());
}

AuthDialog::~AuthDialog()
{
    delete ui;
}
