#pragma once
#include <glaze/glaze.hpp>
#include "model.hpp"
#include <sstream>
#include <iomanip>
#include <ctime>

namespace malvex::json {
    using namespace glz;

    // ISO8601 utilities (UTC)
    inline std::string timepoint_to_iso(TimePoint& tp) {
        auto t = std::chrono::system_clock::to_time_t(tp);
        std::tm tm{};
#if defined(_MSC_VER)
        gmtime_s(&tm, &t);
#else
        gmtime_r(&t, &tm);
#endif
        char buf[64];
        std::strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%SZ", &tm);
        return std::string(buf);
    }

    inline TimePoint timepoint_from_iso(const std::string& s) {
        std::tm tm{};
#if defined(_MSC_VER)
        std::istringstream ss(s);
        ss >> std::get_time(&tm, "%Y-%m-%dT%H:%M:%SZ");
        time_t tt = _mkgmtime(&tm);
#else
        strptime(s.c_str(), "%Y-%m-%dT%H:%M:%SZ", &tm);
        time_t tt = timegm(&tm);
#endif
        return std::chrono::system_clock::from_time_t(tt);
    }

    // Specialize glaze meta for TimePoint
    template<>
    struct glz::meta<TimePoint> {
        static constexpr auto value = transform(
            [](const auto &t){ return timepoint_to_iso(t); },
            [](const auto &s){ return timepoint_from_iso(s); }
        );
    };

    // Provide meta for each model (we reflect the fields we want serialized)
    template<> struct glz::meta<User> {
        static constexpr auto value = object(
            member("id",&User::id),
            member("uid",&User::uid),
            member("username",&User::username),
            member("clearance",&User::clearance)
        );
    };

    template<> struct glz::meta<Victim> {
        static constexpr auto value = object(
            member("id",&Victim::id),
            member("uid",&Victim::uid),
            member("internal_ip",&Victim::internal_ip),
            member("external_ip",&Victim::external_ip),
            member("hostname",&Victim::hostname),
            member("username",&Victim::username),
            member("operating_system",&Victim::operating_system),
            member("last_update",&Victim::last_update),
            member("status",&Victim::status),
            member("template_id",&Victim::template_id)
        );
    };

    // similarly add meta for Job, Output, Connection, Event as needed
    template<> struct glz::meta<Log> {
        static constexpr auto value = object(
            member("id",&Log::id),
            member("uid",&Log::uid),
            member("prev",&Log::prev),
            member("nonce",&Log::nonce),
            member("command",&Log::command),
            member("signature",&Log::signature),
            member("status",&Log::status)
        );
    };

    template<> struct glz::meta<Result> {
        static constexpr auto value = object(
            member("id",&Result::id),
            member("uid",&Result::uid),
            member("data",&Result::data)
        );
    };

    template<> struct glz::meta<Session> {
        static constexpr auto value = object(
            member("id",&Session::id),
            member("uid",&Session::uid),
            member("port",&Session::port)
        );
    };

    template<> struct glz::meta<Log> {
        static constexpr auto value = object(
            member("id",&Log::id),
            member("uid",&Log::uid),
            member("key",&Log::key),
            member("value",&Log::value),
            member("time",&Log::time)
        );
    }
}