#pragma once

#include <bitset>
#include <iostream>
#include <unordered_map>
#include <vector>

#include <shk/asm/util.h>

namespace shk {
	class assembler {
	  private:
		std::vector<instruction> instrs;
		std::unordered_map<std::string, size_t> labels;
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

				for(auto it = label_split.begin(); it != label_split.end() - 1; ++it) {
					std::cout << "label " << trim(*it) << " at " << instrs.size() << std::endl;
					labels.emplace(trim(*it), instrs.size());
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

		bool resolve_operand(const std::vector<uint16_t> &addrs, operand &oper) {
			if(oper.ty == operand::type::label) {
				auto it = labels.find(oper.label);
				if(it == labels.end()) {
					std::cerr << "error: " << oper.label << ": unknown label" << std::endl;
					return false;
				}

				oper.ty = operand::type::imm;
				oper.value = addrs[it->second];
			}

			return true;
		}

		bool resolve() {
			std::vector<uint16_t> addrs;

			uint16_t addr = 0;
			for(auto &instr : instrs) {
				addrs.emplace_back(addr);

				++addr;
				addr += instr.operands.size();
				for(auto &cmd : instr.commands) {
					++addr;
					addr += cmd.operands.size();
				}
			}

			for(size_t i = 0; i < instrs.size(); ++i) {
				for(auto &oper : instrs[i].operands) {
					if(!resolve_operand(addrs, oper)) {
						return false;
					}
				}
				for(auto &cmd : instrs[i].commands) {
					for(auto &oper : cmd.operands) {
						if(!resolve_operand(addrs, oper)) {
							return false;
						}
					}
				}
			}

			return true;
		}

		bool encode_operand(std::ostream &os, const operand &oper) const {
			if(oper.ty == operand::type::label) {
				std::cerr << "error: unresolved label " << oper.label << std::endl;
				return false;
			}

			uint16_t byte = oper.value;
			byte |= static_cast<uint16_t>(oper.ty) << 14u;

			std::cout << ' ' << std::bitset<16>(byte);
			os << HI8(byte) << LO8(byte);

			return true;
		}

		bool encode_one(std::ostream &os, const instruction &instr) const {
			for(auto &cmd : instr.commands) {
				uint16_t byte = static_cast<uint16_t>(cmd.ty);
				byte |= 1u << 15u;
				std::cout << std::bitset<16>(byte);
				os << HI8(byte) << LO8(byte);

				for(auto &oper : cmd.operands) {
					if(!encode_operand(os, oper)) {
						return false;
					}
				}

				std::cout << ' ';
			}

			uint16_t byte = static_cast<uint16_t>(instr.op);
			std::cout << std::bitset<16>(byte);
			os << HI8(byte) << LO8(byte);

			for(auto &oper : instr.operands) {
				if(!encode_operand(os, oper)) {
					return false;
				}
			}
			std::cout << std::endl;

			return true;
		}

		bool encode(std::ostream &os) const {
			for(auto &instr : instrs) {
				if(!encode_one(os, instr)) {
					return false;
				}
			}
			return true;
		}
	};
} // namespace shk
