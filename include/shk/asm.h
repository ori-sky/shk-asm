#pragma once

#include <bitset>
#include <iostream>

#include <shk/asm/util.h>

namespace shk {
	class assembler {
	  private:
		std::vector<instruction> instrs;
	  public:
		bool process(std::istream &is) {
			std::string line;
			while(std::getline(is, line)) {
				auto comment_split = split(line, ';', 1);

				if(comment_split.empty()) {
					continue;
				}

				auto label_split = split(comment_split[0], ':');

				if(label_split.empty()) {
					continue;
				}

				auto mnemonic_split = split(label_split.back(), ' ', 1);

				if(mnemonic_split.empty()) {
					continue;
				}

				auto opcode_str = mnemonic_split[0];

				auto opcode = mnemonic_to_opcode(opcode_str);
				if(!opcode) {
					std::cerr << "error: " << opcode_str << ": invalid opcode" << std::endl;
					return {};
				}

				instruction instr;
				instr.op = *opcode;

				if(mnemonic_split.size() >= 2) {
					auto operand_split = split(mnemonic_split[1], ',');
					for(auto &operand_str : operand_split) {
						if(operand_str.find_first_not_of(' ') == std::string::npos) {
							break;
						}

						auto words = split(operand_str, ' ');

						if(words.empty()) {
							std::cerr << "error: syntax: ";
							std::cerr << '"' << operand_str << '"' << std::endl;
							return false;
						}

						if(words[0][0] == '!') {
							std::string command_str(words[0].substr(1));
							auto command_ty = mnemonic_to_command(command_str);
							if(!command_ty) {
								std::cerr << "error: " << command_str << ": invalid command" << std::endl;
								return false;
							}

							command cmd;
							cmd.ty = *command_ty;
							for(size_t w = 1; w < words.size(); ++w) {
								cmd.operands.emplace_back(parse_operand(words[w]));
							}
							instr.commands.emplace_back(cmd);
						} else {
							instr.operands.emplace_back(parse_operand(words[0]));
						}
					}
				}

				instrs.emplace_back(instr);
			}

			return true;
		}

		void encode_one(std::ostream &os, const instruction &instr) const {
			for(auto &cmd : instr.commands) {
				uint16_t byte = static_cast<uint16_t>(cmd.ty);
				byte |= 1u << 15u;
				std::cout << std::bitset<16>(byte) << ' ';
				os << HI8(byte) << LO8(byte);

				for(auto &operand : cmd.operands) {
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

		void encode(std::ostream &os) const {
			for(auto &instr : instrs) {
				encode_one(os, instr);
			}
		}
	};
} // namespace shk
