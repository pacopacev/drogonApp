// AuthController.h
#pragma once
#include <drogon/HttpSimpleController.h>

class AuthController : public drogon::HttpSimpleController<AuthController> {
public:
    void asyncHandleHttpRequest(
        const drogon::HttpRequestPtr& req,
        std::function<void(const drogon::HttpResponsePtr&)>&& callback) override;
    
    PATH_LIST_BEGIN
    PATH_ADD("/api/register", drogon::Post);
    PATH_ADD("/api/login", drogon::Post);
    PATH_ADD("/api/logout", drogon::Post);
    PATH_ADD("/api/me", drogon::Get);
    PATH_LIST_END
};