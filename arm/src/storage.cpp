#include "storage.h"
#include <QStandardPaths>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonArray>
#include <QCryptographicHash>
#include <QSet>
#include <QDateTime>
#include <QtGlobal>
#include <QFileInfo>

QString Storage::dataDir() {
    QString base = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    if (base.isEmpty()) {
        base = QDir::homePath() + "/FreelanceExchangeData";
    }
    QDir dir(base);
    if (!dir.exists()) dir.mkpath(".");
    return dir.absolutePath();
}

bool Storage::ensureDataDir() {
    QDir dir(dataDir());
    return dir.exists() || dir.mkpath(".");
}

QString Storage::copyAttachmentToStorage(const QString &sourcePath) {
    if (sourcePath.trimmed().isEmpty()) return QString();
    QFileInfo info(sourcePath);
    if (!info.exists() || !info.isFile()) return QString();
    ensureDataDir();
    QDir dir(dataDir());
    if (!dir.exists("attachments")) dir.mkpath("attachments");
    QString safeName = info.fileName();
    safeName.replace('/', '_');
    safeName.replace('\\', '_');
    QString target = dir.absoluteFilePath("attachments/" + QString::number(QDateTime::currentMSecsSinceEpoch()) + "_" + safeName);
    if (QFile::copy(sourcePath, target)) return target;
    return QString();
}

QString Storage::hashPassword(const QString &password) {
    QByteArray bytes = QCryptographicHash::hash(password.toUtf8(), QCryptographicHash::Sha256).toHex();
    return QString::fromLatin1(bytes);
}

bool Storage::readArray(const QString &fileName, QJsonArray *array) {
    ensureDataDir();
    QFile file(dataDir() + "/" + fileName);
    if (!file.exists()) { *array = QJsonArray(); return true; }
    if (!file.open(QIODevice::ReadOnly)) return false;
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    file.close();
    if (!doc.isArray()) { *array = QJsonArray(); return true; }
    *array = doc.array();
    return true;
}

bool Storage::writeArray(const QString &fileName, const QJsonArray &array) {
    ensureDataDir();
    QFile file(dataDir() + "/" + fileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) return false;
    file.write(QJsonDocument(array).toJson(QJsonDocument::Indented));
    file.close();
    return true;
}

QVector<UserAccount> Storage::loadUsers() {
    QJsonArray a; readArray("users.json", &a);
    QVector<UserAccount> result;
    for (int i=0;i<a.size();++i) {
        UserAccount u = UserAccount::fromJson(a[i].toObject());
        if (!u.login.trimmed().isEmpty()) result.append(u);
    }
    return result;
}

bool Storage::saveUsers(const QVector<UserAccount> &users) {
    QJsonArray a;
    for (int i=0;i<users.size();++i) a.append(users[i].toJson());
    return writeArray("users.json", a);
}

bool Storage::loginExists(const QString &login) {
    QVector<UserAccount> users = loadUsers();
    for (int i=0;i<users.size();++i) {
        if (users[i].login.compare(login, Qt::CaseInsensitive)==0) return true;
    }
    return false;
}

bool Storage::addUser(const UserAccount &user, QString *errorText) {
    if (user.login.trimmed().isEmpty() || user.displayName.trimmed().isEmpty()) {
        if (errorText) *errorText = "Заполните имя пользователя и логин";
        return false;
    }
    if (loginExists(user.login)) {
        if (errorText) *errorText = "Такой логин уже существует";
        return false;
    }
    QVector<UserAccount> users = loadUsers();
    users.append(user);
    return saveUsers(users);
}

bool Storage::authenticate(const QString &login, const QString &password, UserAccount *outUser, QString *errorText) {
    // ВАЖНО: авторизация проверяет только логин и пароль.
    // Роль аккаунта в базе не используется и не проверяется.
    // Выбранная роль применяется только в AuthDialog как временная роль текущего входа.
    QVector<UserAccount> users = loadUsers();
    QString normalizedLogin = login.trimmed();
    QString hash = hashPassword(password);
    for (int i=0;i<users.size();++i) {
        if (users[i].login.compare(normalizedLogin, Qt::CaseInsensitive) == 0) {
            if (users[i].deleted) {
                if (errorText) *errorText = "Профиль удален";
                return false;
            }
            if (users[i].passwordHash == hash) {
                if (outUser) {
                    *outUser = users[i];
                    outUser->role = QString();
                }
                return true;
            }
            if (errorText) *errorText = "Неверный пароль";
            return false;
        }
    }
    if (errorText) *errorText = "Аккаунт не найден";
    return false;
}

QString Storage::displayNameForLogin(const QString &login) {
    QVector<UserAccount> users = loadUsers();
    for (int i=0;i<users.size();++i) {
        if (users[i].login.compare(login, Qt::CaseInsensitive) == 0) {
            if (users[i].deleted) return "Профиль удален";
            return users[i].displayName;
        }
    }
    return "Профиль удален";
}

QString Storage::photoForLogin(const QString &login) {
    QVector<UserAccount> users = loadUsers();
    for (int i=0;i<users.size();++i) {
        if (users[i].login.compare(login, Qt::CaseInsensitive) == 0) {
            if (users[i].deleted) return QString();
            return users[i].photoPath;
        }
    }
    return QString();
}

bool Storage::updateUserPhoto(const QString &login, const QString &photoPath) {
    QVector<UserAccount> users = loadUsers();
    for (int i=0;i<users.size();++i) {
        if (users[i].login.compare(login, Qt::CaseInsensitive) == 0) {
            users[i].photoPath = photoPath;
            return saveUsers(users);
        }
    }
    return false;
}


bool Storage::isUserDeleted(const QString &login) {
    QVector<UserAccount> users = loadUsers();
    for (int i=0;i<users.size();++i) {
        if (users[i].login.compare(login, Qt::CaseInsensitive) == 0) return users[i].deleted;
    }
    return true;
}

bool Storage::deleteUserAccount(const QString &login, const QString &password, QString *errorText) {
    QVector<UserAccount> users = loadUsers();
    QString hash = hashPassword(password);
    bool found = false;
    for (int i=0;i<users.size();++i) {
        if (users[i].login.compare(login, Qt::CaseInsensitive) == 0) {
            found = true;
            if (users[i].passwordHash != hash) {
                if (errorText) *errorText = "Неверный пароль";
                return false;
            }
            users[i].deleted = true;
            users[i].displayName = "Профиль удален";
            users[i].photoPath.clear();
            break;
        }
    }
    if (!found) {
        if (errorText) *errorText = "Аккаунт не найден";
        return false;
    }
    if (!saveUsers(users)) return false;
    deleteFreelancerProfile(login);
    return true;
}


QVector<FreelancerProfile> Storage::loadFreelancers() {
    QJsonArray a; readArray("freelancers.json", &a);
    QVector<FreelancerProfile> result;
    for (int i=0;i<a.size();++i) {
        FreelancerProfile fp = FreelancerProfile::fromJson(a[i].toObject());
        if (!fp.login.trimmed().isEmpty() && !isUserDeleted(fp.login)) result.append(fp);
    }
    return result;
}

bool Storage::saveFreelancers(const QVector<FreelancerProfile> &items) {
    QJsonArray a;
    for (int i=0;i<items.size();++i) a.append(items[i].toJson());
    return writeArray("freelancers.json", a);
}

bool Storage::saveFreelancerProfile(const FreelancerProfile &profile) {
    QVector<FreelancerProfile> items = loadFreelancers();
    bool found = false;
    for (int i=0;i<items.size();++i) {
        if (items[i].login == profile.login) { items[i] = profile; found = true; break; }
    }
    if (!found) items.append(profile);
    return saveFreelancers(items);
}

bool Storage::getFreelancerProfile(const QString &login, FreelancerProfile *profile) {
    QVector<FreelancerProfile> items = loadFreelancers();
    for (int i=0;i<items.size();++i) {
        if (items[i].login == login) { if (profile) *profile = items[i]; return true; }
    }
    return false;
}


bool Storage::deleteFreelancerProfile(const QString &login) {
    QJsonArray a;
    readArray("freelancers.json", &a);
    QJsonArray out;
    for (int i=0;i<a.size();++i) {
        QJsonObject obj = a[i].toObject();
        if (obj["login"].toString() != login) out.append(obj);
    }
    return writeArray("freelancers.json", out);
}

QVector<TaskInfo> Storage::loadTasks() {
    QJsonArray a; readArray("tasks.json", &a);
    QVector<TaskInfo> result;
    for (int i=0;i<a.size();++i) result.append(TaskInfo::fromJson(a[i].toObject()));
    return result;
}

bool Storage::saveTasks(const QVector<TaskInfo> &items) {
    QJsonArray a;
    for (int i=0;i<items.size();++i) a.append(items[i].toJson());
    return writeArray("tasks.json", a);
}

bool Storage::addTask(const TaskInfo &task) {
    QVector<TaskInfo> items = loadTasks();
    items.append(task);
    return saveTasks(items);
}


QVector<OrderInfo> Storage::loadOrders() {
    QJsonArray a; readArray("orders.json", &a);
    QVector<OrderInfo> result;
    for (int i=0;i<a.size();++i) result.append(OrderInfo::fromJson(a[i].toObject()));
    return result;
}

bool Storage::saveOrders(const QVector<OrderInfo> &items) {
    QJsonArray a;
    for (int i=0;i<items.size();++i) a.append(items[i].toJson());
    return writeArray("orders.json", a);
}

bool Storage::startOrder(const QString &buyerLogin, const QString &freelancerLogin, const QString &taskTitle) {
    QVector<OrderInfo> items = loadOrders();
    for (int i=0;i<items.size();++i) {
        if (items[i].buyerLogin == buyerLogin && items[i].freelancerLogin == freelancerLogin && items[i].status == "in_progress") return true;
    }
    OrderInfo o;
    o.orderId = buyerLogin + "_" + freelancerLogin + "_" + QString::number(QDateTime::currentMSecsSinceEpoch());
    o.buyerLogin = buyerLogin;
    o.freelancerLogin = freelancerLogin;
    o.taskTitle = taskTitle;
    o.status = "in_progress";
    o.rating = 0;
    items.append(o);
    if (!saveOrders(items)) return false;
    addSystemMessage(o.orderId, buyerLogin, freelancerLogin, "Заказ открыт");
    return true;
}

bool Storage::completeOrder(const QString &buyerLogin, const QString &freelancerLogin, int rating) {
    QVector<OrderInfo> items = loadOrders();
    bool found = false;
    QString completedOrderId;
    for (int i=items.size()-1;i>=0;--i) {
        if (items[i].buyerLogin == buyerLogin && items[i].freelancerLogin == freelancerLogin && items[i].status == "in_progress") {
            if (items[i].orderId.isEmpty()) items[i].orderId = items[i].buyerLogin + "_" + items[i].freelancerLogin + "_legacy";
            completedOrderId = items[i].orderId;
            items[i].status = "completed";
            items[i].rating = qMax(1, qMin(5, rating));
            found = true;
            break;
        }
    }
    if (!found) return false;
    bool ok = saveOrders(items);
    if (!ok) return false;
    addSystemMessage(completedOrderId, buyerLogin, freelancerLogin, "Заказ закрыт");
    updateFreelancerRating(freelancerLogin);

    QVector<UserAccount> users = loadUsers();
    for (int i=0;i<users.size();++i) {
        if (users[i].login == buyerLogin) {
            users[i].purchases = users[i].purchases + 1;
            break;
        }
    }
    saveUsers(users);
    return true;
}

bool Storage::hasStartedOrder(const QString &buyerLogin, const QString &freelancerLogin) {
    return hasActiveOrder(buyerLogin, freelancerLogin);
}

bool Storage::hasActiveOrder(const QString &buyerLogin, const QString &freelancerLogin) {
    QVector<OrderInfo> items = loadOrders();
    for (int i=0;i<items.size();++i) {
        if (items[i].buyerLogin == buyerLogin && items[i].freelancerLogin == freelancerLogin && items[i].status == "in_progress") return true;
    }
    return false;
}

QString Storage::activeOrderId(const QString &buyerLogin, const QString &freelancerLogin) {
    QVector<OrderInfo> items = loadOrders();
    for (int i=items.size()-1;i>=0;--i) {
        if (items[i].buyerLogin == buyerLogin && items[i].freelancerLogin == freelancerLogin && items[i].status == "in_progress") {
            if (!items[i].orderId.isEmpty()) return items[i].orderId;
            return items[i].buyerLogin + "_" + items[i].freelancerLogin + "_legacy";
        }
    }
    return QString();
}

bool Storage::hasCompletedRatedOrder(const QString &buyerLogin, const QString &freelancerLogin) {
    QVector<OrderInfo> items = loadOrders();
    for (int i=0;i<items.size();++i) {
        if (items[i].buyerLogin == buyerLogin && items[i].freelancerLogin == freelancerLogin && items[i].status == "completed" && items[i].rating > 0) return true;
    }
    return false;
}

void Storage::updateFreelancerRating(const QString &freelancerLogin) {
    QVector<OrderInfo> orders = loadOrders();
    int count = 0;
    int sum = 0;
    for (int i=0;i<orders.size();++i) {
        if (orders[i].freelancerLogin == freelancerLogin && orders[i].status == "completed" && orders[i].rating > 0) {
            sum += orders[i].rating;
            count++;
        }
    }
    QVector<FreelancerProfile> profiles = loadFreelancers();
    for (int i=0;i<profiles.size();++i) {
        if (profiles[i].login == freelancerLogin) {
            profiles[i].rating = count > 0 ? double(sum) / double(count) : 0.0;
            profiles[i].completedOrders = count;
            saveFreelancers(profiles);
            return;
        }
    }
}

QVector<ChatMessage> Storage::loadMessages() {
    QJsonArray a; readArray("messages.json", &a);
    QVector<ChatMessage> result;
    for (int i=0;i<a.size();++i) result.append(ChatMessage::fromJson(a[i].toObject()));
    return result;
}

bool Storage::saveMessages(const QVector<ChatMessage> &items) {
    QJsonArray a;
    for (int i=0;i<items.size();++i) a.append(items[i].toJson());
    return writeArray("messages.json", a);
}

bool Storage::addMessage(const ChatMessage &message) {
    QVector<ChatMessage> items = loadMessages();
    ChatMessage m = message;
    if (m.orderId.isEmpty()) m.orderId = activeOrderId(m.buyerLogin, m.freelancerLogin);
    if (m.orderId.isEmpty()) m.orderId = latestOrderId(m.buyerLogin, m.freelancerLogin);
    items.append(m);
    return saveMessages(items);
}

bool Storage::addSystemMessage(const QString &orderId, const QString &buyerLogin, const QString &freelancerLogin, const QString &text) {
    if (orderId.trimmed().isEmpty()) return false;
    ChatMessage msg;
    msg.orderId = orderId;
    msg.buyerLogin = buyerLogin;
    msg.freelancerLogin = freelancerLogin;
    msg.senderLogin = "__system__";
    msg.senderName = "Система";
    msg.text = text;
    msg.time = QDateTime::currentDateTime().toString("dd.MM.yyyy HH:mm");
    return addMessage(msg);
}

QString Storage::latestOrderId(const QString &buyerLogin, const QString &freelancerLogin) {
    QVector<OrderInfo> items = loadOrders();
    for (int i=items.size()-1;i>=0;--i) {
        if (items[i].buyerLogin == buyerLogin && items[i].freelancerLogin == freelancerLogin) {
            if (!items[i].orderId.isEmpty()) return items[i].orderId;
            return items[i].buyerLogin + "_" + items[i].freelancerLogin + "_legacy";
        }
    }
    return QString();
}

QVector<ChatMessage> Storage::messagesForOrder(const QString &orderId) {
    QVector<ChatMessage> all = loadMessages();
    QVector<ChatMessage> result;
    if (orderId.trimmed().isEmpty()) return result;
    for (int i=0;i<all.size();++i) {
        if (all[i].orderId == orderId) result.append(all[i]);
    }
    return result;
}

QVector<ChatMessage> Storage::messagesForPair(const QString &buyerLogin, const QString &freelancerLogin) {
    // История чата между покупателем и фрилансером должна сохраняться полностью.
    // Поэтому возвращаем сообщения по всем заказам этой пары, а не только
    // по последнему активному заказу. Системные сообщения "Заказ открыт"
    // и "Заказ закрыт" остаются в общей истории и видны обеим сторонам.
    QVector<ChatMessage> all = loadMessages();
    QVector<ChatMessage> result;
    for (int i=0;i<all.size();++i) {
        if (all[i].buyerLogin == buyerLogin && all[i].freelancerLogin == freelancerLogin) {
            result.append(all[i]);
        }
    }
    return result;
}

QStringList Storage::buyersForFreelancer(const QString &freelancerLogin) {
    QStringList result;
    QSet<QString> seen;
    QVector<OrderInfo> orders = loadOrders();
    for (int i=0;i<orders.size();++i) {
        if (orders[i].freelancerLogin == freelancerLogin && !isUserDeleted(orders[i].buyerLogin) && !seen.contains(orders[i].buyerLogin)) {
            seen.insert(orders[i].buyerLogin);
            result << orders[i].buyerLogin;
        }
    }
    QVector<ChatMessage> all = loadMessages();
    for (int i=0;i<all.size();++i) {
        if (all[i].freelancerLogin == freelancerLogin && !isUserDeleted(all[i].buyerLogin) && !seen.contains(all[i].buyerLogin)) {
            seen.insert(all[i].buyerLogin);
            result << all[i].buyerLogin;
        }
    }
    return result;
}
