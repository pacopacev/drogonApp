// AuthFilter.h
#pragma once
#include <drogon/HttpFilter.h>
#include <json/json.h>

using namespace drogon;

class AuthFilter : public HttpFilter<AuthFilter> {
public:
    virtual void doFilter(const HttpRequestPtr& req,
                         FilterCallback&& fcb,
                         FilterChainCallback&& fccb) override;
};