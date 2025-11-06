#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "httplib.h"
#include "http_api_server.h"
#include "thread_safe_control.h"
#include "config_parser.h"
#include <sstream>
#include <stdio.h>

// Helper function to parse JSON-like simple format: {"key": value}
static bool ParseJsonInt(const std::string& body, const std::string& key, int& value) {
    // Very simple JSON parser for {"key": value} format
    std::string search_key = "\"" + key + "\"";
    size_t key_pos = body.find(search_key);
    if (key_pos == std::string::npos) {
        return false;
    }

    size_t colon_pos = body.find(":", key_pos);
    if (colon_pos == std::string::npos) {
        return false;
    }

    // Find the value (skip whitespace)
    size_t value_start = colon_pos + 1;
    while (value_start < body.length() && (body[value_start] == ' ' || body[value_start] == '\t')) {
        value_start++;
    }

    if (value_start >= body.length()) {
        return false;
    }

    // Parse the number
    try {
        value = std::stoi(body.substr(value_start));
        return true;
    } catch (...) {
        return false;
    }
}

static bool ParseJsonFloat(const std::string& body, const std::string& key, float& value) {
    int int_value;
    if (ParseJsonInt(body, key, int_value)) {
        value = static_cast<float>(int_value);
        return true;
    }
    return false;
}

ServerConfig ServerConfig::LoadConfig(const std::string& config_path) {
    ServerConfig config;

    ConfigParser parser;
    if (parser.LoadFromFile(config_path)) {
        config.port = parser.GetInt("HTTP_PORT", 45678);
        config.host = parser.GetString("HTTP_HOST", "127.0.0.1");
        config.enabled = parser.GetBool("API_ENABLED", true);
    }
    // If file doesn't exist or fails to load, use defaults

    return config;
}

std::string HttpApiServer::CreateJsonResponse(bool success, const std::string& message,
                                              const std::string& additional_fields) {
    std::ostringstream json;
    json << "{";
    json << "\"success\": " << (success ? "true" : "false");
    if (!message.empty()) {
        json << ", \"message\": \"" << message << "\"";
    }
    if (!additional_fields.empty()) {
        json << ", " << additional_fields;
    }
    json << "}";
    return json.str();
}

HttpApiServer::HttpApiServer(ThreadSafeMonitorControl* control)
    : running(false), should_stop(false), monitor_control(control) {
}

HttpApiServer::~HttpApiServer() {
    Stop();
}

void HttpApiServer::ServerThreadFunc() {
    httplib::Server server;

    // POST /api/brightness - Set brightness (0-100)
    server.Post("/api/brightness", [this](const httplib::Request& req, httplib::Response& res) {
        float brightness;
        if (!ParseJsonFloat(req.body, "value", brightness)) {
            res.status = 400;
            res.set_content(CreateJsonResponse(false, "Invalid request: missing or invalid 'value' field"), "application/json");
            return;
        }

        if (brightness < 0.0f || brightness > 100.0f) {
            res.status = 400;
            res.set_content(CreateJsonResponse(false, "Value must be between 0 and 100"), "application/json");
            return;
        }

        if (!monitor_control->IsInitialized()) {
            res.status = 503;
            res.set_content(CreateJsonResponse(false, "NvAPI not initialized"), "application/json");
            return;
        }

        bool success = monitor_control->SetBrightness(brightness);
        if (success) {
            std::ostringstream fields;
            fields << "\"brightness\": " << static_cast<int>(brightness);
            res.set_content(CreateJsonResponse(true, "Brightness set successfully", fields.str()), "application/json");
        } else {
            res.status = 500;
            res.set_content(CreateJsonResponse(false, "Failed to set brightness"), "application/json");
        }
    });

    // POST /api/contrast - Set contrast (0-100)
    server.Post("/api/contrast", [this](const httplib::Request& req, httplib::Response& res) {
        float contrast;
        if (!ParseJsonFloat(req.body, "value", contrast)) {
            res.status = 400;
            res.set_content(CreateJsonResponse(false, "Invalid request: missing or invalid 'value' field"), "application/json");
            return;
        }

        if (contrast < 0.0f || contrast > 100.0f) {
            res.status = 400;
            res.set_content(CreateJsonResponse(false, "Value must be between 0 and 100"), "application/json");
            return;
        }

        if (!monitor_control->IsInitialized()) {
            res.status = 503;
            res.set_content(CreateJsonResponse(false, "NvAPI not initialized"), "application/json");
            return;
        }

        bool success = monitor_control->SetContrast(contrast);
        if (success) {
            std::ostringstream fields;
            fields << "\"contrast\": " << static_cast<int>(contrast);
            res.set_content(CreateJsonResponse(true, "Contrast set successfully", fields.str()), "application/json");
        } else {
            res.status = 500;
            res.set_content(CreateJsonResponse(false, "Failed to set contrast"), "application/json");
        }
    });

    // POST /api/input - Set input source (1-4)
    server.Post("/api/input", [this](const httplib::Request& req, httplib::Response& res) {
        int source;
        if (!ParseJsonInt(req.body, "source", source)) {
            res.status = 400;
            res.set_content(CreateJsonResponse(false, "Invalid request: missing or invalid 'source' field"), "application/json");
            return;
        }

        if (source < 1 || source > 4) {
            res.status = 400;
            res.set_content(CreateJsonResponse(false, "Source must be between 1 and 4 (1=HDMI 1, 2=HDMI 2, 3=DisplayPort, 4=USB-C)"), "application/json");
            return;
        }

        if (!monitor_control->IsInitialized()) {
            res.status = 503;
            res.set_content(CreateJsonResponse(false, "NvAPI not initialized"), "application/json");
            return;
        }

        const char* input_names[] = {"HDMI 1", "HDMI 2", "DisplayPort", "USB-C"};
        bool success = monitor_control->SetInputSource(source);
        if (success) {
            std::ostringstream fields;
            fields << "\"input\": " << source << ", \"input_name\": \"" << input_names[source - 1] << "\"";
            res.set_content(CreateJsonResponse(true, "Input switched successfully", fields.str()), "application/json");
        } else {
            res.status = 500;
            res.set_content(CreateJsonResponse(false, "Failed to switch input"), "application/json");
        }
    });

    // GET /api/status - Get current status
    server.Get("/api/status", [this](const httplib::Request& req, httplib::Response& res) {
        std::ostringstream fields;
        fields << "\"brightness\": " << static_cast<int>(monitor_control->GetBrightness());
        fields << ", \"contrast\": " << static_cast<int>(monitor_control->GetContrast());
        fields << ", \"display_index\": " << monitor_control->GetSelectedDisplay();
        fields << ", \"nvapi_initialized\": " << (monitor_control->IsInitialized() ? "true" : "false");
        fields << ", \"status_message\": \"" << monitor_control->GetStatusMessage() << "\"";

        res.set_content("{" + fields.str() + "}", "application/json");
    });

    // GET /health - Health check
    server.Get("/health", [](const httplib::Request& req, httplib::Response& res) {
        res.set_content("{\"status\": \"ok\", \"version\": \"1.0.0\"}", "application/json");
    });

    // Start the server
    running = true;
    printf("HTTP API server starting on %s:%d\n", config.host.c_str(), config.port);

    // Listen (blocking call)
    if (!server.listen(config.host.c_str(), config.port)) {
        printf("HTTP API server failed to bind to %s:%d\n", config.host.c_str(), config.port);
        running = false;
        return;
    }

    running = false;
}

bool HttpApiServer::Start(const ServerConfig& cfg) {
    if (running) {
        return false; // Already running
    }

    config = cfg;
    should_stop = false;

    try {
        server_thread = std::make_unique<std::thread>(&HttpApiServer::ServerThreadFunc, this);

        // Give the server a moment to start
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        return running;
    } catch (...) {
        return false;
    }
}

void HttpApiServer::Stop() {
    if (server_thread && server_thread->joinable()) {
        should_stop = true;

        // Note: cpp-httplib server.listen() is blocking and doesn't have a clean shutdown method
        // The thread will terminate when the application exits
        // For a cleaner shutdown, we'd need to send a shutdown request to the server

        // Wait for thread (with timeout)
        if (server_thread->joinable()) {
            server_thread->join();
        }
    }
    running = false;
}

bool HttpApiServer::IsRunning() const {
    return running;
}

ServerConfig HttpApiServer::GetConfig() const {
    return config;
}
