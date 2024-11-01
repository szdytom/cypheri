#include "cypheri/nametable.hpp"
#include <cassert>

namespace cypheri {

NameIdType NameTable::get_id(std::string_view name) const {
    auto it = name_to_id.find(name);
    if (it != name_to_id.end()) {
        return it->second;
    }
	return NameTable::INVALID_ID;
}

NameIdType NameTable::get_id_or_insert(std::string_view name) {
    auto it = name_to_id.find(name);
    if (it != name_to_id.end()) {
        return it->second;
    }
	names.emplace_back(name);
	return name_to_id[std::string_view{names.back()}] = names.size() - 1;
}

const std::string& NameTable::get_name(NameIdType id) const {
	assert(id >= 0 && id < names.size());
    return names[id];
}

size_t NameTable::size() const {
    return names.size();
}

} // namespace cypheri
