#include "authdialog.h"
#include "registerdialog.h"
#include "storage.h"
#include <QVBoxLayout>
#include <QFormLayout>
#include <QPushButton>
#include <QLabel>
#include <QMessageBox>

AuthDialog::AuthDialog(QWidget *parent) : QDialog(parent)
{
    setWindowTitle("Вход — Биржа фриланса");
    setMinimumWidth(420);

    QVBoxLayout *root = new QVBoxLayout(this);
    QLabel *title = new QLabel("Биржа фриланса");
    title->setObjectName("titleLabel");
    QLabel *subtitle = new QLabel("Войдите в аккаунт и выберите роль");
    subtitle->setObjectName("mutedLabel");
    root->addWidget(title);
    root->addWidget(subtitle);

    QFormLayout *form = new QFormLayout();
    m_loginEdit = new QLineEdit();
    m_passwordEdit = new QLineEdit();
    m_passwordEdit->setEchoMode(QLineEdit::Password);
    m_roleBox = new QComboBox();
    m_roleBox->addItem("Покупатель", "buyer");
    m_roleBox->addItem("Фрилансер", "freelancer");
    form->addRow("Логин", m_loginEdit);
    form->addRow("Пароль", m_passwordEdit);
    form->addRow("Войти как", m_roleBox);
    root->addLayout(form);

    QPushButton *loginBtn = new QPushButton("Войти");
    QPushButton *regBtn = new QPushButton("Создать аккаунт");
    root->addWidget(loginBtn);
    root->addWidget(regBtn);

    connect(loginBtn, SIGNAL(clicked()), this, SLOT(onLogin()));
    connect(regBtn, SIGNAL(clicked()), this, SLOT(onRegister()));
}

UserAccount AuthDialog::user() const { return m_user; }

void AuthDialog::onLogin()
{
    QString err;
    const QString selectedRole = m_roleBox->currentData().toString();
    if (!Storage::authenticate(m_loginEdit->text().trimmed(), m_passwordEdit->text(), &m_user, &err)) {
        QMessageBox::warning(this, "Ошибка входа", err);
        return;
    }

    // Роль не принадлежит аккаунту. Это только режим текущего входа.
    // Поэтому один и тот же логин может открыть интерфейс покупателя или фрилансера.
    m_user.role = selectedRole;
    accept();
}

void AuthDialog::onRegister()
{
    RegisterDialog dlg(this);
    dlg.exec();
}
