#include "buyerwidget.h"
#include "storage.h"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QDateTime>
#include <QPixmap>
#include <QMessageBox>
#include <QLayoutItem>
#include <QIcon>
#include <QInputDialog>
#include <QSize>
#include <QFrame>
#include <QSignalBlocker>
#include <QApplication>
#include <QPalette>
#include <QColor>
#include <QStringList>
#include <QRegExp>
#include <QTextBrowser>
#include <QFileDialog>
#include <QFileInfo>
#include <QDesktopServices>
#include <QUrl>
#include <QTime>
#include <QTimeEdit>
#include <QCheckBox>
#include <algorithm>


static bool isAnyTimeText(const QString &raw)
{
    QString s = raw.toLower().trimmed();
    s.replace(" ", "");
    if (s.isEmpty()) return false;
    return s == "круглосуточно" ||
           s == "24/7" ||
           s == "24\\7" ||
           s == "24часа" ||
           s == "24ч" ||
           s == "24:00" ||
           s == "влюбоевремя" ||
           s == "любоевремя" ||
           s == "любое" ||
           s == "00:00-24:00" ||
           s == "0:00-24:00" ||
           s == "00:00-00:00";
}

static bool parseTimeRange(const QString &raw, int *startMinutes, int *endMinutes)
{
    QString s = raw.toLower().trimmed();
    if (s.isEmpty()) return false;
    s.replace("—", "-");
    s.replace("–", "-");
    s.replace("до", "-");
    s.replace("с", "");
    s = s.trimmed();

    QRegExp rx("(\\d{1,2})\\s*:\\s*(\\d{2})\\s*-\\s*(\\d{1,2})\\s*:\\s*(\\d{2})");
    if (rx.indexIn(s) < 0) return false;

    int h1 = rx.cap(1).toInt();
    int m1 = rx.cap(2).toInt();
    int h2 = rx.cap(3).toInt();
    int m2 = rx.cap(4).toInt();

    if (h1 < 0 || h1 > 23 || m1 < 0 || m1 > 59) return false;
    if (h2 == 24 && m2 == 0) {
        // 24:00 допустимо как конец суток.
    } else if (h2 < 0 || h2 > 23 || m2 < 0 || m2 > 59) {
        return false;
    }

    int a = h1 * 60 + m1;
    int b = (h2 == 24 ? 24 * 60 : h2 * 60 + m2);
    if (b <= a) b += 24 * 60; // смена через полночь
    if (startMinutes) *startMinutes = a;
    if (endMinutes) *endMinutes = b;
    return true;
}

static bool workTimeCoversTaskTime(const QString &freelancerTime, const QString &taskTime)
{
    QString need = taskTime.trimmed();
    if (need.isEmpty() || isAnyTimeText(need)) return true;
    if (isAnyTimeText(freelancerTime)) return true;

    int needStart = 0, needEnd = 0;
    if (!parseTimeRange(need, &needStart, &needEnd)) {
        // Если покупатель ввел время в непонятном формате, не скрываем всех исполнителей.
        return true;
    }

    int workStart = 0, workEnd = 0;
    if (!parseTimeRange(freelancerTime, &workStart, &workEnd)) {
        return false;
    }

    // Обычное сравнение.
    if (workStart <= needStart && workEnd >= needEnd) return true;

    // Если рабочее время переходит через полночь, проверяем задачу на следующий день.
    if (workEnd > 24 * 60) {
        int shiftedNeedStart = needStart + 24 * 60;
        int shiftedNeedEnd = needEnd + 24 * 60;
        if (workStart <= shiftedNeedStart && workEnd >= shiftedNeedEnd) return true;
    }

    return false;
}

static int editDistance(const QString &aRaw, const QString &bRaw)
{
    QString a = aRaw.toLower().trimmed();
    QString b = bRaw.toLower().trimmed();
    const int n = a.size();
    const int m = b.size();
    if (n == 0) return m;
    if (m == 0) return n;
    QVector<int> prev(m + 1), cur(m + 1);
    for (int j = 0; j <= m; ++j) prev[j] = j;
    for (int i = 1; i <= n; ++i) {
        cur[0] = i;
        for (int j = 1; j <= m; ++j) {
            int cost = a[i - 1] == b[j - 1] ? 0 : 1;
            cur[j] = qMin(qMin(prev[j] + 1, cur[j - 1] + 1), prev[j - 1] + cost);
        }
        prev = cur;
    }
    return prev[m];
}

static QStringList skillTokens(const QString &text)
{
    QString normalized = text.toLower();
    normalized.replace(',', ' ');
    normalized.replace(';', ' ');
    normalized.replace('/', ' ');
    normalized.replace('|', ' ');
    QStringList tokens = normalized.split(QRegExp("\\s+"), QString::SkipEmptyParts);
    return tokens;
}

static int skillMatchScore(const QString &needRaw, const QString &skillsRaw)
{
    QString need = needRaw.toLower().trimmed();
    QString skills = skillsRaw.toLower().trimmed();
    if (need.isEmpty()) return 1;
    if (skills.isEmpty()) return 0;

    if (skills == need) return 1000;
    if (skills.contains(need)) return 900;

    QStringList needTokens = skillTokens(need);
    QStringList skillTokensList = skillTokens(skills);
    int best = 0;

    for (int i = 0; i < needTokens.size(); ++i) {
        for (int j = 0; j < skillTokensList.size(); ++j) {
            const QString &n = needTokens[i];
            const QString &s = skillTokensList[j];
            if (n == s) best = qMax(best, 800);
            else if (s.contains(n) || n.contains(s)) best = qMax(best, 650);
            else {
                int d = editDistance(n, s);
                int limit = qMax(1, qMin(3, qMax(n.size(), s.size()) / 3));
                if (d <= limit) best = qMax(best, 600 - d * 80);
            }
        }
    }

    int phraseDistance = editDistance(need, skills);
    int phraseLimit = qMax(2, qMin(5, qMax(need.size(), skills.size()) / 4));
    if (phraseDistance <= phraseLimit) best = qMax(best, 500 - phraseDistance * 30);

    return best;
}

BuyerWidget::BuyerWidget(const UserAccount &user, QWidget *parent) : QWidget(parent), m_user(user), m_rebuildingCard(false)
{
    m_chatHistory = 0;
    m_messageEdit = 0;
    m_sendBtn = 0;
    m_attachBtn = 0;
    m_attachmentLabel = 0;
    m_placeholderLabel = 0;
    m_infoFrame = 0;
    m_selectedPhotoLabel = 0;
    m_selectedInfoLabel = 0;
    m_selfLabel = 0;
    m_orderWidget = 0;
    m_startBtn = 0;
    m_finishBtn = 0;
    m_lockedLabel = 0;
    m_chatTitle = 0;
    m_chatWidget = 0;

    QHBoxLayout *root = new QHBoxLayout(this);
    root->setSpacing(14);

    QGroupBox *taskBox = new QGroupBox("Задача покупателя");
    QVBoxLayout *taskRoot = new QVBoxLayout(taskBox);
    QFormLayout *form = new QFormLayout();
    m_titleEdit = 0;
    m_skillEdit = new QLineEdit();
    m_budgetSpin = new QDoubleSpinBox();
    m_budgetSpin->setMaximum(10000000);
    m_budgetSpin->setSuffix(" ₽");
    m_taskStartTimeEdit = new QTimeEdit();
    m_taskEndTimeEdit = new QTimeEdit();
    m_taskAnyTimeCheck = new QCheckBox("Круглосуточно");
    m_taskStartTimeEdit->setDisplayFormat("HH:mm");
    m_taskEndTimeEdit->setDisplayFormat("HH:mm");
    m_taskStartTimeEdit->setTime(QTime(9, 0));
    m_taskEndTimeEdit->setTime(QTime(18, 0));
    m_skillEdit->setPlaceholderText("например: фотография, перевод, монтаж, дизайн");
    form->addRow("Нужный навык", m_skillEdit);
    form->addRow("Бюджет", m_budgetSpin);
    QWidget *taskTimeWidget = new QWidget();
    QHBoxLayout *taskTimeRow = new QHBoxLayout(taskTimeWidget);
    taskTimeRow->setContentsMargins(0, 0, 0, 0);
    taskTimeRow->setSpacing(8);
    QLabel *taskDashLabel = new QLabel("-");
    taskDashLabel->setAlignment(Qt::AlignCenter);
    taskTimeRow->addWidget(m_taskStartTimeEdit);
    taskTimeRow->addWidget(taskDashLabel);
    taskTimeRow->addWidget(m_taskEndTimeEdit);
    taskTimeRow->addWidget(m_taskAnyTimeCheck);
    form->addRow("Нужное время", taskTimeWidget);
    taskRoot->addLayout(form);
    QPushButton *saveTaskBtn = new QPushButton("Сохранить задачу и найти исполнителей");
    taskRoot->addWidget(saveTaskBtn);

    QGroupBox *listBox = new QGroupBox("Список фрилансеров");
    QVBoxLayout *listRoot = new QVBoxLayout(listBox);
    m_freelancerList = new QListWidget();
    m_freelancerList->setObjectName("freelancerCompactList");
    m_freelancerList->setIconSize(QSize(48, 48));
    m_freelancerList->setSpacing(4);
    m_freelancerList->setMinimumHeight(390);
    m_freelancerList->setStyleSheet("QListWidget#freelancerCompactList { padding: 6px; } QListWidget#freelancerCompactList::item { padding: 4px; min-height: 64px; }");
    listRoot->addWidget(m_freelancerList, 1);
    QPushButton *refreshBtn = new QPushButton("Обновить список");
    refreshBtn->setObjectName("secondaryButton");
    listRoot->addWidget(refreshBtn);

    QVBoxLayout *left = new QVBoxLayout();
    left->addWidget(taskBox);
    left->addWidget(listBox, 1);
    root->addLayout(left, 1);

    QGroupBox *rightBox = new QGroupBox("Выбранный исполнитель и чат");
    QVBoxLayout *rightRoot = new QVBoxLayout(rightBox);
    m_cardWidget = new QWidget();
    m_cardWidget->setObjectName("selectedFreelancerCard");
    m_cardWidget->setAttribute(Qt::WA_StyledBackground, true);
    m_cardLayout = new QVBoxLayout(m_cardWidget);
    m_cardLayout->setContentsMargins(16, 16, 16, 16);
    m_cardLayout->setSpacing(10);
    // Все содержимое карточки прижато к верху. Это убирает большие пустые
    // отступы в состояниях "Это вы" и "Начните заказ, чтобы открыть чат".
    m_cardLayout->setAlignment(Qt::AlignTop);

    m_placeholderLabel = new QLabel("Выберите фрилансера");
    m_placeholderLabel->setObjectName("titleLabel");
    m_placeholderLabel->setAlignment(Qt::AlignCenter);
    m_cardLayout->addWidget(m_placeholderLabel);

    m_infoFrame = new QFrame();
    m_infoFrame->setObjectName("freelancerInfoFrame");
    m_infoFrame->setAttribute(Qt::WA_StyledBackground, true);
    QHBoxLayout *top = new QHBoxLayout(m_infoFrame);
    top->setContentsMargins(0, 0, 0, 0);
    top->setSpacing(12);
    m_selectedPhotoLabel = makePhotoLabel(QString(), 120, 120);
    m_selectedInfoLabel = new QLabel();
    m_selectedInfoLabel->setWordWrap(true);
    top->addWidget(m_selectedPhotoLabel);
    top->addWidget(m_selectedInfoLabel, 1);
    m_cardLayout->addWidget(m_infoFrame);

    m_selfLabel = new QLabel("Это вы");
    m_selfLabel->setObjectName("sectionTitle");
    m_selfLabel->setAlignment(Qt::AlignCenter);
    m_selfLabel->setFixedHeight(36);
    m_selfLabel->setStyleSheet("background: transparent; color: #1E3A8A; font-weight: 700;");
    m_cardLayout->addWidget(m_selfLabel, 0, Qt::AlignTop);

    m_orderWidget = new QWidget();
    QHBoxLayout *orderRow = new QHBoxLayout(m_orderWidget);
    orderRow->setContentsMargins(0, 0, 0, 0);
    m_startBtn = new QPushButton("Начать заказ");
    m_finishBtn = new QPushButton("Завершить заказ");
    m_finishBtn->setObjectName("secondaryButton");
    orderRow->addWidget(m_startBtn);
    orderRow->addWidget(m_finishBtn);
    m_cardLayout->addWidget(m_orderWidget);

    m_lockedLabel = new QLabel("Начните заказ, чтобы открыть чат");
    m_lockedLabel->setObjectName("sectionTitle");
    m_lockedLabel->setAlignment(Qt::AlignCenter);
    m_cardLayout->addWidget(m_lockedLabel);

    m_chatWidget = new QWidget();
    QVBoxLayout *chatLayout = new QVBoxLayout(m_chatWidget);
    chatLayout->setContentsMargins(0, 0, 0, 0);
    chatLayout->setSpacing(8);
    m_chatTitle = new QLabel("Чат");
    m_chatTitle->setObjectName("sectionTitle");
    m_chatHistory = new QTextBrowser();
    m_chatHistory->setReadOnly(true);
    m_chatHistory->setMinimumHeight(260);
    m_chatHistory->setMaximumHeight(360);
    m_chatHistory->setOpenLinks(false);
    m_messageEdit = new QLineEdit();
    m_messageEdit->setPlaceholderText("Введите сообщение фрилансеру");
    m_attachBtn = new QPushButton("Прикрепить файл");
    m_attachBtn->setObjectName("secondaryButton");
    m_sendBtn = new QPushButton("Отправить");
    m_attachmentLabel = new QLabel();
    m_attachmentLabel->setObjectName("mutedLabel");
    m_attachmentLabel->setVisible(false);
    QHBoxLayout *sendRow = new QHBoxLayout();
    sendRow->addWidget(m_attachBtn);
    sendRow->addWidget(m_messageEdit, 1);
    sendRow->addWidget(m_sendBtn);
    chatLayout->addWidget(m_chatTitle);
    chatLayout->addWidget(m_chatHistory, 1);
    chatLayout->addWidget(m_attachmentLabel);
    chatLayout->addLayout(sendRow);
    m_cardLayout->addWidget(m_chatWidget);
    m_cardLayout->addStretch(1);

    rightRoot->addWidget(m_cardWidget, 1);
    root->addWidget(rightBox, 1);

    connect(saveTaskBtn, SIGNAL(clicked()), this, SLOT(saveTaskAndRefresh()));
    connect(refreshBtn, SIGNAL(clicked()), this, SLOT(delayedRefreshFreelancers()));
    connect(m_freelancerList, SIGNAL(itemSelectionChanged()), this, SLOT(onFreelancerSelected()));
    connect(m_startBtn, SIGNAL(clicked()), this, SLOT(startOrder()));
    connect(m_finishBtn, SIGNAL(clicked()), this, SLOT(completeOrder()));
    connect(m_sendBtn, SIGNAL(clicked()), this, SLOT(sendMessage()));
    connect(m_attachBtn, SIGNAL(clicked()), this, SLOT(chooseAttachment()));
    connect(m_chatHistory, SIGNAL(anchorClicked(QUrl)), this, SLOT(openChatLink(QUrl)));
    connect(m_taskAnyTimeCheck, &QCheckBox::toggled, this, [this](bool checked) {
        if (m_taskStartTimeEdit) m_taskStartTimeEdit->setEnabled(!checked);
        if (m_taskEndTimeEdit) m_taskEndTimeEdit->setEnabled(!checked);
    });

    m_timer = new QTimer(this);
    connect(m_timer, SIGNAL(timeout()), this, SLOT(refreshChat()));
    m_timer->start(3000);

    resetCardColors();
    refreshFreelancers();
    showNoFreelancerSelected();
}

QLabel *BuyerWidget::makePhotoLabel(const QString &path, int w, int h)
{
    QLabel *label = new QLabel();
    label->setFixedSize(w, h);
    label->setAlignment(Qt::AlignCenter);
    label->setStyleSheet("background:#E2E8F0;border-radius:12px;color:#64748B;");
    QPixmap pix(path);
    if (!path.isEmpty() && !pix.isNull()) label->setPixmap(pix.scaled(w, h, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation));
    else label->setText("Нет фото");
    return label;
}

void BuyerWidget::saveTaskAndRefresh()
{
    const QString skill = m_skillEdit->text().trimmed();
    const QString timeRange = selectedTaskTimeRange();
    if (!skill.isEmpty() || m_budgetSpin->value() > 0 || !timeRange.isEmpty()) {
        TaskInfo t;
        t.buyerLogin = m_user.login;
        t.title = currentTaskTitle();
        t.skill = skill;
        t.budget = m_budgetSpin->value();
        t.timeRange = timeRange;
        Storage::addTask(t);
    }
    refreshFreelancers();
}

bool BuyerWidget::matchesTask(const FreelancerProfile &p) const
{
    QString skill = m_skillEdit->text().trimmed();
    if (!skill.isEmpty() && skillMatchScore(skill, p.skills) <= 0) return false;
    if (m_budgetSpin->value() > 0 && p.rate > m_budgetSpin->value()) return false;
    if (!workTimeCoversTaskTime(p.workTime, selectedTaskTimeRange())) return false;
    return true;
}

void BuyerWidget::refreshFreelancers()
{
    QString selected = selectedFreelancerLogin();
    QSignalBlocker blocker(m_freelancerList);
    m_freelancerList->clear();
    QVector<FreelancerProfile> items = Storage::loadFreelancers();
    QString needSkill = m_skillEdit->text().trimmed();
    QString needTime = selectedTaskTimeRange();
    std::sort(items.begin(), items.end(), [needSkill, needTime](const FreelancerProfile &a, const FreelancerProfile &b) {
        int scoreA = skillMatchScore(needSkill, a.skills);
        int scoreB = skillMatchScore(needSkill, b.skills);
        if (scoreA != scoreB) return scoreA > scoreB;
        bool timeA = workTimeCoversTaskTime(a.workTime, needTime);
        bool timeB = workTimeCoversTaskTime(b.workTime, needTime);
        if (timeA != timeB) return timeA;
        if (a.rating == b.rating) return a.rate < b.rate;
        return a.rating > b.rating;
    });
    for (int i=0;i<items.size();++i) {
        if (items[i].login == m_user.login) continue;
        if (items[i].name.trimmed().isEmpty()) continue;
        if (!matchesTask(items[i])) continue;
        QString text = items[i].name + "\n" +
                "Навыки: " + items[i].skills + " | " +
                "Ставка: " + QString::number(items[i].rate, 'f', 0) + " ₽ | Рейтинг: " + QString::number(items[i].rating, 'f', 1);
        QListWidgetItem *it = new QListWidgetItem(text);
        it->setSizeHint(QSize(0, 68));
        QPixmap pix(items[i].photoPath);
        if (!pix.isNull()) it->setIcon(QIcon(pix.scaled(48, 48, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation)));
        it->setData(Qt::UserRole, items[i].login);
        m_freelancerList->addItem(it);
        if (items[i].login == selected) m_freelancerList->setCurrentItem(it);
    }
    blocker.unblock();
    if (m_freelancerList->currentItem()) onFreelancerSelected();
    else showNoFreelancerSelected();
}

QString BuyerWidget::selectedFreelancerLogin() const
{
    QListWidgetItem *it = m_freelancerList->currentItem();
    if (!it) return QString();
    return it->data(Qt::UserRole).toString();
}

QString BuyerWidget::currentTaskTitle() const
{
    QString skill = m_skillEdit ? m_skillEdit->text().trimmed() : QString();
    if (!skill.isEmpty()) return "Заказ: " + skill;
    return "Заказ покупателя";
}

QString BuyerWidget::selectedTaskTimeRange() const
{
    if (m_taskAnyTimeCheck && m_taskAnyTimeCheck->isChecked()) return "круглосуточно";
    if (!m_taskStartTimeEdit || !m_taskEndTimeEdit) return QString();
    return m_taskStartTimeEdit->time().toString("HH:mm") + " - " + m_taskEndTimeEdit->time().toString("HH:mm");
}

void BuyerWidget::resetCardColors()
{
    m_cardWidget->setStyleSheet(
        "QWidget#selectedFreelancerCard { background-color: #FFFFFF; border: none; }"
        "QFrame#freelancerInfoFrame { background-color: #FFFFFF; border: none; }"
        "QWidget#selectedFreelancerCard QLabel { background: transparent; color: #1F2937; }"
        "QWidget#selectedFreelancerCard QTextEdit, QWidget#selectedFreelancerCard QLineEdit { background-color: #FFFFFF; color: #1F2937; border: 1px solid #CBD5E1; border-radius: 10px; padding: 8px; }"
        "QWidget#selectedFreelancerCard QPushButton { background-color: #2563EB; color: white; border: none; border-radius: 12px; padding: 10px 16px; font-weight: 700; }"
        "QWidget#selectedFreelancerCard QPushButton#secondaryButton { background-color: #E2E8F0; color: #1E293B; }"
        "QWidget#selectedFreelancerCard QPushButton:disabled { background-color: #E2E8F0; color: #64748B; }"
    );
    QPalette cardPalette = m_cardWidget->palette();
    cardPalette.setColor(QPalette::Window, QColor("#FFFFFF"));
    m_cardWidget->setPalette(cardPalette);
    m_cardWidget->setAutoFillBackground(true);
    if (m_infoFrame) {
        QPalette p = m_infoFrame->palette();
        p.setColor(QPalette::Window, QColor("#FFFFFF"));
        m_infoFrame->setPalette(p);
        m_infoFrame->setAutoFillBackground(true);
    }
}

void BuyerWidget::setPhoto(QLabel *label, const QString &path, int w, int h)
{
    if (!label) return;
    label->setFixedSize(w, h);
    label->setAlignment(Qt::AlignCenter);
    label->setStyleSheet("background:#E2E8F0;border-radius:12px;color:#64748B;");
    QPixmap pix(path);
    if (!path.isEmpty() && !pix.isNull()) {
        label->setText(QString());
        label->setPixmap(pix.scaled(w, h, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation));
    } else {
        label->setPixmap(QPixmap());
        label->setText("Нет фото");
    }
}

void BuyerWidget::setChatVisible(bool visible)
{
    if (m_chatWidget) m_chatWidget->setVisible(visible);
    if (!visible && m_chatHistory) m_chatHistory->clear();
    if (!visible) {
        m_pendingAttachmentPath.clear();
        m_pendingAttachmentName.clear();
        if (m_attachmentLabel) m_attachmentLabel->setVisible(false);
    }
}

void BuyerWidget::setOrderControlsVisible(bool visible)
{
    if (m_orderWidget) m_orderWidget->setVisible(visible);
}

void BuyerWidget::showNoFreelancerSelected()
{
    m_selectedLogin.clear();
    resetCardColors();
    if (m_placeholderLabel) {
        m_placeholderLabel->setText("Выберите фрилансера");
        m_placeholderLabel->setVisible(true);
    }
    if (m_infoFrame) m_infoFrame->setVisible(false);
    if (m_selfLabel) m_selfLabel->setVisible(false);
    setOrderControlsVisible(false);
    if (m_lockedLabel) m_lockedLabel->setVisible(false);
    setChatVisible(false);
}

void BuyerWidget::showChatLocked(const QString &text)
{
    if (m_lockedLabel) {
        m_lockedLabel->setText(text);
        m_lockedLabel->setVisible(true);
    }
    setChatVisible(false);
}

void BuyerWidget::showChatArea(bool inputEnabled)
{
    if (m_lockedLabel) m_lockedLabel->setVisible(false);
    setChatVisible(true);
    if (m_messageEdit) m_messageEdit->setEnabled(inputEnabled);
    if (m_attachBtn) m_attachBtn->setEnabled(inputEnabled);
    if (m_sendBtn) m_sendBtn->setEnabled(inputEnabled);
    refreshChat();
}

void BuyerWidget::updateFreelancerCard(const FreelancerProfile &p)
{
    resetCardColors();
    if (m_placeholderLabel) m_placeholderLabel->setVisible(false);
    if (m_infoFrame) m_infoFrame->setVisible(true);
    setPhoto(m_selectedPhotoLabel, p.photoPath, 120, 120);
    if (m_selectedInfoLabel) {
        m_selectedInfoLabel->setText(
            "<b>" + p.name.toHtmlEscaped() + "</b><br>"
            "Навыки: " + p.skills.toHtmlEscaped() + "<br>"
            "Ставка: " + QString::number(p.rate, 'f', 0) + " ₽<br>"
            "Рейтинг: " + QString::number(p.rating, 'f', 1) + "<br>"
            "Выполнено заказов: " + QString::number(p.completedOrders) + "<br>"
            "Часы работы: " + p.workTime.toHtmlEscaped() + "<br>"
            "Описание: " + p.description.toHtmlEscaped()
        );
    }

    QString fl = selectedFreelancerLogin();
    m_selectedLogin = fl;
    if (fl == m_user.login) {
        if (m_selfLabel) {
            m_selfLabel->setText("Это вы");
            m_selfLabel->setVisible(true);
        }
        setOrderControlsVisible(false);
        if (m_lockedLabel) m_lockedLabel->setVisible(false);
        setChatVisible(false);
        return;
    }

    if (m_selfLabel) m_selfLabel->setVisible(false);
    setOrderControlsVisible(true);
    const bool active = !fl.isEmpty() && Storage::hasActiveOrder(m_user.login, fl);
    const bool hasHistory = !fl.isEmpty() && !Storage::latestOrderId(m_user.login, fl).isEmpty();
    if (m_startBtn) m_startBtn->setEnabled(!active);
    if (m_finishBtn) m_finishBtn->setEnabled(active);

    // Если заказ активен — чат открыт и можно писать.
    // Если заказ уже закрыт — показываем сохраненную историю чата,
    // включая системное сообщение "Заказ закрыт", но ввод сообщений блокируем.
    // Если заказа еще не было — показываем старую подсказку.
    if (active) showChatArea(true);
    else if (hasHistory) showChatArea(false);
    else showChatLocked("Начните заказ, чтобы открыть чат");
}

void BuyerWidget::onFreelancerSelected()
{
    FreelancerProfile p;
    QString fl = selectedFreelancerLogin();
    if (!fl.isEmpty() && Storage::getFreelancerProfile(fl, &p)) updateFreelancerCard(p);
    else showNoFreelancerSelected();
}

void BuyerWidget::startOrder()
{
    const QString fl = selectedFreelancerLogin();
    if (fl.isEmpty() || fl == m_user.login) return;
    if (Storage::startOrder(m_user.login, fl, currentTaskTitle())) {
        rebuildCurrentCard();
    }
}

void BuyerWidget::completeOrder()
{
    QString fl = selectedFreelancerLogin();
    if (fl.isEmpty() || !Storage::hasActiveOrder(m_user.login, fl)) return;
    bool ok = false;
    int rating = QInputDialog::getInt(this, "Оценка заказа", "Поставьте оценку фрилансеру от 1 до 5", 5, 1, 5, 1, &ok);
    if (!ok) return;
    if (Storage::completeOrder(m_user.login, fl, rating)) {
        QMessageBox::information(this, "Готово", "Заказ завершен. Оценка сохранена.");
        refreshFreelancers();
        if (!fl.isEmpty()) {
            for (int i = 0; i < m_freelancerList->count(); ++i) {
                QListWidgetItem *it = m_freelancerList->item(i);
                if (it && it->data(Qt::UserRole).toString() == fl) {
                    m_freelancerList->setCurrentItem(it);
                    break;
                }
            }
            rebuildCurrentCard();
            if (m_startBtn) m_startBtn->setEnabled(true);
            if (m_finishBtn) m_finishBtn->setEnabled(false);
            showChatArea(false);
        }
    }
}

void BuyerWidget::chooseAttachment()
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

void BuyerWidget::sendMessage()
{
    QString fl = selectedFreelancerLogin();
    if (fl.isEmpty() || !Storage::hasActiveOrder(m_user.login, fl) || !m_messageEdit) return;
    const QString text = m_messageEdit->text().trimmed();
    if (text.isEmpty() && m_pendingAttachmentPath.isEmpty()) return;
    ChatMessage msg;
    msg.orderId = Storage::activeOrderId(m_user.login, fl);
    if (msg.orderId.isEmpty()) return;
    msg.buyerLogin = m_user.login;
    msg.freelancerLogin = fl;
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

void BuyerWidget::rebuildCurrentCard()
{
    onFreelancerSelected();
}

void BuyerWidget::delayedRefreshFreelancers()
{
    refreshFreelancers();
}

void BuyerWidget::refreshChat()
{
    if (!m_chatWidget || !m_chatWidget->isVisible()) return;
    if (!m_chatHistory) return;
    QString fl = selectedFreelancerLogin();
    if (fl.isEmpty()) {
        m_chatHistory->clear();
        return;
    }
    const bool active = Storage::hasActiveOrder(m_user.login, fl);
    if (m_messageEdit) m_messageEdit->setEnabled(active);
    if (m_attachBtn) m_attachBtn->setEnabled(active);
    if (m_sendBtn) m_sendBtn->setEnabled(active);
    QVector<ChatMessage> msgs = Storage::messagesForPair(m_user.login, fl);
    if (msgs.isEmpty()) {
        m_chatHistory->clear();
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


void BuyerWidget::openChatLink(const QUrl &url)
{
    if (!QDesktopServices::openUrl(url)) {
        QMessageBox::warning(this, "Ошибка", "Не удалось открыть файл");
    }
}
