#pragma once

#include "Bus.hpp"
#include "CPU.hpp"
#include "PPU.hpp"

#include <cstdint>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <string>

class Logger
{
public:

	Logger(const std::string& log_file, Bus& _bus, CPU& _cpu, PPU& _ppu);
	~Logger();

	void logLine();

private:

	std::ofstream ofs;
	Bus *bus;
	CPU *cpu;
	PPU *ppu;

	uint8_t cpuRead(uint16_t addr);

	////////////////////
	// Addressing Modes
	////////////////////

	enum class AddressingMode
	{
		Absolute,    // ... $LLHH
		AbsoluteX,   // ... $LLHH,X
		AbsoluteY,   // ... $LLHH,Y
		Accumulator, // ... A
		Immediate,   // ... #$BB
		Implied,     // ...
		Indirect,    // ... ($LLHH)
		IndirectX,   // ... ($LL,X)
		IndirectY,   // ... ($LL),Y
		Relative,    // ... $BB
		ZeroPage,    // ... $LL
		ZeroPageX,   // ... $LL,X
		ZeroPageY    // ... $LL,Y
	};

	////////////////////
	// Instructions (official)
	////////////////////

	enum class Instruction
	{
		ADC, // add with carry
		AND, // AND (with A)
		ASL, // arithmetic shift left
		BCC, // branch if C clear
		BCS, // branch if C set
		BEQ, // branch if Z set
		BIT, // test bits
		BMI, // branch if N set
		BNE, // branch if Z clear
		BPL, // branch if N clear
		BRK, // break
		BVC, // branch if V clear
		BVS, // branch if V set
		CLC, // unset C
		CLD, // unset D (unused)
		CLI, // unset I
		CLV, // unset V
		CMP, // compare (with A)
		CPX, // compare (with X)
		CPY, // compare (with Y)
		DEC, // decrement
		DEX, // decrement X
		DEY, // decrement Y
		EOR, // XOR (with A)
		INC, // increment
		INX, // increment X
		INY, // increment Y
		JMP, // jump
		JSR, // jump to subroutine
		LDA, // load A
		LDX, // load X
		LDY, // load Y
		LSR, // logical shift right
		NOP, // no operation
		ORA, // OR (with A)
		PHA, // push A
		PHP, // push P
		PLA, // pull A
		PLP, // pull P
		ROL, // rotate left
		ROR, // rotate right
		RTI, // return from interrupt
		RTS, // return from subroutine
		SBC, // subtract with carry
		SEC, // set C
		SED, // set D (unused)
		SEI, // set I
		STA, // store accumulator
		STX, // store X
		STY, // store Y
		TAX, // transfer A to X
		TAY, // transfer A to Y
		TSX, // transfer SP to X
		TXA, // transfer X to A
		TXS, // transfer X to SP
		TYA, // transfer Y to A
	};

	struct Info
	{
		std::string mnemonic;
		Instruction instruction;
		AddressingMode addr_mode;
		uint8_t num_cycles;
	};

	////////////////////
	// Decode
	////////////////////

	Info decodeOpcode(uint8_t opcode) const;

	////////////////////
	// Logging Functions
	////////////////////

	void logPC(uint16_t PC);
	void logOpcode(uint8_t opcode);
	void logMnemonic(const std::string& mnemonic);
	void logOperands(const Info& info, uint16_t PC);
	void logAddressingMode(const Info& info, uint16_t PC);
	void logRegisters();
	void logCycles(const Info& info);
};