#include <limits.h>
#include <string.h>
#include <unistd.h>

#include <bitset>
#include <fstream>
#include <iostream>
#include <sstream>
#include <unordered_map>
#include <vector>

#define HI8(U16) (static_cast<uint8_t>((U16 & 0xFF00u) >> 8u))
#define LO8(U16) (static_cast<uint8_t>(U16 & 0x00FFu))

enum class Opcode : uint8_t {
	noop    = 0b0000,
	halt    = 0b0010,
	die     = 0b0011,

	load    = 0b0100,
	store   = 0b0101,

	move    = 0b1000,
	add     = 0b1010,
	compare = 0b1011,
};

struct Operand {
	enum class Type : uint8_t {
		imm = 0b0,
		reg = 0b1,

		mem,
	};

	Type type;
	uint16_t value;
};

struct Command {
	enum class Type : uint8_t {
		eq = 0b0000,
	};

	Type type;
	std::vector<Operand> operands;
};

struct Instruction {
	Opcode opcode;
	std::vector<Operand> operands;
	std::vector<Command> commands;
};

std::ostream & operator<<(std::ostream &os, const Opcode opcode) {
	return os << static_cast<int>(opcode);
}

std::ostream & operator<<(std::ostream &os, const Operand::Type type) {
	return os << static_cast<int>(type);
}

const std::unordered_map<std::string, Opcode> opcode_map {
	{"NOP", Opcode::noop},
	{"HLT", Opcode::halt},
	{"DIE", Opcode::die},

	{"LOD", Opcode::load},
	{"STO", Opcode::store},

	{"MOV", Opcode::move},
	{"ADD", Opcode::add},
	{"CMP", Opcode::compare},
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

Operand process_operand(std::string_view operand_str) {
	Operand operand;

	switch(operand_str[0]) {
	case '#':
		operand.type = Operand::Type::imm;
		operand.value = 0;
		break;
	case '$':
		operand.type = Operand::Type::reg;
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
		operand.type = Operand::Type::mem;
		operand.value = 0;
		break;
	}

	return operand;
}

std::vector<Instruction> process(std::istream &is) {
	std::vector<Instruction> ret;

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

		Instruction instr;
		instr.opcode = it->second;

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
				Command command;
				command.type = Command::Type::eq;
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

void encode(std::ostream &os, const std::vector<Instruction> &instrs) {
	for(auto &instr : instrs) {
		for(auto &command : instr.commands) {
			uint16_t byte = static_cast<uint16_t>(command.type);
			byte |= 1u << 15u;
			std::cout << std::bitset<16>(byte) << ' ';
			os << HI8(byte) << LO8(byte);

			for(auto &operand : command.operands) {
				uint16_t byte = operand.value;

				if(operand.type != Operand::Type::mem) {
					byte |= static_cast<uint16_t>(operand.type) << 15u;
				}

				std::cout << std::bitset<16>(byte) << ' ';
				os << HI8(byte) << LO8(byte);
			}
		}

		uint16_t byte = static_cast<uint16_t>(instr.opcode);
		std::cout << std::bitset<16>(byte);
		os << HI8(byte) << LO8(byte);

		for(auto &operand : instr.operands) {
			uint16_t byte = operand.value;

			if(operand.type != Operand::Type::mem) {
				byte |= static_cast<uint16_t>(operand.type) << 15u;
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
