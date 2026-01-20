// AuthController.h
#pragma once
#include <drogon/HttpSimpleController.h>

using namespace drogon;

class AuthController : public drogon::HttpSimpleController<AuthController> {
public:
    AuthController();
    
    virtual void asyncHandleHttpRequest(
        const HttpRequestPtr& req,
        std::function<void(const HttpResponsePtr&)>&& callback) override;
    
    PATH_LIST_BEGIN
    PATH_ADD("/api/register", drogon::Post);
    PATH_ADD("/api/login", drogon::Post);
    PATH_ADD("/api/logout", drogon::Post);
    PATH_ADD("/api/me", drogon::Get);
    PATH_ADD("/api/dashboard", drogon::Get);
    PATH_LIST_END
};