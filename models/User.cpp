#include "User.h"

Json::Value User::toJson() const {
    Json::Value json;
    json["id"] = id;
    json["username"] = username;
    json["email"] = email;
    return json;
}

User User::fromJson(const Json::Value& json) {
    User user;
    if (!json["id"].isNull())
        user.id = json["id"].asInt();
    if (!json["username"].isNull())
        user.username = json["username"].asString();
    if (!json["email"].isNull())
        user.email = json["email"].asString();
    return user;
}