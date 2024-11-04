/*
 * Copyright (c) 2019-2024, Ilya Kotov <iokotov@astralinux.ru>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <QIntValidator>
#include <QSettings>
#include "utils.h"
#include "settingsdialog.h"
#include "ui_settingsdialog.h"

SettingsDialog::SettingsDialog(QWidget *parent) :
    QDialog(parent),
    m_ui(new Ui::SettingsDialog)
{
    m_ui->setupUi(this);
    m_ui->portLineEdit->setValidator(new QIntValidator(0, 65535, this));
    m_ui->proxyTypeComboBox->addItems({ u"HTTP"_s, u"HTTPS"_s, u"SOCKS4"_s, u"SOCKS4a"_s, u"SOCKS5"_s, u"SOCKS5h"_s });
    QSettings settings;
    m_ui->enableProxyCheckBox->setChecked(settings.value("Proxy/use_proxy"_L1, false).toBool());
    QUrl url = settings.value("Proxy/url"_L1).toUrl();
    int index = m_ui->proxyTypeComboBox->findText(url.scheme(), Qt::MatchFixedString);
    m_ui->proxyTypeComboBox->setCurrentIndex(index);
    m_ui->hostLineEdit->setText(url.host());
    m_ui->portLineEdit->setText(url.port() >= 0 ? QString::number(url.port()) : QString());
    m_ui->userLineEdit->setText(url.userName());
    m_ui->passwordLineEdit->setText(url.password());
    m_ui->commandLineTextEdit->setPlainText(settings.value("General/command_line_args"_L1).toString());
}

SettingsDialog::~SettingsDialog()
{
    delete m_ui;
}

void SettingsDialog::accept()
{
    QSettings settings;
    settings.setValue("Proxy/use_proxy"_L1, m_ui->enableProxyCheckBox->isChecked());
    QUrl url;
    url.setScheme(m_ui->proxyTypeComboBox->currentText().toLower());
    url.setHost(m_ui->hostLineEdit->text());
    url.setPort(m_ui->portLineEdit->text().toInt());
    url.setUserName(m_ui->userLineEdit->text());
    url.setPassword(m_ui->passwordLineEdit->text());
    settings.setValue("Proxy/url"_L1, url);
    settings.setValue("General/command_line_args"_L1, m_ui->commandLineTextEdit->toPlainText());
    QDialog::accept();
}
