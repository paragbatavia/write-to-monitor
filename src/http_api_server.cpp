#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>

#include "httplib.h"
#include "http_api_server.h"
#include "thread_safe_control.h"
#include "config_parser.h"
#include <sstream>
#include <stdio.h>
#include <cstdarg>

// ServerLogger implementation
std::ofstream ServerLogger::log_file;
std::mutex ServerLogger::log_mutex;

void ServerLogger::Init(const std::string& log_path) {
    std::lock_guard<std::mutex> lock(log_mutex);
    if (log_file.is_open()) {
        log_file.close();
    }
    log_file.open(log_path, std::ios::out | std::ios::app);
    if (log_file.is_open()) {
        // Write startup marker
        SYSTEMTIME st;
        GetLocalTime(&st);
        char timestamp[64];
        snprintf(timestamp, sizeof(timestamp), "%04d-%02d-%02d %02d:%02d:%02d.%03d",
                 st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);
        log_file << "\n=== Log started at " << timestamp << " ===" << std::endl;
    }
}

void ServerLogger::Log(const char* level, const char* format, ...) {
    std::lock_guard<std::mutex> lock(log_mutex);
    if (!log_file.is_open()) return;

    // Get timestamp
    SYSTEMTIME st;
    GetLocalTime(&st);
    char timestamp[64];
    snprintf(timestamp, sizeof(timestamp), "%04d-%02d-%02d %02d:%02d:%02d.%03d",
             st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);

    // Format message
    char message[512];
    va_list args;
    va_start(args, format);
    vsnprintf(message, sizeof(message), format, args);
    va_end(args);

    log_file << "[" << timestamp << "] [" << level << "] " << message << std::endl;
    log_file.flush();
}

void ServerLogger::Close() {
    std::lock_guard<std::mutex> lock(log_mutex);
    if (log_file.is_open()) {
        log_file << "=== Log closed ===" << std::endl;
        log_file.close();
    }
}

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
    : running(false), should_stop(false), bind_attempted(false), bind_succeeded(false), monitor_control(control) {
}

HttpApiServer::~HttpApiServer() {
    Stop();
}

void HttpApiServer::ServerThreadFunc() {
    httplib::Server server;

    // POST /api/brightness - Set brightness (0-100)
    server.Post("/api/brightness", [this](const httplib::Request& req, httplib::Response& res) {
        ServerLogger::Log("INFO", "POST /api/brightness - body: %s", req.body.c_str());

        float brightness;
        if (!ParseJsonFloat(req.body, "value", brightness)) {
            ServerLogger::Log("WARN", "Invalid brightness request - missing value");
            res.status = 400;
            res.set_content(CreateJsonResponse(false, "Invalid request: missing or invalid 'value' field"), "application/json");
            return;
        }

        if (brightness < 0.0f || brightness > 100.0f) {
            ServerLogger::Log("WARN", "Invalid brightness value: %.0f", brightness);
            res.status = 400;
            res.set_content(CreateJsonResponse(false, "Value must be between 0 and 100"), "application/json");
            return;
        }

        if (!monitor_control->IsInitialized()) {
            ServerLogger::Log("ERROR", "NvAPI not initialized for brightness request");
            res.status = 503;
            res.set_content(CreateJsonResponse(false, "NvAPI not initialized"), "application/json");
            return;
        }

        bool success = monitor_control->SetBrightness(brightness);
        ServerLogger::Log("INFO", "SetBrightness(%.0f) = %s", brightness, success ? "success" : "failed");
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
        ServerLogger::Log("INFO", "POST /api/contrast - body: %s", req.body.c_str());

        float contrast;
        if (!ParseJsonFloat(req.body, "value", contrast)) {
            ServerLogger::Log("WARN", "Invalid contrast request - missing value");
            res.status = 400;
            res.set_content(CreateJsonResponse(false, "Invalid request: missing or invalid 'value' field"), "application/json");
            return;
        }

        if (contrast < 0.0f || contrast > 100.0f) {
            ServerLogger::Log("WARN", "Invalid contrast value: %.0f", contrast);
            res.status = 400;
            res.set_content(CreateJsonResponse(false, "Value must be between 0 and 100"), "application/json");
            return;
        }

        if (!monitor_control->IsInitialized()) {
            ServerLogger::Log("ERROR", "NvAPI not initialized for contrast request");
            res.status = 503;
            res.set_content(CreateJsonResponse(false, "NvAPI not initialized"), "application/json");
            return;
        }

        bool success = monitor_control->SetContrast(contrast);
        ServerLogger::Log("INFO", "SetContrast(%.0f) = %s", contrast, success ? "success" : "failed");
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
        ServerLogger::Log("INFO", "POST /api/input - body: %s", req.body.c_str());

        int source;
        if (!ParseJsonInt(req.body, "source", source)) {
            ServerLogger::Log("WARN", "Invalid input request - missing source");
            res.status = 400;
            res.set_content(CreateJsonResponse(false, "Invalid request: missing or invalid 'source' field"), "application/json");
            return;
        }

        if (source < 1 || source > 4) {
            ServerLogger::Log("WARN", "Invalid input source: %d", source);
            res.status = 400;
            res.set_content(CreateJsonResponse(false, "Source must be between 1 and 4 (1=HDMI 1, 2=HDMI 2, 3=DisplayPort, 4=USB-C)"), "application/json");
            return;
        }

        if (!monitor_control->IsInitialized()) {
            ServerLogger::Log("ERROR", "NvAPI not initialized for input request");
            res.status = 503;
            res.set_content(CreateJsonResponse(false, "NvAPI not initialized"), "application/json");
            return;
        }

        const char* input_names[] = {"HDMI 1", "HDMI 2", "DisplayPort", "USB-C"};
        ServerLogger::Log("INFO", "Switching input to %s (source=%d)", input_names[source - 1], source);
        bool success = monitor_control->SetInputSource(source);
        ServerLogger::Log("INFO", "SetInputSource(%d) = %s", source, success ? "success" : "failed");
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
        ServerLogger::Log("INFO", "GET /api/status");
        std::ostringstream fields;
        fields << "\"brightness\": " << static_cast<int>(monitor_control->GetBrightness());
        fields << ", \"contrast\": " << static_cast<int>(monitor_control->GetContrast());
        fields << ", \"display_index\": " << monitor_control->GetSelectedDisplay();
        fields << ", \"nvapi_initialized\": " << (monitor_control->IsInitialized() ? "true" : "false");
        fields << ", \"status_message\": \"" << monitor_control->GetStatusMessage() << "\"";

        res.set_content("{" + fields.str() + "}", "application/json");
    });

    // GET /health - Health check
    server.Get("/health", [this](const httplib::Request& req, httplib::Response& res) {
        ServerLogger::Log("INFO", "GET /health");
        res.set_content("{\"status\": \"ok\", \"version\": \"1.0.0\"}", "application/json");
    });

    // Log that we're attempting to bind
    ServerLogger::Log("INFO", "Attempting to bind to %s:%d", config.host.c_str(), config.port);

    // Ensure WSA is initialized (may be redundant but helps diagnose)
    WSADATA wsaData;
    int wsa_init_result = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (wsa_init_result != 0) {
        ServerLogger::Log("ERROR", "WSAStartup failed with error: %d", wsa_init_result);
    } else {
        ServerLogger::Log("INFO", "WSAStartup succeeded (or was already initialized)");
    }

    // Test getaddrinfo directly
    struct addrinfo hints = {}, *result = nullptr;
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    int gai_result = getaddrinfo(config.host.c_str(), std::to_string(config.port).c_str(), &hints, &result);
    if (gai_result != 0) {
        ServerLogger::Log("ERROR", "getaddrinfo failed: %d (%s)", gai_result, gai_strerrorA(gai_result));
    } else {
        ServerLogger::Log("INFO", "getaddrinfo succeeded for %s:%d", config.host.c_str(), config.port);
        freeaddrinfo(result);
    }

    // Try to bind first (this is a non-blocking check)
    if (!server.bind_to_port(config.host.c_str(), config.port)) {
        int wsa_error = WSAGetLastError();
        ServerLogger::Log("ERROR", "Failed to bind to %s:%d - WSA error code: %d", config.host.c_str(), config.port, wsa_error);

        // Signal bind failure
        {
            std::lock_guard<std::mutex> lock(bind_mutex);
            bind_attempted = true;
            bind_succeeded = false;
            running = false;
        }
        bind_cv.notify_one();
        return;
    }

    // Bind succeeded - signal and start listening
    ServerLogger::Log("INFO", "Successfully bound to %s:%d, starting to listen", config.host.c_str(), config.port);
    {
        std::lock_guard<std::mutex> lock(bind_mutex);
        bind_attempted = true;
        bind_succeeded = true;
        running = true;
    }
    bind_cv.notify_one();

    // Now listen (this blocks until server.stop() is called)
    if (!server.listen_after_bind()) {
        ServerLogger::Log("WARN", "Server listen loop ended");
    }

    running = false;
    ServerLogger::Log("INFO", "Server thread exiting");
}

bool HttpApiServer::Start(const ServerConfig& cfg) {
    if (running) {
        ServerLogger::Log("WARN", "Server already running");
        return false;
    }

    config = cfg;
    should_stop = false;
    bind_attempted = false;
    bind_succeeded = false;

    // Initialize logging - use absolute path next to executable
    char exe_path[MAX_PATH];
    GetModuleFileNameA(NULL, exe_path, MAX_PATH);
    std::string log_path(exe_path);
    size_t last_slash = log_path.find_last_of("\\/");
    if (last_slash != std::string::npos) {
        log_path = log_path.substr(0, last_slash + 1);
    }
    log_path += "monitor_control.log";
    ServerLogger::Init(log_path);
    ServerLogger::Log("INFO", "Starting HTTP API server on %s:%d", cfg.host.c_str(), cfg.port);

    try {
        server_thread = std::make_unique<std::thread>(&HttpApiServer::ServerThreadFunc, this);

        // Wait for bind to complete (with timeout)
        std::unique_lock<std::mutex> lock(bind_mutex);
        bool wait_result = bind_cv.wait_for(lock, std::chrono::seconds(5),
            [this]() { return bind_attempted.load(); });

        if (!wait_result) {
            ServerLogger::Log("ERROR", "Timeout waiting for server to start");
            return false;
        }

        if (!bind_succeeded) {
            ServerLogger::Log("ERROR", "Server failed to bind - check if port %d is in use", cfg.port);
            return false;
        }

        ServerLogger::Log("INFO", "Server started successfully");
        return true;
    } catch (const std::exception& e) {
        ServerLogger::Log("ERROR", "Exception starting server: %s", e.what());
        return false;
    } catch (...) {
        ServerLogger::Log("ERROR", "Unknown exception starting server");
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
