#include "registerdialog.h"
#include "storage.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QPushButton>
#include <QLabel>
#include <QMessageBox>
#include <QFileDialog>
#include <QPixmap>

RegisterDialog::RegisterDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle("Регистрация — Биржа фриланса");
    setMinimumWidth(420);

    QVBoxLayout *root = new QVBoxLayout(this);
    QLabel *title = new QLabel("Создание аккаунта");
    title->setObjectName("titleLabel");
    root->addWidget(title);

    QFormLayout *form = new QFormLayout();
    m_nameEdit = new QLineEdit();
    m_loginEdit = new QLineEdit();
    m_passwordEdit = new QLineEdit();
    m_passwordEdit->setEchoMode(QLineEdit::Password);
    m_repeatEdit = new QLineEdit();
    m_repeatEdit->setEchoMode(QLineEdit::Password);

    form->addRow("Имя пользователя", m_nameEdit);
    form->addRow("Логин", m_loginEdit);
    form->addRow("Пароль", m_passwordEdit);
    form->addRow("Повтор пароля", m_repeatEdit);
    root->addLayout(form);

    QHBoxLayout *photoRow = new QHBoxLayout();
    m_photoLabel = new QLabel("Нет фото");
    m_photoLabel->setFixedSize(90, 90);
    m_photoLabel->setAlignment(Qt::AlignCenter);
    m_photoLabel->setStyleSheet("background:#E2E8F0;border-radius:12px;color:#64748B;");
    QPushButton *photoBtn = new QPushButton("Выбрать фото");
    photoBtn->setObjectName("secondaryButton");
    photoRow->addWidget(m_photoLabel);
    photoRow->addWidget(photoBtn);
    photoRow->addStretch();
    root->addLayout(photoRow);

    QPushButton *btn = new QPushButton("Зарегистрироваться");
    QPushButton *back = new QPushButton("Назад ко входу");
    root->addWidget(btn);
    root->addWidget(back);

    connect(photoBtn, SIGNAL(clicked()), this, SLOT(choosePhoto()));
    connect(btn, SIGNAL(clicked()), this, SLOT(onRegister()));
    connect(back, SIGNAL(clicked()), this, SLOT(reject()));
}

void RegisterDialog::choosePhoto()
{
    QString path = QFileDialog::getOpenFileName(this, "Выберите фото", QString(), "Изображения (*.png *.jpg *.jpeg *.bmp)");
    if (!path.isEmpty()) {
        m_photoPath = path;
        updatePhotoPreview();
    }
}

void RegisterDialog::updatePhotoPreview()
{
    QPixmap pix(m_photoPath);
    if (!m_photoPath.isEmpty() && !pix.isNull()) {
        m_photoLabel->setPixmap(pix.scaled(90, 90, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation));
    } else {
        m_photoLabel->setPixmap(QPixmap());
        m_photoLabel->setText("Нет фото");
    }
}

void RegisterDialog::onRegister()
{
    QString name = m_nameEdit->text().trimmed();
    QString login = m_loginEdit->text().trimmed();
    QString pass = m_passwordEdit->text();
    QString rep = m_repeatEdit->text();
    if (name.isEmpty() || login.isEmpty() || pass.isEmpty()) {
        QMessageBox::warning(this, "Ошибка", "Заполните все поля");
        return;
    }
    if (pass != rep) {
        QMessageBox::warning(this, "Ошибка", "Пароли не совпадают");
        return;
    }
    UserAccount u;
    u.displayName = name;
    u.login = login;
    u.passwordHash = Storage::hashPassword(pass);
    u.photoPath = m_photoPath;
    QString err;
    if (!Storage::addUser(u, &err)) {
        QMessageBox::warning(this, "Ошибка", err);
        return;
    }
    QMessageBox::information(this, "Готово", "Аккаунт создан. Теперь можно войти.");
    accept();
}
