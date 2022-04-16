#pragma once

#include <array>
#include <cstdint>

class Bus;

constexpr size_t NAMETABLE_W { 32 };
constexpr size_t NAMETABLE_H { 30 };
constexpr size_t SCREEN_W { 256 };
constexpr size_t SCREEN_H { 240 };
constexpr size_t TILE_W { 8 };
constexpr size_t TILE_H { 8 };

class PPU
{
public:

	PPU(Bus&);
	~PPU();

	////////////////////
	// Data access
	////////////////////

	uint8_t read(uint16_t addr) const;
	void write(uint16_t addr, uint8_t data);

	uint8_t readRegister(uint16_t index, bool read_only);
	void writeRegister(uint16_t index, uint8_t data);

	////////////////////
	// Timing
	////////////////////

	size_t cycles {};
	size_t scanlines {};

	void step(uint8_t ppu_cycles);

	////////////////////
	// Palettes
	////////////////////

	// stores indexes
	std::array<uint8_t, 32> vram_palettes {};

	////////////////////
	// Nametables
	////////////////////

	using Nametable = std::array<uint8_t, 0x400>;

	Nametable nametable_0 {};
	Nametable nametable_1 {};
	Nametable nametable_2 {};
	Nametable nametable_3 {};

	const Nametable& getNametable() const;

	void getPalettes(
		size_t X,
		size_t Y,
		uint8_t& col_0,
		uint8_t& col_1,
		uint8_t& col_2,
		uint8_t& col_3
	);

	////////////////////
	// Frame
	////////////////////

	bool update_screen {};

	using Tile = std::array<uint8_t, 8 * 8>;

	uint32_t buffer[SCREEN_H][SCREEN_W];

	const Tile getTile(uint8_t id) const;
	void updateBuffer();

private:

	////////////////////
	// Bus
	////////////////////

	Bus *bus;

	////////////////////
	// Palette
	////////////////////

	struct RGB
	{
		uint8_t R;
		uint8_t G;
		uint8_t B;
	};

	std::array<uint32_t, 64> palettes {};

	////////////////////
	// Registers
	////////////////////

	union
	{
		struct
		{
			uint8_t base_nt_addr       : 2;
			uint8_t addr_vram_inc      : 1;
			uint8_t addr_pt_fg         : 1;
			uint8_t background_pt_addr : 1;
			uint8_t sprite_size        : 1;
			uint8_t master_slave       : 1;
			uint8_t generate_nmi       : 1;
		};

		uint8_t val;
	} PPUCTRL {};

	union
	{
		struct
		{
			uint8_t greyscale        : 1;
			uint8_t show_bg_leftmost : 1;
			uint8_t show_fg_leftmost : 1;
			uint8_t show_bg          : 1;
			uint8_t show_fg          : 1;
			uint8_t emphasize_red    : 1;
			uint8_t emphasize_green  : 1;
			uint8_t emphasize_blue   : 1;
		};

		uint8_t val;
	} PPUMASK {};

	union
	{
		struct
		{
			uint8_t previous_lsb    : 5;
			uint8_t sprite_overflow : 1;
			uint8_t sprite_0_hit    : 1;
			uint8_t vblank          : 1;
		};

		uint8_t val;
	} PPUSTATUS {};

	union LoopyAddress
	{
		struct
		{
			unsigned coarse_x : 5;
			uint8_t coarse_y  : 5;
			uint8_t nt_select : 2;
			uint8_t fine_y    : 3;
		};

		struct
		{
			unsigned l : 8;
			unsigned h : 7;
		};

		uint16_t val;
	};

	LoopyAddress temp_addr {};
	LoopyAddress vram_addr {};
	uint8_t fine_x_scroll {};
	uint8_t internal_buffer {};
	bool latch {};
};