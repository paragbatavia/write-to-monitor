#ifndef HTTP_API_SERVER_H
#define HTTP_API_SERVER_H

#include <thread>
#include <atomic>
#include <string>
#include <memory>

class ThreadSafeMonitorControl;

// Server configuration
struct ServerConfig {
    std::string host = "127.0.0.1";
    int port = 45678;
    bool enabled = true;

    // Load configuration from file
    static ServerConfig LoadConfig(const std::string& config_path);
};

// HTTP API Server
class HttpApiServer {
private:
    std::unique_ptr<std::thread> server_thread;
    std::atomic<bool> running;
    std::atomic<bool> should_stop;
    ServerConfig config;
    ThreadSafeMonitorControl* monitor_control;

    // Server thread function
    void ServerThreadFunc();

    // Helper function to create JSON response
    static std::string CreateJsonResponse(bool success, const std::string& message,
                                         const std::string& additional_fields = "");

public:
    HttpApiServer(ThreadSafeMonitorControl* control);
    ~HttpApiServer();

    // Start the HTTP server
    bool Start(const ServerConfig& cfg);

    // Stop the HTTP server
    void Stop();

    // Check if server is running
    bool IsRunning() const;

    // Get current configuration
    ServerConfig GetConfig() const;
};

#endif // HTTP_API_SERVER_H
