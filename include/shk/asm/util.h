#pragma once

#define HI8(U16) (static_cast<uint8_t>((U16 & 0xFF00u) >> 8u))
#define LO8(U16) (static_cast<uint8_t>(U16 & 0x00FFu))

namespace shk {
	std::vector<std::string_view> split(std::string_view sv, char delim, int max_splits = 0) {
		std::vector<std::string_view> ret;

		size_t i = 0;

		while(sv.size() >= i && sv[i] == delim) {
			++i;
		}

		size_t first = i;
		size_t count;

		for(count = 0; first + count < sv.size(); ++count) {
			if(sv[first + count] == delim) {
				ret.emplace_back(sv.substr(first, count));
				first = first + count + 1;
				count = 0;

				if(max_splits && ret.size() >= max_splits) {
					count = sv.size() - first;
				}
			}
		}

		if(count > 0) {
			ret.emplace_back(sv.substr(first, count));
		}

		return ret;
	}

	uint16_t parse_literal(std::string_view sv) {
		if(sv.empty()) {
			std::cerr << "error: expected numeric literal" << std::endl;
			return 0;
		}

		int base = 10;

		if(sv[0] == '0' && sv.size() >= 2) {
			if(sv[1] == 'X' || sv[1] == 'x') {
				base = 16;
				sv.remove_prefix(2);
			} else if(sv[1] == 'O' || sv[1] == 'o') {
				base = 8;
				sv.remove_prefix(2);
			} else if(sv[1] == 'B' || sv[1] == 'b') {
				base = 2;
				sv.remove_prefix(2);
			} else {
				base = 8;
				sv.remove_prefix(1);
			}
		}

		uint16_t ret = 0;

		auto result = std::from_chars(sv.data(), sv.data() + sv.size(), ret, base);
		if(result.ptr == sv.data()) {
			std::cerr << "error: failed to parse numeric literal: " << sv << std::endl;
			return 0;
		}

		return ret;
	}
} // namespace shk
