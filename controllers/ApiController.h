#pragma once
#include <drogon/HttpController.h>

using namespace drogon;

class ApiController : public HttpController<ApiController> {
public:
    METHOD_LIST_BEGIN
        ADD_METHOD_TO(ApiController::getInfo, "/api/v1/info", Get);
        ADD_METHOD_TO(ApiController::createUser, "/api/v1/users", Post);
    METHOD_LIST_END
    
    void getInfo(const HttpRequestPtr& req,
                 std::function<void(const HttpResponsePtr&)>&& callback);
    
    void createUser(const HttpRequestPtr& req,
                    std::function<void(const HttpResponsePtr&)>&& callback);
};