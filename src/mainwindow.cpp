/*
 * Copyright (c) 2019, Ilya Kotov <iokotov@astralinux.ru>
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
#include <QMap>
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
    connect(m_process, SIGNAL(readyRead()), SLOT(onReadyRead()));
    readSettings();
}

MainWindow::~MainWindow()
{
    if(m_process->state() == QProcess::Running)
    {
        m_process->kill();
        m_process->waitForFinished();
    }
    delete m_ui;
}

void MainWindow::on_fetchButton_clicked()
{
    if(m_ui->urlEdit->text().isEmpty())
        return;

    m_model->clear();
    m_ui->formatComboBox->clear();
    QStringList args = { "-j", m_ui->urlEdit->text() };
    m_process->start("youtube-dl", args);
    m_state = Fetching;
}

void MainWindow::onReadyRead()
{
    if(m_state == Downloading)
    {
        QString line = QString::fromLatin1(m_process->readAll()).trimmed();
        static QRegularExpression progressRegexp("^\\[download\\]\\s+(\\d+\\.\\d+)%\\s+of\\s+(\\d+\\.\\d+)MiB"
                                                 "\\s+at\\s+(\\d+\\.\\d+)KiB/s\\s+ETA\\s+(\\d+:\\d+)");
        QRegularExpressionMatch m = progressRegexp.match(line);
        if(m.isValid() && m.hasMatch())
        {
            m_ui->progressBar->setValue(int(m.captured(1).toDouble()));
            m_ui->statusbar->showMessage(tr("%1 MiB | %2 KiB/s | ETA: %3")
                                         .arg(m.captured(2))
                                         .arg(m.captured(3))
                                         .arg(m.captured(4)));
        }
    }
}

void MainWindow::onFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    if(m_state == Cancelling)
    {
        m_state = Idle;
        return;
    }

    if(exitCode != EXIT_SUCCESS || exitStatus != QProcess::NormalExit)
    {
        qWarning("MainWindow: youtube-dl finished with error: %s", qPrintable(m_process->errorString()));
        return;
    }

    if(m_state == Fetching)
    {
        QJsonDocument json = QJsonDocument::fromJson(m_process->readAllStandardOutput());
        if(json.isEmpty())
        {
            qWarning("MainWindow: unable to parse youtube-dl output");
            return;
        }

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

        m_model->setAlbum(album);
        m_ui->albumArtistLabel->setText(artist);
        m_ui->durationLabel->setText(Utils::formatDuration(duration));
        m_ui->fileNameLabel->setText(m_file);

        m_ui->formatComboBox->addItem("opus", "opus");
        m_ui->formatComboBox->addItem("vorbis", "ogg");
        m_ui->formatComboBox->addItem("m4a", "m4a");
        m_ui->formatComboBox->addItem("aac", "aac");
    }
    else if(m_state == Downloading)
    {
        m_ui->statusbar->showMessage(tr("Finished"));
    }
    m_state = Idle;
}

void MainWindow::on_downloadButton_clicked()
{
    if(m_ui->formatComboBox->currentIndex() < 0)
        return;

    QString codec = m_ui->formatComboBox->currentText();
    QString ext = m_ui->formatComboBox->currentData().toString();

    QString cueDir = m_ui->outDirLineEdit->text() + "/" + m_title;
    QDir("/").mkpath(cueDir);

    m_model->setFile(m_title + "." + ext);
    QString cuePath = cueDir + "/" + m_title + ".cue";
    QFile file(cuePath);
    file.open(QIODevice::WriteOnly);
    file.write(m_model->generate());

    QStringList args = {
        "-x", "--audio-format", codec, m_ui->urlEdit->text(),
        "-o", m_ui->outDirLineEdit->text() + "/%(title)s/%(title)s.%(ext)s"
    };

    m_state = Downloading;
    m_process->start("youtube-dl", args);
}

void MainWindow::on_cancelButton_clicked()
{
    if(m_process->state() == QProcess::Running)
    {
        m_state = Cancelling;
        m_process->kill();
        m_process->waitForFinished();
        m_ui->statusbar->showMessage(tr("Stopped"));
    }
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
    m_ui->urlEdit->setText(settings.value("url").toString());
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
