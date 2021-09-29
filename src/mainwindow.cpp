/*
 * Copyright (c) 2019-2021-2021, Ilya Kotov <iokotov@astralinux.ru>
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
#include <QInputDialog>
#include <QDir>
#include <QFile>
#include <QSettings>
#include <QMap>
#include <QMessageBox>
#include <QFileDialog>
#include "tracklistitemdelegate.h"
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
    m_ui->treeView->setItemDelegate(new TrackListItemDelegate(this));
    m_process = new QProcess(this);
    connect(m_process, SIGNAL(finished(int,QProcess::ExitStatus)), SLOT(onFinished(int,QProcess::ExitStatus)));
    connect(m_process, SIGNAL(readyRead()), SLOT(onReadyRead()));
    connect(m_model, &CueModel::countChanged, [=] {
        m_ui->downloadButton->setEnabled(!m_model->isEmpty());
        m_ui->saveAsAction->setEnabled(!m_model->isEmpty());
    });
    readSettings();
    m_ui->downloadButton->setEnabled(false);
    m_ui->saveAsAction->setEnabled(false);
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

    if(!findBackend())
        return;

    m_model->clear();
    m_ui->formatComboBox->clear();
    QStringList args = { "-j", m_ui->urlEdit->text() };
    m_process->start(m_backend, args);
    m_state = Fetching;
}

void MainWindow::onReadyRead()
{
    if(m_state == Downloading)
    {
        QString line = QString::fromLatin1(m_process->readAll()).trimmed();
        static QRegularExpression progressRegexp("^\\[download\\]\\s+(\\d+\\.\\d+)%\\s+of\\s+~*(\\d+\\.\\d+)MiB"
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

        m_title = json["title"].toString();
        QString album = json["album"].toString();
        QString artist = json["artist"].toString();
        QString date = json["upload_date"].toString();
        int duration = json["duration"].toInt();

        if(album.isEmpty() || artist.isEmpty())
        {
            QStringList parts = m_title.split(" - ");
            if(parts.count() >= 2)
            {
                artist = parts.at(0);
                album = parts.at(1);
            }
        }

        m_ui->albumEdit->setText(album);
        m_ui->albumArtistEdit->setText(artist);
        m_ui->fileEdit->setText(m_title);
        m_ui->genreEdit->clear();
        m_ui->dateEdit->setText(date.mid(0, 4));
        m_ui->commentEdit->setText(m_ui->urlEdit->text());

        QRegularExpression titleRegexp("^\\d+\\.\\s");

        for(const QJsonValue &value : json["chapters"].toArray())
        {
            m_model->addTrack(artist, value["title"].toString().remove(titleRegexp).trimmed(),
                    value["start_time"].toInt());
        }

        m_ui->durationLabel->setText(Utils::formatDuration(duration));

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
    if(m_ui->formatComboBox->currentIndex() < 0 || !findBackend())
        return;

    applyMetaData();
    QString codec = m_ui->formatComboBox->currentText();
    QString cueDir = m_ui->outDirLineEdit->text() + "/" + m_title;
    QDir("/").mkpath(cueDir);
    QString cuePath = cueDir + "/" + m_ui->fileEdit->text() + ".cue";
    QFile file(cuePath);
    file.open(QIODevice::WriteOnly);
    file.write(m_model->generate());

    QStringList args = {
        "-x", "--audio-format", codec, m_ui->urlEdit->text(),
        "-o", cueDir + "/" + m_ui->fileEdit->text() + ".%(ext)s",
        "--write-thumbnail"
    };

    m_state = Downloading;
    m_process->start(m_backend, args);
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

void MainWindow::on_addTrackButton_clicked()
{
    m_model->addTrack("?", "?", 0);
}

void MainWindow::on_addFromTextButton_clicked()
{
    QString text = QInputDialog::getMultiLineText(this, tr("Add track list"), tr("Titles and offsets:"));
    QRegularExpression durationRegExp("(\\d+)\\:(\\d+)");

    if(text.isEmpty())
        return;

    m_model->clear();

    for(QString line : text.split("\n"))
    {
        line = line.trimmed();
        line.replace("\"", QString());
        QRegularExpressionMatch m = durationRegExp.match(line);
        if(m.hasMatch())
        {
            m_model->addTrack(m_ui->albumArtistEdit->text(),
                              line.remove(durationRegExp).trimmed(),
                              m.captured(1).toInt() * 60 + m.captured(2).toInt());
        }
    }

    //is it duration?
    bool convert = false;
    int previousOffset = 0;
    for(int i = 0; i < m_model->count(); ++i)
    {
        if(m_model->offset(i) < previousOffset) //detect duration
        {
            convert = true;
            break;
        }
        else
        {
            previousOffset = m_model->offset(0);
        }
    }

    //convert duration to offset
    if(convert)
    {
        int offset = 0;
        for(int i = 0; i < m_model->count(); ++i)
        {
            int duration = m_model->offset(i);
            m_model->setOffset(i, offset);
            offset += duration;
        }
    }
}

void MainWindow::on_removeTrackButton_clicked()
{
    QModelIndex idx = m_ui->treeView->currentIndex();
    if(idx.isValid())
        m_model->removeTrack(idx.row());
}

void MainWindow::on_saveAsAction_triggered()
{
    QString path = QFileDialog::getSaveFileName(this, tr("Save CUE As..."),
                                                QDir::homePath() + "/" + m_ui->fileEdit->text() + ".cue",
                                                "*.cue");
    if(!path.isEmpty())
    {
        applyMetaData();
        QFile file(path);
        file.open(QIODevice::WriteOnly);
        file.write(m_model->generate());
    }
}

void MainWindow::on_exitAction_triggered()
{
    QApplication::closeAllWindows();
}

void MainWindow::on_aboutAction_triggered()
{
    if(!findBackend())
        return;

    QString backendName = QString("<b>%1</b>").arg(m_backend);

    if(m_backend == QStringLiteral("yt-dlp"))
        backendName = QStringLiteral("<a href=\"https://github.com/yt-dlp/yt-dlp\">yt-dlp</a>");
    else if(m_backend == QStringLiteral("youtube-dl"))
        backendName = QStringLiteral("<a href=\"https://youtube-dl.org\">youtube-dl</a>");

    QMessageBox::about(this, tr("About YouTube to CUE Converter"),
                       QStringLiteral("<b>") + tr("YouTube to CUE Converter %1").arg(YTBTOCUE_VERSION_STR) + "</b><br>" +
                       tr("This program is intended to download audio albums from <a href=\"https://www.youtube.com\">YouTube</a>. It downloads "
                          "audio file using %1 and generates Cue Sheet with metadata.").arg(backendName) + "<br><br>"+
                       tr("Qt version: %1 (compiled with %2)").arg(qVersion()).arg(QT_VERSION_STR) + "<br>"+
                       tr("%1 version: %2").arg(m_backend).arg(m_version) + "<br><br>" +
                       tr("Written by: Ilya Kotov &lt;iokotov@astralinux.ru&gt;")  + "<br>" +
                       tr("Home page: <a href=\"https://github.com/trialuser02/ytbtocue\">"
                          "https://github.com/trialuser02/ytbtocue</a>"));
}

void MainWindow::on_aboutQtAction_triggered()
{
    QMessageBox::aboutQt(this);
}

void MainWindow::on_selectOutDirButton_clicked()
{
    QString p = QFileDialog::getExistingDirectory(this, tr("Select Output Directory"), m_ui->outDirLineEdit->text());
    if(!p.isEmpty())
        m_ui->outDirLineEdit->setText(p);
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

void MainWindow::applyMetaData()
{
    QString ext = m_ui->formatComboBox->currentData().toString();
    m_model->setMetaData(CueModel::PERFORMER, m_ui->albumArtistEdit->text());
    m_model->setMetaData(CueModel::TITLE, m_ui->albumEdit->text());
    m_model->setMetaData(CueModel::GENRE, m_ui->genreEdit->text());
    m_model->setMetaData(CueModel::DATE, m_ui->dateEdit->text());
    m_model->setMetaData(CueModel::COMMENT, m_ui->commentEdit->text());
    m_model->setMetaData(CueModel::FILE, m_ui->fileEdit->text() + "." + ext);
}

bool MainWindow::findBackend()
{
    if(!m_backend.isEmpty())
        return true;

    static const QStringList backends = { "yt-dlp", "youtube-dl" };

    for(const QString &backend : qAsConst(backends))
    {
        QProcess p;
        p.start(backend, { "--version" });
        p.waitForFinished();
        if(p.exitCode() == EXIT_SUCCESS)
        {
            m_backend = backend;
            m_version = QString::fromLatin1(p.readAll()).trimmed();
            qDebug() << "using" << m_backend << m_version;
            return true;
        }
    }

    return false;
}
