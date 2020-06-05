#pragma once

namespace shk {
	enum class opcode : uint8_t {
		noop    = 0b0000,
		halt    = 0b0010,
		die     = 0b0011,

		load    = 0b0100,
		store   = 0b0101,

		move    = 0b1000,
		add     = 0b1010,
		compare = 0b1011,
	};

	struct operand {
		enum class type : uint8_t {
			imm = 0b0,
			reg = 0b1,

			mem,
		};

		type ty;
		uint16_t value;
	};

	struct command {
		enum class type : uint8_t {
			eq = 0b0000,
		};

		type ty;
		std::vector<operand> operands;
	};

	struct instruction {
		opcode op;
		std::vector<operand> operands;
		std::vector<command> commands;
	};
} // namespace shk
