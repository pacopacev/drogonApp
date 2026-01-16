#pragma once
#include <drogon/drogon.h>
#include <string>

class User {
public:
    User() = default;

    User(int id, const std::string& username, const std::string& email)
        : id(id), username(username), email(email) {}

    int getId() const { return id; }
    std::string getUsername() const { return username; }
    std::string getEmail() const { return email; }

    static User fromJson(const Json::Value& json);
    Json::Value toJson() const;

private:
    int id = 0;
    std::string username;
    std::string email;
};
