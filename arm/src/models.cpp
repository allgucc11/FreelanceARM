#include "models.h"

QJsonObject UserAccount::toJson() const {
    QJsonObject o;
    o["login"] = login;
    o["displayName"] = displayName;
    o["passwordHash"] = passwordHash;
    o["photoPath"] = photoPath;
    o["purchases"] = purchases;
    o["deleted"] = deleted;
    return o;
}

UserAccount UserAccount::fromJson(const QJsonObject &obj) {
    UserAccount u;
    u.login = obj["login"].toString();
    u.displayName = obj["displayName"].toString();
    // Роль аккаунта намеренно не читается из базы. Один аккаунт может входить и как покупатель, и как фрилансер.
    // Старое поле role из прошлых версий игнорируется.
    u.role = QString();
    u.passwordHash = obj["passwordHash"].toString();
    u.photoPath = obj["photoPath"].toString();
    u.purchases = obj["purchases"].toInt();
    u.deleted = obj["deleted"].toBool(false);
    return u;
}

QJsonObject FreelancerProfile::toJson() const {
    QJsonObject o;
    o["login"] = login;
    o["name"] = name;
    o["skills"] = skills;
    o["rate"] = rate;
    o["rating"] = rating;
    o["workTime"] = workTime;
    o["description"] = description;
    o["photoPath"] = photoPath;
    o["completedOrders"] = completedOrders;
    return o;
}

FreelancerProfile FreelancerProfile::fromJson(const QJsonObject &obj) {
    FreelancerProfile p;
    p.login = obj["login"].toString();
    p.name = obj["name"].toString();
    p.skills = obj["skills"].toString();
    p.rate = obj["rate"].toDouble();
    p.rating = obj["rating"].toDouble();
    p.workTime = obj["workTime"].toString();
    p.description = obj["description"].toString();
    p.photoPath = obj["photoPath"].toString();
    p.completedOrders = obj["completedOrders"].toInt();
    return p;
}

QJsonObject TaskInfo::toJson() const {
    QJsonObject o;
    o["buyerLogin"] = buyerLogin;
    o["title"] = title;
    o["skill"] = skill;
    o["budget"] = budget;
    o["timeRange"] = timeRange;
    return o;
}

TaskInfo TaskInfo::fromJson(const QJsonObject &obj) {
    TaskInfo t;
    t.buyerLogin = obj["buyerLogin"].toString();
    t.title = obj["title"].toString();
    t.skill = obj["skill"].toString();
    t.budget = obj["budget"].toDouble();
    t.timeRange = obj["timeRange"].toString();
    return t;
}

QJsonObject OrderInfo::toJson() const {
    QJsonObject o;
    o["orderId"] = orderId;
    o["buyerLogin"] = buyerLogin;
    o["freelancerLogin"] = freelancerLogin;
    o["taskTitle"] = taskTitle;
    o["status"] = status;
    o["rating"] = rating;
    return o;
}

OrderInfo OrderInfo::fromJson(const QJsonObject &obj) {
    OrderInfo o;
    o.orderId = obj["orderId"].toString();
    o.buyerLogin = obj["buyerLogin"].toString();
    o.freelancerLogin = obj["freelancerLogin"].toString();
    o.taskTitle = obj["taskTitle"].toString();
    o.status = obj["status"].toString();
    o.rating = obj["rating"].toInt();
    return o;
}

QJsonObject ChatMessage::toJson() const {
    QJsonObject o;
    o["orderId"] = orderId;
    o["buyerLogin"] = buyerLogin;
    o["freelancerLogin"] = freelancerLogin;
    o["senderLogin"] = senderLogin;
    o["senderName"] = senderName;
    o["text"] = text;
    o["time"] = time;
    o["attachmentName"] = attachmentName;
    o["attachmentPath"] = attachmentPath;
    return o;
}

ChatMessage ChatMessage::fromJson(const QJsonObject &obj) {
    ChatMessage m;
    m.orderId = obj["orderId"].toString();
    m.buyerLogin = obj["buyerLogin"].toString();
    m.freelancerLogin = obj["freelancerLogin"].toString();
    m.senderLogin = obj["senderLogin"].toString();
    m.senderName = obj["senderName"].toString();
    m.text = obj["text"].toString();
    m.time = obj["time"].toString();
    m.attachmentName = obj["attachmentName"].toString();
    m.attachmentPath = obj["attachmentPath"].toString();
    return m;
}
