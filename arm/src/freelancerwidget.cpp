#include "freelancerwidget.h"
#include "storage.h"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QPushButton>
#include <QPixmap>
#include <QDateTime>
#include <QMessageBox>
#include <QFileDialog>
#include <QFileInfo>
#include <QDesktopServices>
#include <QUrl>
#include <QTextBrowser>
#include <QSignalBlocker>
#include <QTimeEdit>
#include <QCheckBox>
#include <QRegExp>
#include <QTime>

static bool freelancerIsAnyTimeText(const QString &raw)
{
    QString s = raw.toLower().trimmed();
    s.replace(" ", "");
    return s == "круглосуточно" || s == "24/7" || s == "24\\7" ||
           s == "24часа" || s == "24ч" || s == "влюбоевремя" ||
           s == "любоевремя" || s == "00:00-24:00" || s == "0:00-24:00";
}

static bool freelancerParseTimeRange(const QString &raw, QTime *start, QTime *end)
{
    QString s = raw.toLower().trimmed();
    s.replace("—", "-");
    s.replace("–", "-");
    s.replace("до", "-");
    s.replace("с", "");
    QRegExp rx("(\\d{1,2})\\s*:\\s*(\\d{2})\\s*-\\s*(\\d{1,2})\\s*:\\s*(\\d{2})");
    if (rx.indexIn(s) < 0) return false;
    int h1 = rx.cap(1).toInt();
    int m1 = rx.cap(2).toInt();
    int h2 = rx.cap(3).toInt();
    int m2 = rx.cap(4).toInt();
    if (h1 < 0 || h1 > 23 || m1 < 0 || m1 > 59) return false;
    if (h2 == 24 && m2 == 0) {
        h2 = 23;
        m2 = 59;
    } else if (h2 < 0 || h2 > 23 || m2 < 0 || m2 > 59) {
        return false;
    }
    if (start) *start = QTime(h1, m1);
    if (end) *end = QTime(h2, m2);
    return true;
}


FreelancerWidget::FreelancerWidget(const UserAccount &user, QWidget *parent) : QWidget(parent), m_user(user)
{
    QHBoxLayout *root = new QHBoxLayout(this);

    QGroupBox *profileBox = new QGroupBox("Анкета фрилансера");
    QVBoxLayout *profileRoot = new QVBoxLayout(profileBox);
    QHBoxLayout *photoRow = new QHBoxLayout();
    m_photoLabel = new QLabel("Нет фото");
    m_photoLabel->setFixedSize(130, 130);
    m_photoLabel->setAlignment(Qt::AlignCenter);
    m_photoLabel->setStyleSheet("background:#E2E8F0;border-radius:14px;color:#64748B;");
    photoRow->addWidget(m_photoLabel);
    QPushButton *photoBtn = new QPushButton("Изменить фото");
    photoBtn->setObjectName("secondaryButton");
    photoRow->addWidget(photoBtn);
    photoRow->addStretch();
    profileRoot->addLayout(photoRow);

    QFormLayout *form = new QFormLayout();
    m_nameEdit = new QLineEdit();
    m_skillsEdit = new QLineEdit();
    m_rateSpin = new QDoubleSpinBox();
    m_rateSpin->setMaximum(10000000);
    m_rateSpin->setSuffix(" ₽");
    m_ratingLabel = new QLabel("0.0");
    m_completedLabel = new QLabel("0");
    m_workStartEdit = new QTimeEdit();
    m_workEndEdit = new QTimeEdit();
    m_workStartEdit->setDisplayFormat("HH:mm");
    m_workEndEdit->setDisplayFormat("HH:mm");
    m_workStartEdit->setTime(QTime(9, 0));
    m_workEndEdit->setTime(QTime(18, 0));
    m_anyTimeCheck = new QCheckBox("Круглосуточно");
    m_descriptionEdit = new QTextEdit();
    m_nameEdit->setPlaceholderText("например: Иван Петров");
    m_skillsEdit->setPlaceholderText("например: фотография, перевод, монтаж, дизайн");
    form->addRow("Имя", m_nameEdit);
    form->addRow("Навыки", m_skillsEdit);
    form->addRow("Ставка", m_rateSpin);
    form->addRow("Рейтинг", m_ratingLabel);
    form->addRow("Выполнено заказов", m_completedLabel);

    QWidget *workTimeWidget = new QWidget();
    QHBoxLayout *workTimeRow = new QHBoxLayout(workTimeWidget);
    workTimeRow->setContentsMargins(0, 0, 0, 0);
    workTimeRow->setSpacing(8);
    QLabel *dashLabel = new QLabel("-");
    dashLabel->setAlignment(Qt::AlignCenter);
    workTimeRow->addWidget(m_workStartEdit);
    workTimeRow->addWidget(dashLabel);
    workTimeRow->addWidget(m_workEndEdit);
    form->addRow("Время работы", workTimeWidget);
    form->addRow("", m_anyTimeCheck);
    form->addRow("Описание о себе", m_descriptionEdit);
    profileRoot->addLayout(form);

    QHBoxLayout *btnRow = new QHBoxLayout();
    QPushButton *saveBtn = new QPushButton("Сохранить анкету");
    QPushButton *clearBtn = new QPushButton("Очистить поля");
    clearBtn->setObjectName("secondaryButton");
    QPushButton *deleteFormBtn = new QPushButton("Удалить анкету");
    deleteFormBtn->setObjectName("dangerButton");
    btnRow->addWidget(saveBtn);
    btnRow->addWidget(clearBtn);
    btnRow->addWidget(deleteFormBtn);
    profileRoot->addLayout(btnRow);
    root->addWidget(profileBox, 1);

    QVBoxLayout *right = new QVBoxLayout();
    QGroupBox *buyersBox = new QGroupBox("Список покупателей");
    QVBoxLayout *buyersRoot = new QVBoxLayout(buyersBox);
    m_buyersList = new QListWidget();
    buyersRoot->addWidget(m_buyersList);
    QPushButton *refreshBtn = new QPushButton("Обновить список");
    refreshBtn->setObjectName("secondaryButton");
    buyersRoot->addWidget(refreshBtn);
    right->addWidget(buyersBox, 1);

    QGroupBox *chatBox = new QGroupBox("Чат с выбранным покупателем");
    QVBoxLayout *chatRoot = new QVBoxLayout(chatBox);
    m_chatTitle = new QLabel("Выберите покупателя из списка");
    m_chatTitle->setObjectName("sectionTitle");
    m_chatHistory = new QTextBrowser();
    m_chatHistory->setReadOnly(true);
    m_chatHistory->setOpenLinks(false);
    m_messageEdit = new QLineEdit();
    m_messageEdit->setPlaceholderText("Введите сообщение покупателю");
    m_attachBtn = new QPushButton("Прикрепить файл");
    m_attachBtn->setObjectName("secondaryButton");
    QPushButton *sendBtn = new QPushButton("Отправить");
    m_attachmentLabel = new QLabel();
    m_attachmentLabel->setObjectName("mutedLabel");
    m_attachmentLabel->setVisible(false);
    QHBoxLayout *sendRow = new QHBoxLayout();
    sendRow->addWidget(m_attachBtn);
    sendRow->addWidget(m_messageEdit, 1);
    sendRow->addWidget(sendBtn);
    chatRoot->addWidget(m_chatTitle);
    chatRoot->addWidget(m_chatHistory, 1);
    chatRoot->addWidget(m_attachmentLabel);
    chatRoot->addLayout(sendRow);
    right->addWidget(chatBox, 2);
    root->addLayout(right, 1);

    connect(photoBtn, SIGNAL(clicked()), this, SLOT(choosePhoto()));
    connect(saveBtn, SIGNAL(clicked()), this, SLOT(saveProfile()));
    connect(clearBtn, SIGNAL(clicked()), this, SLOT(clearFields()));
    connect(deleteFormBtn, SIGNAL(clicked()), this, SLOT(deleteProfileForm()));
    connect(refreshBtn, SIGNAL(clicked()), this, SLOT(refreshBuyers()));
    connect(m_buyersList, SIGNAL(itemSelectionChanged()), this, SLOT(onBuyerSelected()));
    connect(sendBtn, SIGNAL(clicked()), this, SLOT(sendMessage()));
    connect(m_attachBtn, SIGNAL(clicked()), this, SLOT(chooseAttachment()));
    connect(m_chatHistory, SIGNAL(anchorClicked(QUrl)), this, SLOT(openChatLink(QUrl)));
    connect(m_anyTimeCheck, SIGNAL(toggled(bool)), m_workStartEdit, SLOT(setDisabled(bool)));
    connect(m_anyTimeCheck, SIGNAL(toggled(bool)), m_workEndEdit, SLOT(setDisabled(bool)));

    m_timer = new QTimer(this);
    connect(m_timer, SIGNAL(timeout()), this, SLOT(refreshChat()));
    connect(m_timer, SIGNAL(timeout()), this, SLOT(refreshBuyers()));
    m_timer->start(3000);

    loadProfile();
    refreshBuyers();
    refreshChat();
}


QString FreelancerWidget::selectedWorkTime() const
{
    if (m_anyTimeCheck && m_anyTimeCheck->isChecked()) return "круглосуточно";
    if (!m_workStartEdit || !m_workEndEdit) return QString();
    return m_workStartEdit->time().toString("HH:mm") + " - " + m_workEndEdit->time().toString("HH:mm");
}

void FreelancerWidget::setWorkTimeFromString(const QString &value)
{
    if (freelancerIsAnyTimeText(value)) {
        m_anyTimeCheck->setChecked(true);
        return;
    }
    QTime start, end;
    if (freelancerParseTimeRange(value, &start, &end)) {
        m_anyTimeCheck->setChecked(false);
        m_workStartEdit->setTime(start);
        m_workEndEdit->setTime(end);
    }
}

void FreelancerWidget::loadProfile()
{
    m_photoPath = m_user.photoPath;
    FreelancerProfile p;
    if (Storage::getFreelancerProfile(m_user.login, &p)) {
        m_nameEdit->setText(p.name);
        m_skillsEdit->setText(p.skills);
        m_rateSpin->setValue(p.rate);
        m_ratingLabel->setText(QString::number(p.rating, 'f', 1));
        m_completedLabel->setText(QString::number(p.completedOrders));
        setWorkTimeFromString(p.workTime);
        m_descriptionEdit->setText(p.description);
        if (!p.photoPath.isEmpty()) m_photoPath = p.photoPath;
    } else {
        m_nameEdit->setText(m_user.displayName);
        m_ratingLabel->setText("0.0");
        m_completedLabel->setText("0");
    }
    updatePhotoPreview();
}

void FreelancerWidget::updatePhotoPreview()
{
    QPixmap pix(m_photoPath);
    if (!m_photoPath.isEmpty() && !pix.isNull()) {
        m_photoLabel->setPixmap(pix.scaled(130, 130, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation));
    } else {
        m_photoLabel->setPixmap(QPixmap());
        m_photoLabel->setText("Нет фото");
    }
}

void FreelancerWidget::choosePhoto()
{
    QString path = QFileDialog::getOpenFileName(this, "Выберите фото", QString(), "Изображения (*.png *.jpg *.jpeg *.bmp)");
    if (!path.isEmpty()) {
        m_photoPath = path;
        m_user.photoPath = path;
        Storage::updateUserPhoto(m_user.login, path);
        updatePhotoPreview();
    }
}

void FreelancerWidget::saveProfile()
{
    if (m_nameEdit->text().trimmed().isEmpty()) {
        QMessageBox::warning(this, "Ошибка", "Введите имя фрилансера");
        return;
    }
    FreelancerProfile old;
    double currentRating = 0.0;
    int currentCompleted = 0;
    if (Storage::getFreelancerProfile(m_user.login, &old)) {
        currentRating = old.rating;
        currentCompleted = old.completedOrders;
    }

    FreelancerProfile p;
    p.login = m_user.login;
    p.name = m_nameEdit->text().trimmed();
    p.skills = m_skillsEdit->text().trimmed();
    p.rate = m_rateSpin->value();
    p.rating = currentRating;
    p.completedOrders = currentCompleted;
    p.workTime = selectedWorkTime();
    p.description = m_descriptionEdit->toPlainText().trimmed();
    p.photoPath = m_photoPath;
    Storage::updateUserPhoto(m_user.login, m_photoPath);
    if (Storage::saveFreelancerProfile(p)) QMessageBox::information(this, "Готово", "Анкета сохранена");
    loadProfile();
}

void FreelancerWidget::clearFields()
{
    m_nameEdit->clear();
    m_skillsEdit->clear();
    m_rateSpin->setValue(0);
    m_workStartEdit->setTime(QTime(9, 0));
    m_workEndEdit->setTime(QTime(18, 0));
    m_anyTimeCheck->setChecked(false);
    m_descriptionEdit->clear();
}


void FreelancerWidget::deleteProfileForm()
{
    if (QMessageBox::question(this, "Удаление анкеты", "Удалить анкету фрилансера? Аккаунт останется.") != QMessageBox::Yes) return;
    if (Storage::deleteFreelancerProfile(m_user.login)) {
        clearFields();
        m_ratingLabel->setText("0.0");
        m_completedLabel->setText("0");
        QMessageBox::information(this, "Готово", "Анкета удалена");
    } else {
        QMessageBox::warning(this, "Ошибка", "Не удалось удалить анкету");
    }
}

QString FreelancerWidget::selectedBuyerLogin() const
{
    QListWidgetItem *it = m_buyersList->currentItem();
    if (!it) return QString();
    return it->data(Qt::UserRole).toString();
}

void FreelancerWidget::refreshBuyers()
{
    QString selected = selectedBuyerLogin();
    QSignalBlocker blocker(m_buyersList);
    m_buyersList->clear();
    QStringList buyers = Storage::buyersForFreelancer(m_user.login);
    for (int i=0;i<buyers.size();++i) {
        QListWidgetItem *it = new QListWidgetItem(Storage::displayNameForLogin(buyers[i]) + " (" + buyers[i] + ")");
        it->setData(Qt::UserRole, buyers[i]);
        m_buyersList->addItem(it);
        if (buyers[i] == selected) m_buyersList->setCurrentItem(it);
    }
    blocker.unblock();
    if (m_buyersList->currentItem()) onBuyerSelected();
    else refreshChat();
}

void FreelancerWidget::onBuyerSelected()
{
    QString buyer = selectedBuyerLogin();
    if (buyer.isEmpty()) m_chatTitle->setText("Выберите покупателя из списка");
    else m_chatTitle->setText("Чат с покупателем: " + Storage::displayNameForLogin(buyer));
    refreshChat();
}

void FreelancerWidget::chooseAttachment()
{
    QString path = QFileDialog::getOpenFileName(this, "Выберите файл");
    if (path.isEmpty()) return;
    QString copied = Storage::copyAttachmentToStorage(path);
    if (copied.isEmpty()) {
        QMessageBox::warning(this, "Ошибка", "Не удалось прикрепить файл");
        return;
    }
    m_pendingAttachmentPath = copied;
    m_pendingAttachmentName = QFileInfo(path).fileName();
    if (m_attachmentLabel) {
        m_attachmentLabel->setText("Прикреплен файл: " + m_pendingAttachmentName);
        m_attachmentLabel->setVisible(true);
    }
}

void FreelancerWidget::sendMessage()
{
    QString buyer = selectedBuyerLogin();
    if (buyer.isEmpty() || !Storage::hasActiveOrder(buyer, m_user.login)) return;
    const QString text = m_messageEdit->text().trimmed();
    if (text.isEmpty() && m_pendingAttachmentPath.isEmpty()) return;
    ChatMessage msg;
    msg.orderId = Storage::activeOrderId(buyer, m_user.login);
    msg.buyerLogin = buyer;
    msg.freelancerLogin = m_user.login;
    msg.senderLogin = m_user.login;
    msg.senderName = m_user.displayName;
    msg.text = text;
    msg.time = QDateTime::currentDateTime().toString("dd.MM.yyyy HH:mm");
    msg.attachmentName = m_pendingAttachmentName;
    msg.attachmentPath = m_pendingAttachmentPath;
    Storage::addMessage(msg);
    m_messageEdit->clear();
    m_pendingAttachmentPath.clear();
    m_pendingAttachmentName.clear();
    if (m_attachmentLabel) m_attachmentLabel->setVisible(false);
    refreshChat();
}

void FreelancerWidget::refreshChat()
{
    QString buyer = selectedBuyerLogin();
    if (buyer.isEmpty()) {
        m_chatHistory->setHtml("<p style='color:#64748B'>Выберите покупателя из списка</p>");
        if (m_messageEdit) m_messageEdit->setEnabled(false);
        if (m_attachBtn) m_attachBtn->setEnabled(false);
        m_pendingAttachmentPath.clear();
        m_pendingAttachmentName.clear();
        if (m_attachmentLabel) m_attachmentLabel->setVisible(false);
        return;
    }

    const bool active = Storage::hasActiveOrder(buyer, m_user.login);
    if (m_messageEdit) m_messageEdit->setEnabled(active);
    if (m_attachBtn) m_attachBtn->setEnabled(active);

    QVector<ChatMessage> msgs = Storage::messagesForPair(buyer, m_user.login);
    if (msgs.isEmpty()) {
        m_chatHistory->setHtml(active ? "" : "<p style='color:#64748B'>Нет активного заказа</p>");
        m_pendingAttachmentPath.clear();
        m_pendingAttachmentName.clear();
        if (m_attachmentLabel) m_attachmentLabel->setVisible(false);
        return;
    }

    QString html;
    for (int i=0;i<msgs.size();++i) {
        if (msgs[i].senderLogin == "__system__") {
            html += "<div style='margin:10px 0; padding:8px 12px; background:#E0F2FE; border-radius:10px; color:#1E3A8A; font-weight:700; text-align:center;'>"
                    + msgs[i].text.toHtmlEscaped() +
                    "<br><span style='font-size:11px; color:#64748B; font-weight:400;'>" + msgs[i].time.toHtmlEscaped() + "</span></div>";
            continue;
        }
        QString sender = Storage::isUserDeleted(msgs[i].senderLogin) ? QString("Профиль удален") : msgs[i].senderName;
        QString text = msgs[i].text.toHtmlEscaped();
        if (!msgs[i].attachmentPath.isEmpty()) {
            QString url = QUrl::fromLocalFile(msgs[i].attachmentPath).toString();
            QString name = msgs[i].attachmentName.isEmpty() ? QString("файл") : msgs[i].attachmentName;
            text += "<br><a href='" + url.toHtmlEscaped() + "'>📎 " + name.toHtmlEscaped() + "</a>";
        }
        html += "<p><b>" + sender.toHtmlEscaped() + ":</b> " + text + "<br><span style='color:#64748B'>" + msgs[i].time.toHtmlEscaped() + "</span></p>";
    }
    m_chatHistory->setHtml(html);
}


void FreelancerWidget::openChatLink(const QUrl &url)
{
    if (!QDesktopServices::openUrl(url)) {
        QMessageBox::warning(this, "Ошибка", "Не удалось открыть файл");
    }
}
