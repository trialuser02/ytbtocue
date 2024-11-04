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
#include "settingsdialog.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent)
{
    m_ui = new Ui::MainWindow;
    m_ui->setupUi(this);
    m_model = new CueModel(this);
    m_ui->treeView->setModel(m_model);
    m_ui->treeView->setItemDelegate(new TrackListItemDelegate(this));
    m_process = new QProcess(this);
    connect(m_process, qOverload<int, QProcess::ExitStatus>(&QProcess::finished), this, &MainWindow::onFinished);
    connect(m_process, &QProcess::readyRead, this, &MainWindow::onReadyRead);
    connect(m_model, &CueModel::countChanged, this, [=] {
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

    if(m_process->state() == QProcess::Running || !findBackend())
        return;

    m_model->clear();
    m_ui->formatComboBox->clear();
    QStringList args = { u"-j"_s, m_ui->urlEdit->text() };
    if(m_proxyUrl.isValid())
        args << u"--proxy"_s << m_proxyUrl.toString();
    m_process->start(m_backend, args);
    m_state = Fetching;
}

void MainWindow::onReadyRead()
{
    if(m_state == Downloading)
    {
        QString line = QString::fromLatin1(m_process->readAll()).trimmed();
        static QRegularExpression progressRegexp(u"^\\[download\\]\\s+(\\d+\\.\\d+)%\\s+of\\s+~{0,1}\\s*(\\d+\\.\\d+)MiB"
                                                 "\\s+at\\s+(\\d+\\.\\d+)([MK]iB/s)\\s+ETA\\s+(\\d+:\\d+)\\s*.*"_s);
        QRegularExpressionMatch m = progressRegexp.match(line);
        if(m.isValid() && m.hasMatch())
        {
            m_ui->progressBar->setValue(int(m.captured(1).toDouble()));
            m_ui->statusbar->showMessage(tr("%1 MiB | %2 %3 | ETA: %4").arg(m.captured(2), m.captured(3),
                                                                            m.captured(4).startsWith("KiB") ? tr("KiB/s") : tr("MiB/s"),
                                                                            m.captured(5)));
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

        m_title = json[u"title"_s].toString();
        QString album = json[u"album"_s].toString();
        QString artist = json[u"artist"_s].toString();
        QString date = json[u"upload_date"_s].toString();
        QString description = json[u"description"_s].toString();
        int duration = json[u"duration"_s].toInt();

        if(album.isEmpty() || artist.isEmpty())
        {
            QStringList parts = m_title.split(u" - "_s);
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

        static const QRegularExpression titleRegexp(u"^\\d+\\.\\s"_s);

        for(const QJsonValue &value : json[u"chapters"_s].toArray())
        {
            m_model->addTrack(artist, value[u"title"_s].toString().remove(titleRegexp).trimmed(),
                    value[u"start_time"_s].toInt());
        }

        static const QRegularExpression descriptionRegexp(u"[Gg]enre:\\s+([A-Z,/,a-z,\\s]+)\\\n"_s);
        QRegularExpressionMatch match = descriptionRegexp.match(description);
        if(match.hasMatch())
            m_ui->genreEdit->setText((match.captured(1).trimmed()));

        m_ui->durationLabel->setText(Utils::formatDuration(duration));

        m_ui->formatComboBox->addItem(u"opus"_s, u"opus"_s);
        m_ui->formatComboBox->addItem(u"vorbis"_s, u"ogg"_s);
        m_ui->formatComboBox->addItem(u"m4a"_s, u"m4a"_s);
        m_ui->formatComboBox->addItem(u"aac"_s, u"aac"_s);
    }
    else if(m_state == Downloading)
    {
        m_ui->statusbar->showMessage(tr("Finished"));
        m_ui->progressBar->setValue(100);
    }
    m_state = Idle;
}

void MainWindow::on_downloadButton_clicked()
{
    if(m_ui->formatComboBox->currentIndex() < 0 || !findBackend())
        return;

    applyMetaData();
    QString codec = m_ui->formatComboBox->currentText();
    QString cueDir = m_ui->outDirLineEdit->text() + QLatin1Char('/') + m_title;
    QDir("/").mkpath(cueDir);
    QString cuePath = cueDir + QLatin1Char('/') + m_ui->fileEdit->text() + u".cue"_s;
    QFile file(cuePath);
    file.open(QIODevice::WriteOnly);
    file.write(m_model->generate());

    QStringList args = {
        u"-x"_s, u"--audio-format"_s, codec, m_ui->urlEdit->text(),
        u"-o"_s, cueDir + QLatin1Char('/') + m_ui->fileEdit->text() + ".%(ext)s",
        u"--write-thumbnail"_s
    };

    if(m_proxyUrl.isValid())
        args << u"--proxy"_s << m_proxyUrl.toString();

    args << m_commandLineArgs;

    qDebug() << args;

    m_state = Downloading;
    m_process->start(m_backend, args);
    m_ui->progressBar->setValue(0);
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
    m_model->addTrack(u"?"_s, u"?"_s, 0);
}

void MainWindow::on_addFromTextButton_clicked()
{
    QString text = QInputDialog::getMultiLineText(this, tr("Add track list"), tr("Titles and offsets:"));
    static const QRegularExpression durationRegExp(u"(\\d+)\\:(\\d+)"_s);

    if(text.isEmpty())
        return;

    m_model->clear();

    for(QString line : text.split(QChar::LineFeed))
    {
        line = line.trimmed();
        line.replace(QLatin1Char('\"'), QString());
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

        previousOffset = m_model->offset(0);
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
                                                QDir::homePath() + QLatin1Char('/') + m_ui->fileEdit->text() + u".cue"_s,
                                                u"*.cue"_s);
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

    QString backendName = QStringLiteral("<b>%1</b>").arg(m_backend);

    if(m_backend == QStringLiteral("yt-dlp"))
        backendName = QStringLiteral("<a href=\"https://github.com/yt-dlp/yt-dlp\">yt-dlp</a>");
    else if(m_backend == QStringLiteral("youtube-dl"))
        backendName = QStringLiteral("<a href=\"https://youtube-dl.org\">youtube-dl</a>");

    QMessageBox::about(this, tr("About YouTube to CUE Converter"),
                       QStringLiteral("<b>") + tr("YouTube to CUE Converter %1").arg(QStringLiteral(YTBTOCUE_VERSION_STR)) + u"</b><br>"_s +
                       tr("This program is intended to download audio albums from <a href=\"https://www.youtube.com\">YouTube</a>. It downloads "
                          "audio file using %1 and generates Cue Sheet with metadata.").arg(backendName) + u"<br><br>"_s+
                       tr("Qt version: %1 (compiled with %2)").arg(qVersion(), QT_VERSION_STR) + u"<br>"_s+
                       tr("%1 version: %2").arg(m_backend, m_version) + u"<br><br>"_s +
                       tr("Written by: Ilya Kotov &lt;iokotov@astralinux.ru&gt;")  + u"<br>"_s +
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

void MainWindow::on_settingsAction_triggered()
{
    SettingsDialog dlg(this);
    if(dlg.exec() == QDialog::Accepted)
        readSettings();
}

void MainWindow::closeEvent(QCloseEvent *)
{
    writeSettings();
}

void MainWindow::readSettings()
{
    QSettings settings;
    settings.beginGroup("General");
    if(!m_update)
    {
        restoreGeometry(settings.value("mw_geometry"_L1).toByteArray());
        m_ui->urlEdit->setText(settings.value("url"_L1).toString());
        QString musicLocation = QStandardPaths::writableLocation(QStandardPaths::MusicLocation);
        m_ui->outDirLineEdit->setText(settings.value("out_dir"_L1, musicLocation).toString());
        m_ui->treeView->header()->restoreState(settings.value("track_list_state"_L1).toByteArray());
    }
    m_commandLineArgs = settings.value("command_line_args"_L1).toString().replace(QChar::LineFeed, QChar::Space)
            .split(QChar::Space, Qt::SkipEmptyParts);
    settings.endGroup();

    if(settings.value("Proxy/use_proxy"_L1, false).toBool())
        m_proxyUrl = settings.value("Proxy/url"_L1).toUrl();
    else
        m_proxyUrl.clear();

    m_update = true;
}

void MainWindow::writeSettings()
{
    QSettings settings;
    settings.beginGroup("General"_L1);
    settings.setValue("mw_geometry"_L1, saveGeometry());
    settings.setValue("url"_L1, m_ui->urlEdit->text());
    settings.setValue("out_dir"_L1, m_ui->outDirLineEdit->text());
    settings.setValue("track_list_state"_L1, m_ui->treeView->header()->saveState());
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

    static const QStringList backends = { u"yt-dlp"_s, u"youtube-dl"_s };

    for(const QString &backend : std::as_const(backends))
    {
        QProcess p;
        p.start(backend, { u"--version"_s });
        p.waitForFinished();
        if(p.exitCode() == EXIT_SUCCESS)
        {
            m_backend = backend;
            m_version = QString::fromLatin1(p.readAll()).trimmed();
            qDebug() << u"using"_s << m_backend << m_version;
            return true;
        }
    }

    return false;
}
