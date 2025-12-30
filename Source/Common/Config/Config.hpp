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