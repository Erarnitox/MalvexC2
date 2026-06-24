#include "VictimTemplateDAO.hpp"

namespace {

struct VictimTemplatePublicView {
    int64_t victim_template_id;
    UUID victim_template_uid;
    std::string username;
};

} // namespace

template <>
struct glz::meta<VictimTemplatePublicView> {
    using T = VictimTemplatePublicView;
    static constexpr auto value = object(
        "victim_template_id", &T::victim_template_id,
        "victim_template_uid", &T::victim_template_uid,
        "username", &T::username
    );
};

//--------------------------------
//
//--------------------------------
std::string VictimTemplateDAO::to_public_json() const {
    const VictimTemplatePublicView view{
        .victim_template_id = id,
        .victim_template_uid = uid,
        .username = username
    };

    return glz::write_json(view).value_or("{}");
}

//--------------------------------
//
//--------------------------------
std::string to_public_json_array(const std::vector<VictimTemplateDAO>& items) {
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
