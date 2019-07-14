/*
 * Copyright (c) 2019, Ilya Kotov <forkotov02@ya.ru>
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

#include <QtDebug>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonValue>
#include <QRegularExpression>
#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent)
{
    m_ui = new Ui::MainWindow;
    m_ui->setupUi(this);
    m_process = new QProcess(this);
    connect(m_process, SIGNAL(finished(int,QProcess::ExitStatus)), SLOT(onFinished(int,QProcess::ExitStatus)));
    m_ui->urlEdit->setText("https://www.youtube.com/watch?v=9MolAvqXbzU");
}

MainWindow::~MainWindow()
{
    delete m_ui;
}

void MainWindow::on_fetchButton_clicked()
{
    if(m_ui->urlEdit->text().isEmpty())
        return;

    m_ui->tracksWidget->clear();
    QStringList args = { "-j", m_ui->urlEdit->text() };
    m_process->start("youtube-dl", args);
}

void MainWindow::onFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    QJsonDocument json = QJsonDocument::fromJson(m_process->readAllStandardOutput());
    if(json.isEmpty())
    {
        qWarning("MainWindow: unable to parse youtube-dl output");
        return;
    }

    qDebug("+%s+", json.toJson().constData());

    QString fileName = json["_filename"].toString();
    QString album = json["album"].toString();
    QString artist = json["artist"].toString();
    QString cover = json["thumbnail"].toString();

    if(album.isEmpty() || artist.isEmpty())
    {
        QStringList parts = fileName.split(" - ");
        if(parts.count() == 2)
        {
            artist = parts.at(0);
            album = parts.at(1);
        }
    }

    QRegularExpression titleRegexp("^\\d+\\.\\s");

    for(const QJsonValue &value : json["chapters"].toArray())
    {
        QStringList titles =
        {
            QString::number(m_ui->tracksWidget->topLevelItemCount() + 1),
            artist,
            value["title"].toString().remove(titleRegexp).trimmed(),
            QString::number(value["start_time"].toInt()),
        };

        QTreeWidgetItem *item = new QTreeWidgetItem(titles);
        m_ui->tracksWidget->addTopLevelItem(item);
    }
    m_ui->tracksWidget->resizeColumnToContents(0);
    m_ui->tracksWidget->resizeColumnToContents(1);
    m_ui->tracksWidget->resizeColumnToContents(2);
    m_ui->tracksWidget->resizeColumnToContents(3);
}
