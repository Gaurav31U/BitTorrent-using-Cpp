
#include <stack>
#include "../lib/nlohmann/json.hpp"
#include "bencode.h"
#include <queue>

using json = nlohmann::json;

namespace bencode {

    Encoder::Encoder(const json& decoded_value) {
        this->decoded_value = decoded_value;
    }

    std::string Encoder::bencode_value() {

        std::stack<BEncodeItem> objs;
        objs.push(BEncodeItem::val(decoded_value));
        while (!objs.empty()) {
            const BEncodeItem item = objs.top();
            objs.pop();

            if (item.kind == BEncodeItem::Kind::Token) {
                bencoded_value.push_back(item.token);
            } else {
                json val = item.value;
                if (val.is_object()) {
                    std::map<std::string, json> dict_map;
                    objs.push(BEncodeItem::tok('e'));
                    for (auto it = val.begin(); it != val.end(); ++it) {
                        dict_map[it.key()] = it.value();
                    }
                    for (auto it = dict_map.rbegin(); it != dict_map.rend(); ++it) {
                        objs.push(BEncodeItem::val(it->second));
                        objs.push(BEncodeItem::val(json(it->first)));
                    }
                    objs.push(BEncodeItem::tok('d'));
                } else if (val.is_array()) {
                    objs.push(BEncodeItem::tok('e'));
                    // to keep the order when popping the stack we need to reverse
                    for (auto it = val.rbegin(); it != val.rend(); ++it) {
                        objs.push(BEncodeItem::val(*it));
                    }
                    objs.push(BEncodeItem::tok('l'));
                } else if (val.is_binary()) {
                    const nlohmann::json::binary_t& bin = val.get_binary();
                    bencoded_value.append(std::to_string(bin.size()));
                    bencoded_value.push_back(':');
                    bencoded_value.append(reinterpret_cast<const char*>(bin.data()), bin.size());
                } else if (val.is_string()) {
                    const std::string &s = val.get_ref<const std::string&>();
                    const size_t len = s.length();
                    const std::string size_str = std::to_string(len);
                    bencoded_value.append(size_str);
                    bencoded_value.push_back(':');
                    bencoded_value.append(s);
                } else if (val.is_number_integer()) {
                    const int64_t i = val.get<int64_t>();
                    const std::string num_str = std::to_string(i);
                    bencoded_value.push_back('i');
                    bencoded_value.append(num_str);
                    bencoded_value.push_back('e');
                }
            }
        }
        return bencoded_value;
    }

}
