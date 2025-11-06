#ifndef CONFIG_PARSER_H
#define CONFIG_PARSER_H

#include <string>
#include <map>

// Simple .env file parser for configuration
class ConfigParser {
private:
    std::map<std::string, std::string> config_map;

    static std::string trim(const std::string& str);

public:
    ConfigParser();

    // Load configuration from file
    bool LoadFromFile(const std::string& filename);

    // Get string value with optional default
    std::string GetString(const std::string& key, const std::string& default_value = "") const;

    // Get integer value with optional default
    int GetInt(const std::string& key, int default_value = 0) const;

    // Get boolean value with optional default
    bool GetBool(const std::string& key, bool default_value = false) const;

    // Check if key exists
    bool HasKey(const std::string& key) const;
};

#endif // CONFIG_PARSER_H
