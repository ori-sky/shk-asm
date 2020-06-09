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
#include <shk/asm/util.h>

shk::operand process_operand(std::string_view operand_str) {
	shk::operand operand;

	switch(operand_str[0]) {
	case '#':
		operand.ty = shk::operand::type::imm;
		break;
	case '$':
		operand.ty = shk::operand::type::reg;
		break;
	case '*':
		operand.ty = shk::operand::type::deref;
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
		std::cerr << "error: unqualified numeric literal" << std::endl;
		break;
	}

	operand.value = shk::parse_literal(operand_str.substr(1));
	return operand;
}

std::vector<shk::instruction> process(std::istream &is) {
	std::vector<shk::instruction> ret;

	std::string line;
	while(std::getline(is, line)) {
		auto comment_split = shk::split(line, ';', 1);

		if(comment_split.empty()) {
			continue;
		}

		auto label_split = shk::split(comment_split[0], ':');

		if(label_split.empty()) {
			continue;
		}

		auto mnemonic_split = shk::split(label_split.back(), ' ', 1);

		if(mnemonic_split.empty()) {
			continue;
		}

		auto opcode_str = mnemonic_split[0];

		auto opcode = shk::mnemonic_to_opcode(opcode_str);
		if(!opcode) {
			std::cerr << "error: " << opcode_str << ": invalid opcode" << std::endl;
			return {};
		}

		shk::instruction instr;
		instr.op = *opcode;

		if(mnemonic_split.size() >= 2) {
			auto operand_split = shk::split(mnemonic_split[1], ',');
			for(auto &operand_str : operand_split) {
				if(operand_str.find_first_not_of(' ') == std::string::npos) {
					break;
				}

				auto words = shk::split(operand_str, ' ');

				if(words.empty()) {
					std::cerr << "error: syntax: ";
					std::cerr << '"' << operand_str << '"' << std::endl;
					return {};
				}

				if(words[0][0] == '!') {
					std::string command_str(words[0].substr(1));
					auto command_ty = shk::mnemonic_to_command(command_str);
					if(!command_ty) {
						std::cerr << "error: " << command_str << ": invalid command" << std::endl;
						return {};
					}

					shk::command command;
					command.ty = *command_ty;
					for(size_t w = 1; w < words.size(); ++w) {
						command.operands.emplace_back(process_operand(words[w]));
					}
					instr.commands.emplace_back(command);
				} else {
					instr.operands.emplace_back(process_operand(words[0]));
				}
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
			byte |= static_cast<uint16_t>(operand.ty) << 14u;

			std::cout << std::bitset<16>(byte) << ' ';
			os << HI8(byte) << LO8(byte);
		}
	}

	uint16_t byte = static_cast<uint16_t>(instr.op);
	std::cout << std::bitset<16>(byte);
	os << HI8(byte) << LO8(byte);

	for(auto &operand : instr.operands) {
		uint16_t byte = operand.value;
		byte |= static_cast<uint16_t>(operand.ty) << 14u;

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
