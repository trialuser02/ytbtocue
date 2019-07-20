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
#include <QStandardPaths>
#include <QDir>
#include <QFile>
#include <QSettings>
#include "mainwindow.h"
#include "cuemodel.h"
#include "utils.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent)
{
    m_ui = new Ui::MainWindow;
    m_ui->setupUi(this);
    m_model = new CueModel(this);
    m_ui->treeView->setModel(m_model);
    m_process = new QProcess(this);
    connect(m_process, SIGNAL(finished(int,QProcess::ExitStatus)), SLOT(onFinished(int,QProcess::ExitStatus)));
    readSettings();
}

MainWindow::~MainWindow()
{
    delete m_ui;
}

void MainWindow::on_fetchButton_clicked()
{
    if(m_ui->urlEdit->text().isEmpty())
        return;

    m_model->clear();
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

    m_file = json["_filename"].toString();
    m_title = json["title"].toString();
    QString album = json["album"].toString();
    QString artist = json["artist"].toString();
    QString cover = json["thumbnail"].toString();
    int duration = json["duration"].toInt();

    if(album.isEmpty() || artist.isEmpty())
    {
        QStringList parts = m_title.split(" - ");
        if(parts.count() == 2)
        {
            artist = parts.at(0);
            album = parts.at(1);
        }
    }

    QRegularExpression titleRegexp("^\\d+\\.\\s");

    for(const QJsonValue &value : json["chapters"].toArray())
    {
        m_model->addTrack(artist, value["title"].toString().remove(titleRegexp).trimmed(),
                value["start_time"].toInt());
    }

    m_ui->albumArtistLabel->setText(artist);
    m_ui->durationLabel->setText(Utils::formatDuration(duration));
    m_ui->fileNameLabel->setText(m_file);
}

void MainWindow::on_downloadButton_clicked()
{
   QString cueDir = m_ui->outDirLineEdit->text() + "/" + m_title;
   QDir("/").mkpath(cueDir);
   QString fileName = m_model->file();
   fileName.remove(QRegularExpression("\\.\\S+$"));

   QString cuePath = cueDir + "/" + fileName + ".cue";
   QFile file(cuePath);
   file.open(QIODevice::WriteOnly);
   file.write(m_model->generate());
}

void MainWindow::closeEvent(QCloseEvent *)
{
    writeSettings();
}

void MainWindow::readSettings()
{
   QSettings settings;
   settings.beginGroup("General");
   restoreGeometry(settings.value("mw_geometry").toByteArray());
   m_ui->urlEdit->setText(settings.value("url", "https://www.youtube.com/watch?v=9MolAvqXbzU").toString());
   QString musicLocation = QStandardPaths::writableLocation(QStandardPaths::MusicLocation);
   m_ui->outDirLineEdit->setText(settings.value("out_dir", musicLocation).toString());
   m_ui->treeView->header()->restoreState(settings.value("track_list_state").toByteArray());
   settings.endGroup();
}

void MainWindow::writeSettings()
{
    QSettings settings;
    settings.beginGroup("General");
    settings.setValue("mw_geometry", saveGeometry());
    settings.setValue("url", m_ui->urlEdit->text());
    settings.setValue("out_dir", m_ui->outDirLineEdit->text());
    settings.setValue("track_list_state", m_ui->treeView->header()->saveState());
    settings.endGroup();
}
