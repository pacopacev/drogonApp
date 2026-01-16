// DatabaseConfig.cpp
#include "DatabaseConfig.h"
#include <fstream>
#include <filesystem>
#include <iostream>

DatabaseConfig& DatabaseConfig::getInstance() {
    static DatabaseConfig instance;
    return instance;
}

bool DatabaseConfig::loadEnvFile(const std::string& path) {
    // Try multiple possible locations
    std::vector<std::string> possiblePaths = {
        path,                           // Current directory
        "../" + path,                   // Parent directory
        "../../" + path,                // Grandparent directory
        "H:/drogonApp/" + path,         // Absolute project path
        std::filesystem::current_path().string() + "/" + path
    };
    
    std::ifstream envFile;
    std::string foundPath;
    
    for (const auto& tryPath : possiblePaths) {
        envFile.open(tryPath);
        if (envFile.is_open()) {
            foundPath = tryPath;
            LOG_INFO << "Found .env file at: " << foundPath;
            break;
        }
    }
    
    if (!envFile.is_open()) {
        LOG_ERROR << "Could not find .env file. Tried:";
        for (const auto& tryPath : possiblePaths) {
            LOG_ERROR << "  - " << tryPath;
        }
        LOG_ERROR << "Current directory: " << std::filesystem::current_path();
        return false;
    }
    
    // Parse .env file
    std::string line;
    while (std::getline(envFile, line)) {
        // Skip comments and empty lines
        if (line.empty() || line[0] == '#') continue;
        
        // Parse key=value
        size_t equalsPos = line.find('=');
        if (equalsPos != std::string::npos) {
            std::string key = line.substr(0, equalsPos);
            std::string value = line.substr(equalsPos + 1);
            
            // Trim whitespace
            auto trim = [](std::string& str) {
                str.erase(0, str.find_first_not_of(" \t\r\n"));
                str.erase(str.find_last_not_of(" \t\r\n") + 1);
            };
            
            trim(key);
            trim(value);
            
            // Remove quotes if present
            if (!value.empty() && value.front() == '"' && value.back() == '"') {
                value = value.substr(1, value.length() - 2);
            }
            
            _envVars[key] = value;
        }
    }
    
    envFile.close();
    
    // Debug output
    LOG_INFO << "Loaded " << _envVars.size() << " environment variables";
    for (const auto& [key, value] : _envVars) {
        if (key.find("PASS") != std::string::npos || key.find("SECRET") != std::string::npos) {
            LOG_INFO << "  " << key << ": *******";
        } else {
            LOG_INFO << "  " << key << ": " << value;
        }
    }
    
    return true;
}

bool DatabaseConfig::createDatabaseClient() {
    // Check required variables
    std::vector<std::string> required = {"DB_HOST", "DB_PORT", "DB_NAME", "DB_USER", "DB_PASS"};
    
    for (const auto& var : required) {
        if (_envVars.find(var) == _envVars.end()) {
            LOG_ERROR << "Missing required environment variable: " << var;
            return false;
        }
    }
    
    // Build connection string
    std::string connString = 
        "host=" + _envVars["DB_HOST"] + " " +
        "port=" + _envVars["DB_PORT"] + " " +
        "dbname=" + _envVars["DB_NAME"] + " " +
        "user=" + _envVars["DB_USER"] + " " +
        "password=" + _envVars["DB_PASS"] + " " +
        "sslmode=require " +
        "connect_timeout=10";
    
    LOG_DEBUG << "Database connection string (password hidden): " 
              << connString.substr(0, connString.find("password=") + 9) << "*******";
    
    try {
        // Create PostgreSQL client
        _dbClient = drogon::orm::DbClient::newPgClient(connString, 1);
        
        // Test connection
        auto result = _dbClient->execSqlSync("SELECT version()");
        LOG_INFO << "✓ Database connected: PostgreSQL " 
                 << result[0]["version"].as<std::string>().substr(0, 50);
        
        return true;
        
    } catch (const std::exception& e) {
        LOG_ERROR << "Database connection failed: " << e.what();
        return false;
    }
}

bool DatabaseConfig::initialize() {
    if (_initialized) {
        LOG_WARN << "Database already initialized";
        return true;
    }
    
    LOG_INFO << "Initializing database configuration...";
    
    // Try to load from .env file
    if (!loadEnvFile()) {
        LOG_WARN << "Falling back to system environment variables";
        
        auto getEnv = [](const char* name) -> std::string {
            const char* val = std::getenv(name);
            return val ? std::string(val) : "";
        };
        
        _envVars["DB_HOST"] = getEnv("DB_HOST");
        _envVars["DB_PORT"] = getEnv("DB_PORT");
        _envVars["DB_NAME"] = getEnv("DB_NAME");
        _envVars["DB_USER"] = getEnv("DB_USER");
        _envVars["DB_PASS"] = getEnv("DB_PASS");
    }
    
    // Create database client
    if (createDatabaseClient()) {
        _initialized = true;
        LOG_INFO << "Database configuration initialized successfully";
        return true;
    }
    
    LOG_ERROR << "Database configuration failed";
    return false;
}



std::shared_ptr<drogon::orm::DbClient> DatabaseConfig::getClient() {
    if (!_initialized) {
        LOG_ERROR << "Database not initialized. Call initialize() first.";
        return nullptr;
    }
    return _dbClient;
}