#include <QIntValidator>
#include <QSettings>
#include "settingsdialog.h"
#include "ui_settingsdialog.h"

SettingsDialog::SettingsDialog(QWidget *parent) :
    QDialog(parent),
    m_ui(new Ui::SettingsDialog)
{
    m_ui->setupUi(this);
    m_ui->portLineEdit->setValidator(new QIntValidator(0, 65535, this));
    m_ui->proxyTypeComboBox->addItems({ "HTTP", "HTTPS", "SOCKS4", "SOCKS4a", "SOCKS5", "SOCKS5h" });
    QSettings settings;
    m_ui->enableProxyCheckBox->setChecked(settings.value("Proxy/use_proxy", false).toBool());
    QUrl url = settings.value("Proxy/url").toUrl();
    int index = m_ui->proxyTypeComboBox->findText(url.scheme(), Qt::MatchFixedString);
    m_ui->proxyTypeComboBox->setCurrentIndex(index);
    m_ui->hostLineEdit->setText(url.host());
    m_ui->portLineEdit->setText(url.port() >= 0 ? QString::number(url.port()) : QString());
    m_ui->userLineEdit->setText(url.userName());
    m_ui->passwordLineEdit->setText(url.password());
}

SettingsDialog::~SettingsDialog()
{
    delete m_ui;
}

void SettingsDialog::accept()
{
    QSettings settings;
    settings.setValue("Proxy/use_proxy", m_ui->enableProxyCheckBox->isChecked());
    QUrl url;
    url.setScheme(m_ui->proxyTypeComboBox->currentText().toLower());
    url.setHost(m_ui->hostLineEdit->text());
    url.setPort(m_ui->portLineEdit->text().toInt());
    url.setUserName(m_ui->userLineEdit->text());
    url.setPassword(m_ui->passwordLineEdit->text());
    settings.setValue("Proxy/url", url);
    QDialog::accept();
}
