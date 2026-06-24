#pragma once

#include <cstdint>
#include <string>
#include <string_view>

inline std::string extract_url_scheme(std::string_view url) {
    const auto scheme_end = url.find("://");
    if (scheme_end == std::string_view::npos) {
        return "https";
    }

    return std::string(url.substr(0, scheme_end));
}

inline std::string extract_url_host(std::string_view url) {
    std::string_view host = url;

    const auto scheme_end = host.find("://");
    if (scheme_end != std::string_view::npos) {
        host.remove_prefix(scheme_end + 3);
    }

    const auto path_start = host.find('/');
    if (path_start != std::string_view::npos) {
        host = host.substr(0, path_start);
    }

    const auto port_start = host.find(':');
    if (port_start != std::string_view::npos) {
        host = host.substr(0, port_start);
    }

    return std::string(host);
}

inline std::string_view extract_url_authority(std::string_view url) {
    std::string_view authority = url;

    if (const auto scheme_end = authority.find("://"); scheme_end != std::string_view::npos) {
        authority.remove_prefix(scheme_end + 3);
    }

    if (const auto path_start = authority.find('/'); path_start != std::string_view::npos) {
        authority = authority.substr(0, path_start);
    }

    return authority;
}

inline bool url_has_explicit_port(std::string_view url) {
    const auto authority = extract_url_authority(url);
    if (authority.empty()) {
        return false;
    }

    if (authority.front() == '[') {
        const auto bracket_end = authority.find(']');
        return bracket_end != std::string_view::npos
            && bracket_end + 1 < authority.size()
            && authority[bracket_end + 1] == ':';
    }

    return authority.find(':') != std::string_view::npos;
}

inline std::string make_victim_beacon_url(std::string_view reference_url, uint16_t victim_port = 3000) {
    return extract_url_scheme(reference_url) + "://" + extract_url_host(reference_url) + ":"
        + std::to_string(victim_port);
}

inline std::string normalize_victim_beacon_url(const std::string& url, uint16_t victim_port = 3000) {
    if (url.empty()) {
        return make_victim_beacon_url("https://127.0.0.1", victim_port);
    }

    return make_victim_beacon_url(url, victim_port);
}

inline std::string strip_trailing_slash(std::string url) {
    while (url.size() > 1 && url.back() == '/') {
        url.pop_back();
    }

    return url;
}
