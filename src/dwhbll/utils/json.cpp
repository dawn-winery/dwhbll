#include <dwhbll/utils/utils.hpp>

#include <dwhbll/utils/json.hpp>
#include <sstream>

namespace dwhbll::json {

std::string escape_string(const std::string_view& str) {
    std::string s = utils::replace_all(str, "\"", "\\\"");
    return utils::replace_all(s, "\\", "\\\\");
}

json& json::operator[](size_t index) {
    assert(is_array());
    return std::get<json_array>(value).at(index);
}

const json& json::operator[](size_t index) const {
    assert(is_array());
    return std::get<json_array>(value).at(index);
}

// Object key access
json& json::operator[](std::string_view key) {
    assert(is_object());
    return std::get<json_object>(value)[key];
}

const json& json::operator[](std::string_view key) const {
    assert(is_object());
    auto& obj = std::get<json_object>(value);
    auto it = obj.find(key);
    if (it == obj.end())
        throw std::out_of_range("Key not found: " + std::string(key));
    return it->second;
}

std::string json::dump() const {
    // This is all just horrible and I hate it
    if(is_null())
        return "null";
    if(is_string())
        return "\"" + escape_string(as_string()) + "\"";
    if(is_integer())
        return std::to_string(as_integer());
    if(is_float())
        return std::to_string(as_float());
    if(is_bool())
        return as_bool() ? "true" : "false";

    std::ostringstream ss;
    if(is_object()) {
        json_object members = as_object();
        ss << "{";
        if(!members.empty())
            ss << " ";

        auto it = members.begin();
        while(it != members.end()) {
            ss << "\"" << escape_string(it->first) << "\"" << ": " << it->second.dump();

            ++it;
            if(it != members.end())
                ss << ", ";
        }

        if(!members.empty())
            ss << " ";
        ss << "}";
    }
    else if(is_array()) {
        json_array elements = as_array();
        ss << "[";
        if(!elements.empty())
            ss << " ";

        for(size_t i = 0; i < elements.size(); i++) {
            ss << elements[i].dump();
            if(i != elements.size() - 1)
                ss << ", ";
        }

        if(!elements.empty())
            ss << " ";
        ss << "]";
    }
    return ss.str();
}

std::string json::format_internal(int indentation, int cur_indentation) const {
    if(!is_object() && !is_array())
        return dump();

    std::string base_ind(cur_indentation, ' ');
    cur_indentation += indentation;

    std::ostringstream ss;
    std::string ind(cur_indentation, ' ');
    if(is_object()) {
        json_object members = as_object();
        ss << "{";
        if(!members.empty())
            ss << "\n";

        auto it = members.begin();
        while(it != members.end()) {
            ss << ind << "\"" << escape_string(it->first) << "\"" 
                << ": " << it->second.format_internal(indentation, cur_indentation);

            ++it;
            if(it != members.end())
                ss << ",\n";
        }

        if(!members.empty())
            ss << "\n" << base_ind;
        ss << "}";
    }
    else if(is_array()) {
        json_array elements = as_array();
        ss << "[";
        if(!elements.empty())
            ss << "\n";

        for(size_t i = 0; i < elements.size(); i++) {
            ss << ind << elements[i].format_internal(indentation, cur_indentation);
            if(i != elements.size() - 1)
                ss << ",\n";
        }

        if(!elements.empty())
            ss << "\n" << base_ind;
        ss << "]";
    }
    return ss.str();
   
}

std::string json::format(int indentation) const {
    return format_internal(indentation, 0);
}

}
