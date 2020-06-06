#include <limits.h>
#include <string.h>
#include <unistd.h>

#include <bitset>
#include <charconv>
#include <fstream>
#include <iostream>
#include <sstream>
#include <vector>

#include <shk.h>

#define HI8(U16) (static_cast<uint8_t>((U16 & 0xFF00u) >> 8u))
#define LO8(U16) (static_cast<uint8_t>(U16 & 0x00FFu))

std::ostream & operator<<(std::ostream &os, const shk::operand::type type) {
	return os << static_cast<int>(type);
}

std::vector<std::string_view> split(std::string_view sv, char delim) {
	std::vector<std::string_view> ret;

	size_t i = 0;

	while(sv.size() >= i && sv[i] == ' ') {
		++i;
	}

	size_t first = i;
	size_t count;

	for(count = 0; first + count < sv.size(); ++count) {
		if(sv[first + count] == ' ') {
			ret.emplace_back(sv.substr(first, count));
			first = first + count + 1;
			count = 0;
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

	if(sv[0] == '0') {
		if(sv.size() >= 2 && (sv[1] == 'X' || sv[1] == 'x')) {
			base = 16;
			sv.remove_prefix(2);
		} else if(sv.size() >= 2 && (sv[1] == 'O' || sv[1] == 'o')) {
			base = 8;
			sv.remove_prefix(2);
		} else if(sv.size() >= 2 && (sv[1] == 'B' || sv[1] == 'b')) {
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
		std::cerr << "error: failed to parse numeric literal" << std::endl;
		return 0;
	}

	return ret;
}

shk::operand process_operand(std::string_view operand_str) {
	shk::operand operand;

	switch(operand_str[0]) {
	case '#':
		operand.ty = shk::operand::type::imm;
		operand.value = parse_literal(operand_str.substr(1));
		break;
	case '$':
		operand.ty = shk::operand::type::reg;
		operand.value = parse_literal(operand_str.substr(1));
		break;
	case '0':
	case '1':
	case '2':
	case '3':
	case '4':
	case '5':
	case '6':
	case '7':
	case '8':
	case '9':
		operand.ty = shk::operand::type::mem;
		operand.value = parse_literal(operand_str);
		break;
	}

	return operand;
}

std::vector<shk::instruction> process(std::istream &is) {
	std::vector<shk::instruction> ret;

	std::string line;
	while(std::getline(is, line)) {
		std::istringstream lss(line);

		std::string opcode_str;
		lss >> opcode_str;

		auto opcode = shk::mnemonic_to_opcode(opcode_str);
		if(!opcode) {
			std::cerr << opcode_str << ": invalid opcode" << std::endl;
			return {};
		}

		shk::instruction instr;
		instr.op = *opcode;

		std::string operand_str;
		while(std::getline(lss, operand_str, ',')) {
			if(operand_str.find_first_not_of(' ') == std::string::npos) {
				break;
			}

			auto words = split(operand_str, ' ');

			if(words.empty()) {
				std::cerr << "syntax error" << std::endl;
				std::cerr << '"' << operand_str << '"' << std::endl;
				return {};
			}

			if(words[0][0] == '!') {
				shk::command command;
				command.ty = shk::command::type::eq;
				for(size_t w = 1; w < words.size(); ++w) {
					command.operands.emplace_back(process_operand(words[w]));
				}
				instr.commands.emplace_back(command);
			} else {
				instr.operands.emplace_back(process_operand(words[0]));
			}
		}

		ret.emplace_back(instr);
	}

	return ret;
}

void encode(std::ostream &os, const shk::instruction &instr) {
	for(auto &command : instr.commands) {
		uint16_t byte = static_cast<uint16_t>(command.ty);
		byte |= 1u << 15u;
		std::cout << std::bitset<16>(byte) << ' ';
		os << HI8(byte) << LO8(byte);

		for(auto &operand : command.operands) {
			uint16_t byte = operand.value;

			if(operand.ty != shk::operand::type::mem) {
				byte |= static_cast<uint16_t>(operand.ty) << 15u;
			}

			std::cout << std::bitset<16>(byte) << ' ';
			os << HI8(byte) << LO8(byte);
		}
	}

	uint16_t byte = static_cast<uint16_t>(instr.op);
	std::cout << std::bitset<16>(byte);
	os << HI8(byte) << LO8(byte);

	for(auto &operand : instr.operands) {
		uint16_t byte = operand.value;

		if(operand.ty != shk::operand::type::mem) {
			byte |= static_cast<uint16_t>(operand.ty) << 15u;
		}

		std::cout << ' ' << std::bitset<16>(byte);
		os << HI8(byte) << LO8(byte);
	}
	std::cout << std::endl;
}

int main(int argc, char *argv[]) {
	char out_path[PATH_MAX];
	strcpy(out_path, "a.out");

	int opt;
	while((opt = getopt(argc, argv, "o:")) != -1) {
		switch(opt) {
		case 'o':
			strcpy(out_path, optarg);
			break;
		case '?':
			return 1;
		}
	}

	size_t in_count = argc - optind;
	if(in_count == 0) {
		std::cerr << argv[0] << ": no input files" << std::endl;
		return 1;
	}

	std::cout << "out_path = " << out_path << std::endl;
	std::ofstream os(out_path, std::ios::out | std::ios::binary | std::ios::trunc);

	for(size_t i = 0; i < in_count; ++i) {
		char *in_path = argv[i + optind];
		std::cout << "in[" << i << "] = " << in_path << std::endl;

		std::ifstream is(in_path, std::ios::in);
		if(is.fail()) {
			std::cerr << argv[0] << ": " << in_path << ": failed to open" << std::endl;
			return 1;
		}

		auto instrs = process(is);
		if(instrs.empty()) {
			return 2;
		}

		for(auto &instr : instrs) {
			encode(os, instr);
		}
	}

	return 0;
}
