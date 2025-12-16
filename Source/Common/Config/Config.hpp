// Config.hpp
#pragma once

#include "ConfigRepository.hpp"

#include <string>
#include <unordered_map>
#include <memory>
#include <optional>
#include <mutex>
#include <sstream>
#include <type_traits>
#include <unordered_set>

namespace Key {
    constexpr const char* client_username_key = "malvex_username";
    constexpr const char* client_password_key = "malvex_password";
    constexpr const char* client_server_url_key = "malvex_server_url";
    constexpr const char* client_bearer_token_key = "malvex_bearar_token";
    constexpr const char* client_output_dir_key = "malvex_out_dir";
}

class Config {
public:
    // Delete copy and move constructors/assignments (singleton pattern)
    Config(const Config&) = delete;
    Config& operator=(const Config&) = delete;
    Config(Config&&) = delete;
    Config& operator=(Config&&) = delete;

    static Config& instance(const std::string& db_path = "config.db") {
        static Config instance(db_path);
        return instance;
    }

    template<typename T>
    T get(const std::string& key, const T& default_value = T{}) {
        std::lock_guard<std::mutex> lock(mutex_);

        auto it = cache_.find(key);
        if (it != cache_.end()) {
            return from_string<T>(it->second);
        }

        // Not in cache, try loading from database
        auto dao = repository_->get(key);
        if (dao) {
            cache_[key] = dao->value;
            return from_string<T>(dao->value);
        }

        // Key doesn't exist, return default
        return default_value;
    }

    template<typename T>
    void set(const std::string& key, const T& value, bool immediate = false) {
        std::lock_guard<std::mutex> lock(mutex_);

        std::string str_value = to_string(value);
        cache_[key] = str_value;

        if (immediate) {
            save_to_db(key, str_value);
        } else {
            dirty_keys_.insert(key);
        }
    }

    bool has(const std::string& key) {
        std::lock_guard<std::mutex> lock(mutex_);

        if (cache_.find(key) != cache_.end()) {
            return true;
        }

        auto dao = repository_->get(key);
        if (dao) {
            cache_[key] = dao->value;
            return true;
        }

        return false;
    }

    /**
     * Remove a config key
     */
    bool remove(const std::string& key) {
        std::lock_guard<std::mutex> lock(mutex_);

        cache_.erase(key);
        dirty_keys_.erase(key);
        return repository_->remove(key);
    }

    /**
     * Get all config keys
     */
    std::vector<std::string> keys() {
        std::lock_guard<std::mutex> lock(mutex_);

        std::vector<std::string> result;
        for (const auto& [key, value] : cache_) {
            result.push_back(key);
        }
        return result;
    }

    void save() {
        std::lock_guard<std::mutex> lock(mutex_);

        for (const auto& key : dirty_keys_) {
            auto it = cache_.find(key);
            if (it != cache_.end()) {
                save_to_db(key, it->second);
            }
        }
        dirty_keys_.clear();
    }

    void reload() {
        std::lock_guard<std::mutex> lock(mutex_);

        cache_.clear();
        dirty_keys_.clear();
        load_all_from_db();
    }

    void clear() {
        std::lock_guard<std::mutex> lock(mutex_);

        cache_.clear();
        dirty_keys_.clear();

        // Delete all from database
        auto all = repository_->list();
        for (const auto& dao : all) {
            repository_->remove(dao.id);
        }
    }

    template<typename T>
    class ScopedValue {
    public:
        ScopedValue(Config& config, const std::string& key, T value)
            : config_(config), key_(key), value_(std::move(value)) {}

        ~ScopedValue() {
            config_.set(key_, value_);
        }

        T& get() { return value_; }
        const T& get() const { return value_; }

        void operator=(const T& new_value) { value_ = new_value; }
        operator T&() { return value_; }
        operator const T&() const { return value_; }

    private:
        Config& config_;
        std::string key_;
        T value_;
    };

    template<typename T>
    ScopedValue<T> scoped(const std::string& key, const T& default_value = T{}) {
        return ScopedValue<T>(*this, key, get<T>(key, default_value));
    }

    // Destructor saves all dirty values
    ~Config() {
        save();
    }

private:
    explicit Config(const std::string& db_path)
        : repository_(std::make_unique<ConfigRepository>(db_path)) {
        load_all_from_db();
    }

    void load_all_from_db() {
        auto all = repository_->list();
        for (const auto& dao : all) {
            cache_[dao.key] = dao.value;
        }
    }

    void save_to_db(const std::string& key, const std::string& value) {
        repository_->upsert(key, value);
    }

    // Type conversion helpers
    template<typename T>
    std::string to_string(const T& value) {
        if constexpr (std::is_same_v<T, std::string>) {
            return value;
        } else if constexpr (std::is_same_v<T, bool>) {
            return value ? "true" : "false";
        } else if constexpr (std::is_arithmetic_v<T>) {
            return std::to_string(value);
        } else {
            std::ostringstream oss;
            oss << value;
            return oss.str();
        }
    }

    template<typename T>
    T from_string(const std::string& str) {
        if constexpr (std::is_same_v<T, std::string>) {
            return str;
        } else if constexpr (std::is_same_v<T, bool>) {
            return str == "true" || str == "1" || str == "yes";
        } else if constexpr (std::is_same_v<T, int>) {
            return std::stoi(str);
        } else if constexpr (std::is_same_v<T, long>) {
            return std::stol(str);
        } else if constexpr (std::is_same_v<T, long long>) {
            return std::stoll(str);
        } else if constexpr (std::is_same_v<T, float>) {
            return std::stof(str);
        } else if constexpr (std::is_same_v<T, double>) {
            return std::stod(str);
        } else {
            std::istringstream iss(str);
            T value;
            iss >> value;
            return value;
        }
    }

    std::unique_ptr<ConfigRepository> repository_;
    std::unordered_map<std::string, std::string> cache_;
    std::unordered_set<std::string> dirty_keys_;
    mutable std::mutex mutex_;
};

// ===== Usage Examples =====

/*
// Example 1: Basic usage
void example_basic() {
    auto& config = Config::instance("app.db");

    // Set values (automatically saved to database)
    config.set("server_url", std::string("https://api.example.com"));
    config.set("port", 8080);
    config.set("enable_logging", true);
    config.set("timeout", 30.5);

    // Get values with defaults
    auto url = config.get<std::string>("server_url", "http://localhost");
    auto port = config.get<int>("port", 3000);
    auto logging = config.get<bool>("enable_logging", false);
    auto timeout = config.get<double>("timeout", 10.0);

    std::cout << "URL: " << url << std::endl;
    std::cout << "Port: " << port << std::endl;
    std::cout << "Logging: " << (logging ? "enabled" : "disabled") << std::endl;
}

// Example 2: Deferred saving for performance
void example_batch_update() {
    auto& config = Config::instance();

    // Set multiple values without immediate DB writes
    config.set("setting1", 100, false);  // immediate=false
    config.set("setting2", 200, false);
    config.set("setting3", 300, false);

    // Save all at once
    config.save();
}

// Example 3: Scoped values (RAII auto-save)
void example_scoped() {
    auto& config = Config::instance();

    {
        auto counter = config.scoped<int>("request_count", 0);
        counter.get()++;  // Modify the value
        counter.get()++;
        // Automatically saved to database when counter goes out of scope
    }
}

// Example 4: Check existence
void example_check() {
    auto& config = Config::instance();

    if (config.has("api_key")) {
        auto key = config.get<std::string>("api_key");
        // Use the key...
    } else {
        config.set("api_key", std::string("default_key"));
    }
}

// Example 5: Thread-safe access
void example_threadsafe() {
    auto& config = Config::instance();

    std::thread t1([&]() {
        for (int i = 0; i < 100; ++i) {
            config.set("counter", config.get<int>("counter", 0) + 1);
        }
    });

    std::thread t2([&]() {
        for (int i = 0; i < 100; ++i) {
            config.set("counter", config.get<int>("counter", 0) + 1);
        }
    });

    t1.join();
    t2.join();

    std::cout << "Final counter: " << config.get<int>("counter") << std::endl;
}

// Example 6: Application settings class
class AppSettings {
public:
    static std::string getServerUrl() {
        return Config::instance().get<std::string>("server_url", "http://localhost:8080");
    }

    static void setServerUrl(const std::string& url) {
        Config::instance().set("server_url", url);
    }

    static int getMaxConnections() {
        return Config::instance().get<int>("max_connections", 10);
    }

    static void setMaxConnections(int max) {
        Config::instance().set("max_connections", max);
    }

    static bool isDebugMode() {
        return Config::instance().get<bool>("debug_mode", false);
    }

    static void setDebugMode(bool enabled) {
        Config::instance().set("debug_mode", enabled);
    }
};

// Usage in your GUI
void drawSettingsTab() {
    static char server_url[256];
    static bool debug_mode;
    static int max_connections;

    // Load from config on first render
    static bool initialized = false;
    if (!initialized) {
        auto url = AppSettings::getServerUrl();
        strncpy(server_url, url.c_str(), sizeof(server_url) - 1);
        debug_mode = AppSettings::isDebugMode();
        max_connections = AppSettings::getMaxConnections();
        initialized = true;
    }

    // Render GUI controls
    GuiTextBox({100, 100, 300, 30}, server_url, 256, false);
    GuiCheckBox({100, 150, 20, 20}, "Debug Mode", &debug_mode);
    GuiSlider({100, 200, 300, 20}, "Max Connections", "", &max_connections, 1, 100);

    if (GuiButton({100, 250, 100, 30}, "Save")) {
        AppSettings::setServerUrl(server_url);
        AppSettings::setDebugMode(debug_mode);
        AppSettings::setMaxConnections(max_connections);
    }
}
*/