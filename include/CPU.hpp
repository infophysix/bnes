#pragma once

class Bus;

#include <cstdint>
#include <fstream>
#include <string>

class CPU
{
public:

	CPU(Bus&);
	~CPU();

	void step();

	////////////////////
	// Timing
	////////////////////

	uint8_t current_cycles {};
	uint8_t additional_cycles {};

	void tick(uint8_t cycles);

	////////////////////
	// Interrupts
	////////////////////

	enum class Interrupt
	{
		IRQ,
		RESET,
		NMI
	};

	void handleInterrupt(Interrupt);

	////////////////////
	// Data access
	////////////////////

	uint8_t read(uint16_t addr) const;
	void write(uint16_t addr, uint8_t data);

	////////////////////
	// Registers
	////////////////////

	uint16_t PC {}; // Program Counter
	uint8_t SP {};  // Stack Pointer
	uint8_t A {};   // Accumulator
	uint8_t X {};   // Index Register X
	uint8_t Y {};   // Index Register Y
	uint8_t P {};   // Processor Status

	////////////////////
	// Status Flags
	////////////////////

	enum class Flag
	{
		C = 1 << 0, // Carry
		Z = 1 << 1, // Zero
		I = 1 << 2, // Interrupt Disable
		D = 1 << 3, // Decimal Mode (unused)
		B = 1 << 4, // Break Command
		U = 1 << 5, // Unused
		V = 1 << 6, // Overflow
		N = 1 << 7  // Negative
	};

	bool getFlag(Flag) const;
	void setFlag(Flag, bool);

private:

	////////////////////
	// Bus
	////////////////////

	Bus *bus;

	////////////////////
	// Helpers
	////////////////////

	uint8_t fetchByte();
	uint16_t fetchWord();

	uint16_t buildAddress(uint8_t page, uint8_t offset) const;

	void checkPageCross(uint16_t addr, int8_t s_offset);

	uint8_t lowByte(uint16_t word);
	uint8_t highByte(uint16_t word);

	////////////////////
	// Stack
	////////////////////

	void stackPush(uint8_t data);
	uint8_t stackPop();

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

	uint16_t fetchOperandAddress(AddressingMode);
	uint8_t fetchOperand(AddressingMode);
	uint8_t newfetchOperand(AddressingMode);

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

	void branch(uint8_t offset, bool condition);
	bool checkOverflow(uint8_t, uint8_t, uint16_t);

public:

	////////////////////
	// Dispatch
	////////////////////

	struct Info
	{
		std::string mnemonic;
		Instruction instruction;
		AddressingMode addr_mode;
		uint8_t num_cycles;
	};

	Info decodeOpcode(uint8_t opcode);
	void executeInstruction(Instruction, AddressingMode, uint8_t cycles);
};