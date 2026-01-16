// DatabaseConfig.h
#pragma once
#include <drogon/drogon.h>
#include <string>
#include <map>
#include <memory>

class DatabaseConfig {
public:
    // Singleton instance
    static DatabaseConfig& getInstance();
    
    // Initialize database connection
    bool initialize();
    
    // Get database client
    std::shared_ptr<drogon::orm::DbClient> getClient();
    
    // Check if initialized
    bool isInitialized() const { return _initialized; }
    
private:
    DatabaseConfig() = default;  // Private constructor
    DatabaseConfig(const DatabaseConfig&) = delete;
    DatabaseConfig& operator=(const DatabaseConfig&) = delete;
    
    // Load environment variables from .env file
    bool loadEnvFile(const std::string& path = ".env");
    
    // Create database client from loaded config
    bool createDatabaseClient();
    
    std::map<std::string, std::string> _envVars;
    std::shared_ptr<drogon::orm::DbClient> _dbClient;
    bool _initialized = false;
};