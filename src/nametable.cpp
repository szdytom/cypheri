#include "cypheri/nametable.hpp"
#include <algorithm>
#include <cassert>
#include <exception>

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

	std::unique_ptr<char[]> name_clone = std::make_unique<char[]>(name.size());
	std::copy(name.begin(), name.end(), name_clone.get());
	std::string_view name_view{name_clone.get(), name.size()};
	names.push_back({std::move(name_clone), name.size()});
	return (name_to_id[name_view] = names.size() - 1);
}

std::string_view NameTable::get_name(NameIdType id) const {
	assert(id >= 0 && id < names.size());
	return {names[id].name.get(), names[id].count};
}

size_t NameTable::size() const {
	return names.size();
}

} // namespace cypheri
