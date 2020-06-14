#pragma once

#include <bitset>
#include <iostream>
#include <unordered_map>
#include <vector>

#include <shk.h>
#include <shk/util.h>

namespace shk {
	class assembler {
	  private:
		std::vector<instruction> instrs;
		std::unordered_map<std::string, size_t> labels;
	  public:
		bool verbose;

		assembler(bool verbose = false) : verbose(verbose) {}

		std::optional<instruction> process_one(std::string_view line) {
			auto comment_split = split(line, ';', 1);

			if(comment_split.empty()) {
				return {};
			}

			auto mnemonic_split = split(comment_split[0], ' ', 1);

			if(mnemonic_split.empty()) {
				return {};
			}

			if(mnemonic_split[0].back() == ':') {
				auto label = mnemonic_split[0];
				label.remove_suffix(1);

				if(verbose) {
					std::cout << "label " << label << " at " << instrs.size() << std::endl;
				}
				labels.emplace(label, instrs.size());

				return process_one(mnemonic_split[1]);
			}

			auto opcode_str = mnemonic_split[0];

			instruction instr;
			instr.op = mnemonic_to_opcode(opcode_str);

			if(mnemonic_split.size() >= 2) {
				auto operand_split = split(mnemonic_split[1], ',');
				for(auto &operand_str : operand_split) {
					if(operand_str.find_first_not_of(' ') == std::string::npos) {
						break;
					}

					auto words = split(operand_str, ' ');

					if(words.empty()) {
						throw std::runtime_error("syntax error: " + std::string(operand_str));
					}

					if(words[0][0] == '!') {
						std::string command_str(words[0].substr(1));

						command cmd;
						cmd.ty = mnemonic_to_command(command_str);

						for(size_t w = 1; w < words.size(); ++w) {
							cmd.operands.emplace_back(parse_operand(words[w]));
						}
						instr.commands.emplace_back(std::move(cmd));
					} else {
						instr.operands.emplace_back(parse_operand(words[0]));
					}
				}
			}

			return instr;
		}

		void process(std::istream &is) {
			std::string line;
			while(std::getline(is, line)) {
				if(auto instr = process_one(line)) {
					instrs.emplace_back(std::move(*instr));
				}
			}
		}

		void resolve_operand(const std::vector<uint16_t> &addrs, operand &oper) {
			if(oper.ty == operand::type::label) {
				auto it = labels.find(oper.label);
				if(it == labels.end()) {
					throw std::runtime_error("unknown label: " + oper.label);
				}

				oper.ty = operand::type::imm;
				oper.value = addrs[it->second];
			}
		}

		void resolve() {
			std::vector<uint16_t> addrs;

			uint16_t addr = 0;
			for(auto &instr : instrs) {
				addrs.emplace_back(addr);
				addr += instr.size();
			}

			for(size_t i = 0; i < instrs.size(); ++i) {
				for(auto &oper : instrs[i].operands) {
					resolve_operand(addrs, oper);
				}

				for(auto &cmd : instrs[i].commands) {
					for(auto &oper : cmd.operands) {
						resolve_operand(addrs, oper);
					}
				}
			}
		}

		void encode_operand(std::ostream &os, const operand &oper, bool segment = false) const {
			if(oper.ty == operand::type::label) {
				throw std::runtime_error("unresolved label: " + oper.label);
			}

			if(oper.segment) {
				encode_operand(os, *oper.segment, true);
			}

			uint16_t byte = oper.value;
			byte |= static_cast<uint16_t>(oper.ty) << 12u;
			if(segment) {
				byte |= 1 << 15u;
			}

			if(verbose) {
				std::cout << ' ' << std::bitset<16>(byte);
			}
			os << HI8(byte) << LO8(byte);
		}

		void encode_one(std::ostream &os, const instruction &instr) const {
			for(auto &cmd : instr.commands) {
				uint16_t byte = static_cast<uint16_t>(cmd.ty);
				byte |= 1u << 15u;
				if(verbose) {
					std::cout << std::bitset<16>(byte);
				}
				os << HI8(byte) << LO8(byte);

				for(auto &oper : cmd.operands) {
					encode_operand(os, oper);
				}

				if(verbose) {
					std::cout << ' ';
				}
			}

			if(instr.op != opcode::data) {
				uint16_t byte = static_cast<uint16_t>(instr.op);
				if(verbose) {
					std::cout << std::bitset<16>(byte);
				}
				os << HI8(byte) << LO8(byte);
			}

			for(auto &oper : instr.operands) {
				encode_operand(os, oper);
			}

			if(verbose) {
				std::cout << std::endl;
			}
		}

		void encode(std::ostream &os) const {
			for(auto &instr : instrs) {
				encode_one(os, instr);
			}
		}
	};
} // namespace shk
