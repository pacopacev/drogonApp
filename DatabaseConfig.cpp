// DatabaseConfig.cpp
#include "DatabaseConfig.h"
#include <fstream>
#include <filesystem>
#include <sstream>
#include <iostream>

DatabaseConfig& DatabaseConfig::getInstance() {
    static DatabaseConfig instance;
    return instance;
}

std::string DatabaseConfig::findConfigFile(const std::string& filename) {
    // Try multiple possible locations
    std::vector<std::string> possiblePaths = {
        filename,                           // Current directory
        "../" + filename,                   // Parent directory
        "../../" + filename,                // Grandparent directory
        "H:/drogonApp/" + filename,         // Absolute project path
        std::filesystem::current_path().parent_path().string() + "/" + filename
    };
    
    for (const auto& tryPath : possiblePaths) {
        if (std::filesystem::exists(tryPath)) {
            std::cout << "Found config file at: " << tryPath << std::endl;
            return tryPath;
        }
    }
    
    std::cerr << "Could not find " << filename << ". Tried:" << std::endl;
    for (const auto& tryPath : possiblePaths) {
        std::cerr << "  - " << tryPath << std::endl;
    }
    
    return "";
}

bool DatabaseConfig::loadConfigFile(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        std::cerr << "Cannot open config file: " << path << std::endl;
        return false;
    }
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    file.close();
    
    std::cout << "Loading configuration from: " << path << std::endl;
    
    try {
        Json::Value config;
        Json::CharReaderBuilder readerBuilder;
        std::string errors;
        
        std::istringstream jsonStream(buffer.str());
        if (!Json::parseFromStream(readerBuilder, jsonStream, &config, &errors)) {
            std::cerr << "Failed to parse JSON: " << errors << std::endl;
            return false;
        }
        
        // Look for database configuration in different possible keys
        std::vector<std::string> possibleKeys = {"dbs", "db_clients", "databases"};
        bool foundDbConfig = false;
        
        for (const auto& key : possibleKeys) {
            if (config.isMember(key) && config[key].isArray()) {
                std::cout << "Found database configuration under key: " << key << std::endl;
                if (parseDatabaseConfig(config[key])) {
                    foundDbConfig = true;
                    break; // Stop after first successful parse
                }
            }
        }
        
        if (!foundDbConfig) {
            std::cout << "No database configuration found in config.json" << std::endl;
            std::cout << "Tried keys: dbs, db_clients, databases" << std::endl;
            // Don't return false here - server can run without DB
        }
        
        _configPath = path;
        return true;
        
    } catch (const std::exception& e) {
        std::cerr << "Error loading config file: " << e.what() << std::endl;
        return false;
    }
}

bool DatabaseConfig::parseDatabaseConfig(const Json::Value& dbConfig) {
    if (!dbConfig.isArray()) {
        std::cerr << "Database config is not an array" << std::endl;
        return false;
    }
    
    bool success = false;
    
    for (const auto& db : dbConfig) {
        if (!db.isObject()) continue;
        
        // Get database name (default to "default" if not specified)
        std::string name = "default";
        if (db.isMember("name") && db["name"].isString()) {
            name = db["name"].asString();
        }
        
        std::cout << "Parsing database config for: " << name << std::endl;
        
        if (createDatabaseClient(name, db)) {
            success = true;
            
            // Set as default if it's the first one or explicitly named "default"
            if (name == "default" || !_defaultClient) {
                _defaultClient = _dbClients[name];
            }
        }
    }
    
    return success;
}

bool DatabaseConfig::createDatabaseClient(const std::string& name, const Json::Value& config) {
    try {
        // Check for required fields
        if (!config.isMember("rdbms") || !config["rdbms"].isString()) {
            std::cerr << "Missing or invalid 'rdbms' field for database: " << name << std::endl;
            return false;
        }
        
        std::string rdbms = config["rdbms"].asString();
        if (rdbms != "postgresql") {
            std::cerr << "Unsupported database type: " << rdbms << " for: " << name << std::endl;
            return false;
        }
        
        // Build connection string
        std::string connString;
        
        // Check if connection_info is provided
        if (config.isMember("connection_info") && config["connection_info"].isString()) {
            connString = config["connection_info"].asString();
        } else {
            // Build from individual parameters
            if (!config.isMember("host") || !config.isMember("port") || 
                !config.isMember("dbname") || !config.isMember("user") || 
                !config.isMember("passwd")) {
                std::cerr << "Missing required database parameters for: " << name << std::endl;
                return false;
            }
            
            connString = 
                "host=" + config["host"].asString() + " " +
                "port=" + config["port"].asString() + " " +
                "dbname=" + config["dbname"].asString() + " " +
                "user=" + config["user"].asString() + " " +
                "password=" + config["passwd"].asString();
            
            // Add optional parameters
            if (config.isMember("sslmode") && config["sslmode"].isString()) {
                connString += " sslmode=" + config["sslmode"].asString();
            } else {
                connString += " sslmode=require"; // Default for Aiven
            }
        }
        
        // Get connection number (default to 1)
        size_t connectionNum = 1;
        if (config.isMember("connection_number") && config["connection_number"].isUInt()) {
            connectionNum = config["connection_number"].asUInt();
        } else if (config.isMember("number_of_connections") && config["number_of_connections"].isUInt()) {
            connectionNum = config["number_of_connections"].asUInt();
        }
        
        std::cout << "Creating PostgreSQL client for: " << name << std::endl;
        std::cout << "Connection string (password hidden): " 
                  << connString.substr(0, connString.find("password=") + 9) << "*******" << std::endl;
        
        #ifdef USE_POSTGRESQL
            auto client = drogon::orm::DbClient::newPgClient(connString, connectionNum);
            
            // Test connection
            auto result = client->execSqlSync("SELECT version()");
            std::cout << "✓ Database connected: " << name << " - PostgreSQL " 
                      << result[0]["version"].as<std::string>().substr(0, 50) << std::endl;
            
            _dbClients[name] = client;
            return true;
            
        #else
            std::cerr << "PostgreSQL support not compiled in!" << std::endl;
            return false;
        #endif
        
    } catch (const std::exception& e) {
        std::cerr << "Failed to create database client '" << name << "': " << e.what() << std::endl;
        return false;
    }
}

bool DatabaseConfig::initialize() {
    return initialize("config.json");
}

bool DatabaseConfig::initialize(const std::string& configPath) {
    if (_initialized) {
        std::cout << "Database already initialized" << std::endl;
        return true;
    }
    
    std::cout << "Initializing database configuration..." << std::endl;
    
    std::string foundPath = configPath;
    if (configPath == "config.json") {
        foundPath = findConfigFile();
        if (foundPath.empty()) {
            std::cerr << "Cannot find config.json" << std::endl;
            // Still mark as initialized to avoid repeated errors
            _initialized = true;
            return false;
        }
    }
    
    if (!loadConfigFile(foundPath)) {
        std::cerr << "Failed to load configuration from: " << foundPath << std::endl;
        // Still mark as initialized to avoid repeated errors
        _initialized = true;
        return false;
    }
    
    if (_dbClients.empty()) {
        std::cout << "No database clients created (server will run without DB)" << std::endl;
    } else {
        std::cout << "Database configuration initialized successfully with " 
                  << _dbClients.size() << " client(s)" << std::endl;
    }
    
    _initialized = true;
    return true;
}

std::shared_ptr<drogon::orm::DbClient> DatabaseConfig::getClient() {
    if (!_initialized) {
        // Auto-initialize if not already initialized
        std::cout << "Auto-initializing database configuration..." << std::endl;
        if (!initialize()) {
            std::cerr << "Auto-initialization failed" << std::endl;
            return nullptr;
        }
    }
    
    if (!_defaultClient && !_dbClients.empty()) {
        // Use the first client if no default is set
        _defaultClient = _dbClients.begin()->second;
    }
    
    return _defaultClient;
}

std::shared_ptr<drogon::orm::DbClient> DatabaseConfig::getClient(const std::string& name) {
    if (!_initialized) {
        // Auto-initialize if not already initialized
        std::cout << "Auto-initializing database configuration..." << std::endl;
        if (!initialize()) {
            std::cerr << "Auto-initialization failed" << std::endl;
            return nullptr;
        }
    }
    
    auto it = _dbClients.find(name);
    if (it != _dbClients.end()) {
        return it->second;
    }
    
    std::cout << "Database client not found: " << name << std::endl;
    return nullptr;
}