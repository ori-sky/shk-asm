#include <limits.h>
#include <string.h>
#include <unistd.h>

#include <bitset>
#include <fstream>
#include <iostream>
#include <sstream>
#include <unordered_map>
#include <vector>

#include <shk.h>

#define HI8(U16) (static_cast<uint8_t>((U16 & 0xFF00u) >> 8u))
#define LO8(U16) (static_cast<uint8_t>(U16 & 0x00FFu))

std::ostream & operator<<(std::ostream &os, const shk::opcode opcode) {
	return os << static_cast<int>(opcode);
}

std::ostream & operator<<(std::ostream &os, const shk::operand::type type) {
	return os << static_cast<int>(type);
}

const std::unordered_map<std::string, shk::opcode> opcode_map {
	{"NOP", shk::opcode::noop},
	{"HLT", shk::opcode::halt},
	{"DIE", shk::opcode::die},

	{"LOD", shk::opcode::load},
	{"STO", shk::opcode::store},

	{"MOV", shk::opcode::move},
	{"ADD", shk::opcode::add},
	{"CMP", shk::opcode::compare},
};

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

shk::operand process_operand(std::string_view operand_str) {
	shk::operand operand;

	switch(operand_str[0]) {
	case '#':
		operand.ty = shk::operand::type::imm;
		operand.value = 0;
		break;
	case '$':
		operand.ty = shk::operand::type::reg;
		operand.value = 0;
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
		operand.value = 0;
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

		auto it = opcode_map.find(opcode_str);
		if(it == opcode_map.end()) {
			std::cerr << opcode_str << ": invalid opcode" << std::endl;
			return {};
		}

		shk::instruction instr;
		instr.op = it->second;

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

void encode(std::ostream &os, const std::vector<shk::instruction> &instrs) {
	for(auto &instr : instrs) {
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

		encode(os, instrs);
	}

	return 0;
}
