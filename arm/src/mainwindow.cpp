#include "mainwindow.h"
#include "authdialog.h"
#include "buyerwidget.h"
#include "freelancerwidget.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QApplication>
#include <QInputDialog>
#include <QMessageBox>
#include <QLineEdit>
#include <QDialog>
#include <QFormLayout>
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QGraphicsProxyWidget>
#include <QResizeEvent>
#include <QFrame>
#include <QBrush>
#include <QColor>
#include <QTransform>
#include <QPixmap>
#include "storage.h"

class ScaledWidgetView : public QGraphicsView
{
public:
    explicit ScaledWidgetView(QWidget *content, QWidget *parent = 0)
        : QGraphicsView(parent), m_scene(new QGraphicsScene(this)), m_proxy(0), m_baseSize(1180, 760)
    {
        setFrameShape(QFrame::NoFrame);
        setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        setAlignment(Qt::AlignCenter);
        setBackgroundBrush(QBrush(QColor(243, 246, 251)));
        setScene(m_scene);

        content->setFixedSize(m_baseSize);
        m_proxy = m_scene->addWidget(content);
        m_proxy->setPos(0, 0);
        m_scene->setSceneRect(QRectF(QPointF(0, 0), QSizeF(m_baseSize)));
        updateScale();
    }

protected:
    void resizeEvent(QResizeEvent *event)
    {
        QGraphicsView::resizeEvent(event);
        updateScale();
    }

private:
    void updateScale()
    {
        if (!m_proxy) return;
        const QSize viewportSize = viewport()->size();
        if (viewportSize.width() <= 0 || viewportSize.height() <= 0) return;

        const qreal margin = 24.0;
        const qreal sx = (viewportSize.width() - margin) / qreal(m_baseSize.width());
        const qreal sy = (viewportSize.height() - margin) / qreal(m_baseSize.height());
        const qreal scaleValue = qMax<qreal>(0.55, qMin(sx, sy));

        QTransform transform;
        transform.scale(scaleValue, scaleValue);
        setTransform(transform);
        centerOn(m_baseSize.width() / 2.0, m_baseSize.height() / 2.0);
    }

    QGraphicsScene *m_scene;
    QGraphicsProxyWidget *m_proxy;
    QSize m_baseSize;
};

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), m_central(0), m_stack(0)
{
    setWindowTitle("Биржа фриланса");
    resize(1180, 760);
    setMinimumSize(980, 640);
    applyStyle();
    showAuth();
}

void MainWindow::applyStyle()
{
    qApp->setStyleSheet(
        "QWidget { font-family: Segoe UI, Arial; font-size: 15px; background: #F3F6FB; color: #1F2937; }"
        "QDialog, QMainWindow { background: #F3F6FB; }"
        "#titleLabel { font-size: 28px; font-weight: 700; color: #1E3A8A; margin-bottom: 6px; }"
        "#sectionTitle { font-size: 18px; font-weight: 700; color: #1E3A8A; }"
        "#mutedLabel { color: #64748B; }"
        "QGroupBox { background: white; border: 1px solid #D7DEE9; border-radius: 16px; margin-top: 22px; padding: 16px; padding-top: 18px; font-weight: 700; }"
        "QGroupBox::title { subcontrol-origin: margin; subcontrol-position: top center; top: 2px; padding: 0 8px; }"
        "QLineEdit, QTextEdit, QTextBrowser, QPlainTextEdit, QComboBox, QDoubleSpinBox, QListWidget { background: white; border: 1px solid #CBD5E1; border-radius: 10px; padding: 8px; min-height: 26px; }"
        "QTextEdit, QTextBrowser, QPlainTextEdit { min-height: 80px; }"
        "QPushButton { background: #2563EB; color: white; border: none; border-radius: 12px; padding: 10px 16px; font-weight: 700; }"
        "QPushButton:hover { background: #1D4ED8; }"
        "QPushButton#secondaryButton { background: #E2E8F0; color: #1E293B; }"
        "QPushButton#dangerButton { background: #EF4444; }"
        "QListWidget::item { padding: 8px; border-radius: 8px; }"
        "QListWidget::item:selected { background: #DBEAFE; color: #1E3A8A; }"
        "QWidget#selectedFreelancerCard { background: white; border: none; }"
        "QFrame#freelancerInfoFrame { background: white; border: none; }"
        "QLabel { background: transparent; }"
        "QPushButton:disabled { background: #E2E8F0; color: #64748B; }"
    );
}

void MainWindow::showAuth()
{
    hide();
    QWidget *oldCentral = takeCentralWidget();
    if (oldCentral) oldCentral->deleteLater();
    m_central = 0;
    AuthDialog dlg(0);
    if (dlg.exec() != QDialog::Accepted) {
        qApp->quit();
        return;
    }
    buildForUser(dlg.user());
    show();
}

void MainWindow::buildForUser(const UserAccount &user)
{
    m_user = user;
    if (m_user.role != "buyer" && m_user.role != "freelancer") m_user.role = "buyer";
    QWidget *oldCentral = takeCentralWidget();
    if (oldCentral) oldCentral->deleteLater();
    QWidget *content = new QWidget();
    content->setObjectName("scaledContentRoot");
    QVBoxLayout *root = new QVBoxLayout(content);
    root->setContentsMargins(18, 18, 18, 18);
    root->setSpacing(10);

    QHBoxLayout *top = new QHBoxLayout();
    QLabel *title = new QLabel("Биржа фриланса");
    title->setObjectName("titleLabel");
    QString roleText = m_user.role == "buyer" ? "Покупатель" : "Фрилансер";
    QString statText;
    if (m_user.role == "buyer") statText = " | Покупок: " + QString::number(m_user.purchases);
    else {
        FreelancerProfile fp;
        int done = Storage::getFreelancerProfile(m_user.login, &fp) ? fp.completedOrders : 0;
        statText = " | Выполнено заказов: " + QString::number(done);
    }
    QLabel *info = new QLabel("Вы вошли как: " + m_user.displayName + " | Роль: " + roleText + statText);
    info->setObjectName("mutedLabel");
    QPushButton *profileBtn = new QPushButton("Мой профиль");
    profileBtn->setObjectName("secondaryButton");
    top->addWidget(title);
    top->addStretch();
    top->addWidget(info);
    top->addWidget(profileBtn);
    root->addLayout(top);

    if (m_user.role == "buyer") root->addWidget(new BuyerWidget(m_user));
    else root->addWidget(new FreelancerWidget(m_user));

    m_central = new ScaledWidgetView(content);
    setCentralWidget(m_central);
    connect(profileBtn, SIGNAL(clicked()), this, SLOT(showProfileDialog()));
}


void MainWindow::showProfileDialog()
{
    UserAccount current = m_user;
    QVector<UserAccount> users = Storage::loadUsers();
    for (int i = 0; i < users.size(); ++i) {
        if (users[i].login.compare(m_user.login, Qt::CaseInsensitive) == 0) {
            current = users[i];
            current.role = m_user.role;
            break;
        }
    }

    FreelancerProfile fp;
    const bool hasFreelancerProfile = Storage::getFreelancerProfile(m_user.login, &fp);
    const int completed = hasFreelancerProfile ? fp.completedOrders : 0;
    const QString ratingText = hasFreelancerProfile ? QString::number(fp.rating, 'f', 1) : QString("нет оценок");

    QDialog dlg(this);
    dlg.setWindowTitle("Мой профиль");
    dlg.setModal(true);
    dlg.setMinimumWidth(420);

    QVBoxLayout *root = new QVBoxLayout(&dlg);
    root->setContentsMargins(18, 18, 18, 18);
    root->setSpacing(12);

    QLabel *photo = new QLabel("Нет фото");
    photo->setFixedSize(140, 140);
    photo->setAlignment(Qt::AlignCenter);
    photo->setStyleSheet("background:#E2E8F0;border-radius:14px;color:#64748B;");
    QPixmap pix(current.photoPath);
    if (!current.photoPath.isEmpty() && !pix.isNull()) {
        photo->setText(QString());
        photo->setPixmap(pix.scaled(140, 140, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation));
    }
    root->addWidget(photo, 0, Qt::AlignHCenter);

    QFormLayout *form = new QFormLayout();
    form->addRow("Имя пользователя", new QLabel(current.displayName));
    form->addRow("Логин", new QLabel(current.login));
    form->addRow("Рейтинг", new QLabel(ratingText));
    form->addRow("Выполнено заказов", new QLabel(QString::number(completed)));
    form->addRow("Количество покупок", new QLabel(QString::number(current.purchases)));
    root->addLayout(form);

    QHBoxLayout *buttons = new QHBoxLayout();
    QPushButton *logoutBtn = new QPushButton("Выйти из аккаунта");
    logoutBtn->setObjectName("secondaryButton");
    QPushButton *deleteBtn = new QPushButton("Удалить аккаунт");
    deleteBtn->setObjectName("dangerButton");
    buttons->addWidget(logoutBtn);
    buttons->addWidget(deleteBtn);
    root->addLayout(buttons);

    int action = 0;
    connect(logoutBtn, &QPushButton::clicked, &dlg, [&]() { action = 1; dlg.accept(); });
    connect(deleteBtn, &QPushButton::clicked, &dlg, [&]() { action = 2; dlg.accept(); });

    dlg.exec();
    if (action == 1) {
        logout();
    } else if (action == 2) {
        deleteProfile();
    }
}

void MainWindow::logout()
{
    showAuth();
}


void MainWindow::deleteProfile()
{
    bool ok = false;
    QString password = QInputDialog::getText(this, "Удаление профиля", "Введите пароль для подтверждения удаления профиля", QLineEdit::Password, QString(), &ok);
    if (!ok) return;
    if (password.isEmpty()) {
        QMessageBox::warning(this, "Ошибка", "Введите пароль");
        return;
    }
    if (QMessageBox::question(this, "Подтверждение", "Удалить профиль? Это действие нельзя отменить.") != QMessageBox::Yes) return;
    QString error;
    if (!Storage::deleteUserAccount(m_user.login, password, &error)) {
        QMessageBox::warning(this, "Ошибка", error.isEmpty() ? "Не удалось удалить профиль" : error);
        return;
    }
    QMessageBox::information(this, "Готово", "Профиль удален");
    showAuth();
}
