#include "config_parser.h"
#include <fstream>
#include <sstream>
#include <algorithm>

ConfigParser::ConfigParser() {
}

std::string ConfigParser::trim(const std::string& str) {
    const std::string whitespace = " \t\r\n";
    size_t start = str.find_first_not_of(whitespace);
    if (start == std::string::npos) {
        return "";
    }
    size_t end = str.find_last_not_of(whitespace);
    return str.substr(start, end - start + 1);
}

bool ConfigParser::LoadFromFile(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        return false;
    }

    std::string line;
    while (std::getline(file, line)) {
        // Trim whitespace
        line = trim(line);

        // Skip empty lines and comments
        if (line.empty() || line[0] == '#') {
            continue;
        }

        // Find the '=' separator
        size_t equals_pos = line.find('=');
        if (equals_pos == std::string::npos) {
            continue; // Skip malformed lines
        }

        // Extract key and value
        std::string key = trim(line.substr(0, equals_pos));
        std::string value = trim(line.substr(equals_pos + 1));

        // Remove quotes from value if present
        if (value.length() >= 2 &&
            ((value.front() == '"' && value.back() == '"') ||
             (value.front() == '\'' && value.back() == '\''))) {
            value = value.substr(1, value.length() - 2);
        }

        config_map[key] = value;
    }

    file.close();
    return true;
}

std::string ConfigParser::GetString(const std::string& key, const std::string& default_value) const {
    auto it = config_map.find(key);
    if (it != config_map.end()) {
        return it->second;
    }
    return default_value;
}

int ConfigParser::GetInt(const std::string& key, int default_value) const {
    auto it = config_map.find(key);
    if (it != config_map.end()) {
        try {
            return std::stoi(it->second);
        } catch (...) {
            return default_value;
        }
    }
    return default_value;
}

bool ConfigParser::GetBool(const std::string& key, bool default_value) const {
    auto it = config_map.find(key);
    if (it != config_map.end()) {
        std::string value = it->second;
        // Convert to lowercase for comparison
        std::transform(value.begin(), value.end(), value.begin(), ::tolower);

        if (value == "true" || value == "1" || value == "yes" || value == "on") {
            return true;
        } else if (value == "false" || value == "0" || value == "no" || value == "off") {
            return false;
        }
    }
    return default_value;
}

bool ConfigParser::HasKey(const std::string& key) const {
    return config_map.find(key) != config_map.end();
}
