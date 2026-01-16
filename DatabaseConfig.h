// DatabaseConfig.h
#pragma once
#include <drogon/drogon.h>
#include <string>
#include <map>
#include <memory>
#include <vector>

class DatabaseConfig {
public:
    // Singleton instance
    static DatabaseConfig& getInstance();
    
    // Initialize database connection from config.json
    bool initialize();
    
    // Initialize from specific config file path
    bool initialize(const std::string& configPath);
    
    // Get database client
    std::shared_ptr<drogon::orm::DbClient> getClient();
    
    // Get database client by name
    std::shared_ptr<drogon::orm::DbClient> getClient(const std::string& name);
    
    // Check if initialized
    bool isInitialized() const { return _initialized; }
    
    // Get config file path
    std::string getConfigPath() const { return _configPath; }
    
private:
    DatabaseConfig() = default;
    DatabaseConfig(const DatabaseConfig&) = delete;
    DatabaseConfig& operator=(const DatabaseConfig&) = delete;
    
    // Find config.json in various locations
    std::string findConfigFile(const std::string& filename = "config.json");
    
    // Load configuration from JSON file
    bool loadConfigFile(const std::string& path);
    
    // Parse database configuration from JSON
    bool parseDatabaseConfig(const Json::Value& dbConfig);
    
    // Create database client from configuration
    bool createDatabaseClient(const std::string& name, const Json::Value& config);
    
    std::string _configPath;
    std::map<std::string, std::shared_ptr<drogon::orm::DbClient>> _dbClients;
    std::shared_ptr<drogon::orm::DbClient> _defaultClient;
    bool _initialized = false;
};