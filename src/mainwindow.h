/*
 * Copyright (c) 2019-2022, Ilya Kotov <iokotov@astralinux.ru>
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

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QProcess>

namespace Ui {
    class MainWindow;
}

class CueModel;

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void on_fetchButton_clicked();
    void onReadyRead();
    void onFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void on_downloadButton_clicked();
    void on_cancelButton_clicked();
    void on_addTrackButton_clicked();
    void on_addFromTextButton_clicked();
    void on_removeTrackButton_clicked();
    void on_saveAsAction_triggered();
    void on_exitAction_triggered();
    void on_aboutAction_triggered();
    void on_aboutQtAction_triggered();
    void on_selectOutDirButton_clicked();

private:
    void closeEvent(QCloseEvent *) override;
    void readSettings();
    void writeSettings();
    void applyMetaData();
    bool findBackend();

    enum State
    {
        Idle = 0,
        Fetching,
        Downloading,
        Cancelling,
    };

    Ui::MainWindow *m_ui;
    QProcess *m_process;
    CueModel *m_model;
    QString m_title;
    State m_state = Idle;
    QString m_backend, m_version;
};

#endif // MAINWINDOW_H
