#pragma once

#include <string>
#include <vector>

#include <glaze/glaze.hpp>

struct IDao {
    std::string to_json() const {
        return glz::write_json(*this).value_or("{}");
    }
};

template<typename T>
std::string to_json_array(const std::vector<T>& items) {
    return glz::write_json(items).value_or("[]");
}

// Parse JSON array to vector of DAOs
template<typename T>
std::vector<T> from_json_array(const std::string& json) {
    std::vector<T> items;
    glz::read_json(items, json);
    return items;
}
