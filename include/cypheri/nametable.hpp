#ifndef CYPHERI_NAMETABLE_HPP
#define CYPHERI_NAMETABLE_HPP

#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>

namespace cypheri {

using NameIdType = uint32_t;

class NameTable {
public:
	static constexpr NameIdType INVALID_ID = -1;

	NameIdType get_id(std::string_view name) const;
	NameIdType get_id_or_insert(std::string_view name);
	std::string_view get_name(NameIdType id) const;
	size_t size() const;

private:
	struct Item {
		std::unique_ptr<char[]> name;
		size_t count;
	};
	std::vector<Item> names;
	std::unordered_map<std::string_view, NameIdType> name_to_id;
};

template <typename T> using SpraseNameArray = std::unordered_map<NameIdType, T>;

} // namespace cypheri

#endif // CYPHERI_NAMETABLE_HPP
