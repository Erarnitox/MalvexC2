#include "OperatorDAO.hpp"

namespace {

struct OperatorPublicView {
    int64_t operator_id;
    UUID operator_uid;
    std::string username;
    int clearance;
};

} // namespace

template <>
struct glz::meta<OperatorPublicView> {
    using T = OperatorPublicView;
    static constexpr auto value = object(
        "operator_id", &T::operator_id,
        "operator_uid", &T::operator_uid,
        "username", &T::username,
        "clearance", &T::clearance
    );
};

//--------------------------------
//
//--------------------------------
std::string OperatorDAO::to_public_json() const {
    const OperatorPublicView view{
        .operator_id = id,
        .operator_uid = uid,
        .username = username,
        .clearance = clearance
    };

    return glz::write_json(view).value_or("{}");
}

//--------------------------------
//
//--------------------------------
std::string to_public_json_array(const std::vector<OperatorDAO>& items) {
    std::string json = "[";

    for (std::size_t i = 0; i < items.size(); ++i) {
        if (i > 0) {
            json.push_back(',');
        }
        json += items[i].to_public_json();
    }

    json.push_back(']');
    return json;
}
