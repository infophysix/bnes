#include "PPU.hpp"

#include "Bus.hpp"

#include <iomanip>
#include <iostream>

PPU::PPU(Bus& bus_ref)
	: bus { &bus_ref }
	, palettes { 0x808080, 0x003DA6, 0x0012B0, 0x440096, 0xA1005E, 0xC70028,
	             0xBA0600, 0x8C1700, 0x5C2F00, 0x104500, 0x054A00, 0x00472E,
	             0x004166, 0x000000, 0x050505, 0x050505, 0xC7C7C7, 0x0077FF,
	             0x2155FF, 0x8237FA, 0xEB2FB5, 0xFF2950, 0xFF2200, 0xD63200,
	             0xC46200, 0x358000, 0x058F00, 0x008A55, 0x0099CC, 0x212121,
	             0x090909, 0x090909, 0xFFFFFF, 0x0FD7FF, 0x69A2FF, 0xD480FF,
	             0xFF45F3, 0xFF618B, 0xFF8833, 0xFF9C12, 0xFABC20, 0x9FE30E,
	             0x2BF035, 0x0CF0A4, 0x05FBFF, 0x5E5E5E, 0x0D0D0D, 0x0D0D0D,
	             0xFFFFFF, 0xA6FCFF, 0xB3ECFF, 0xDAABEB, 0xFFA8F9, 0xFFABB3,
	             0xFFD2B0, 0xFFEFA6, 0xFFF79C, 0xD7E895, 0xA6EDAF, 0xA2F2DA,
	             0x99FFFC, 0xDDDDDD, 0x111111, 0x111111 }
{
	for (size_t Y {}; Y < SCREEN_H; ++Y)
		for (size_t X {}; X < SCREEN_W; ++X)
			buffer[Y][X] = 0;
}

PPU::~PPU()
{
}

////////////////////
// Timing
////////////////////

void PPU::step(uint8_t ppu_cycles)
{
	cycles += ppu_cycles;
	bool first_cycle = false;

	// A scanline occurs every 341 PPU cycles
	if (cycles >= 341)
	{
		cycles -= 341;
		scanlines++;
		first_cycle = true;

		// NMI interrupt is triggered on scanline 241
		if (scanlines == 241 && first_cycle == true)
		{
			PPUSTATUS.vblank = 1; // signal start of vblank

			if (PPUCTRL.generate_nmi == 1)
			{
				bus->cpu->handleInterrupt(CPU::Interrupt::NMI);
			}
		}

		// PPU renders 262 scanlines per frame
		if (scanlines >= 262)
		{
			PPUSTATUS.vblank = 0; // end of vblank
			scanlines = 0;
			update_screen = true;
		}
	}
}

////////////////////
// Data Access
////////////////////

uint8_t PPU::read(uint16_t addr) const
{
	return bus->ppuRead(addr);
}

void PPU::write(uint16_t addr, uint8_t data)
{
	bus->ppuWrite(addr, data);
}

uint8_t PPU::readRegister(uint16_t index, bool read_only)
{
	uint8_t data {};

	if (read_only == true)
	{
		switch (index)
		{
		case 0:
			return PPUCTRL.val;
		case 1:
			return PPUMASK.val;
		case 2:
			return PPUSTATUS.val;
		default:
			return 0;
		}
	}

	switch (index)
	{
	// PPUSTATUS
	case 2:
		data = (PPUSTATUS.val & 0b11100000) | (internal_buffer & 0b00011111);
		PPUSTATUS.vblank = 0;
		latch = false;
		break;

	// PPUDATA
	case 7:
	{
		if (vram_addr.val <= 0x3EFF)
		{
			data = internal_buffer;
			internal_buffer = read(vram_addr.val);
		} else
		{
			internal_buffer = read(vram_addr.val);
			data = internal_buffer;
		}

		// Increment PPUADDR based on value in PPUCTRL
		if (PPUCTRL.addr_vram_inc == 0)
			vram_addr.val += 1;
		else
			vram_addr.val += 32;
	}
	break;

	default:
		return 0;
	}

	return data;
}

void PPU::writeRegister(uint16_t index, uint8_t data)
{
	switch (index)
	{
	// PPUCTRL
	case 0:
		PPUCTRL.val = data;
		temp_addr.nt_select = PPUCTRL.base_nt_addr;
		break;

	// PPUMASK
	case 1:
		PPUMASK.val = data;
		break;

	// PPUSCROLL
	case 5:
		if (latch == false)
		{
			temp_addr.coarse_x = data >> 3;
			fine_x_scroll = data & 0b00000111;
			latch = true;
		} else
		{
			temp_addr.coarse_y = data >> 3;
			temp_addr.fine_y = data & 0b00000111;
			latch = false;
		}
		break;

	// PPUADDR
	case 6:
		if (latch == false)
		{
			temp_addr.h = data & 0b00111111;
			latch = true;
		} else
		{
			temp_addr.l = data;
			vram_addr.val = temp_addr.val;
			latch = false;
		}
		break;

	// PPUDATA
	case 7:
		write(vram_addr.val, data);

		if (PPUCTRL.addr_vram_inc == 0)
			vram_addr.val += 1;
		else
			vram_addr.val += 32;
		break;
	}
}

const PPU::Tile PPU::getTile(uint8_t id) const
{
	uint16_t offset {};

	if (PPUCTRL.background_pt_addr == 1)
		offset = 0x1000;

	///////////////////////////////////////////////
	// Load bytes
	///////////////////////////////////////////////
	// 0-7: 18 38 18 18 18 18 7E 00 (low)
	// 8-F: 00 00 00 00 00 00 00 00 (high)
	///////////////////////////////////////////////

	std::array<uint8_t, 8> lo_bytes {};
	std::array<uint8_t, 8> hi_bytes {};

	for (size_t i {}; i < 8; ++i)
	{
		lo_bytes[i] = bus->ppuRead(offset + 16 * id + i);
		hi_bytes[i] = bus->ppuRead(offset + 16 * id + i + 8);
	}

	////////////////////
	// Build tile
	////////////////////

	Tile tile {};

	for (size_t Y {}; Y < TILE_H; ++Y)
	{
		for (size_t X {}; X < TILE_W; ++X)
		{
			const uint8_t pixel_lo = (lo_bytes[Y] >> (7 - X)) & 1;
			const uint8_t pixel_hi = (hi_bytes[Y] >> (7 - X)) & 1;
			const uint8_t pixel = pixel_lo | (pixel_hi << 1);

			tile[(8 * Y) + X] = pixel;
		}
	}

	return tile;
}

const PPU::Nametable& PPU::getNametable() const
{
	switch (PPUCTRL.base_nt_addr)
	{
	case 0:
		return nametable_0;
	case 1:
		return nametable_1;
	case 2:
		return nametable_2;
	case 3:
		return nametable_3;
	default:
		return nametable_0;
	}
}

void PPU::getPalettes(
	size_t X,
	size_t Y,
	uint8_t& col_0,
	uint8_t& col_1,
	uint8_t& col_2,
	uint8_t& col_3
)
{
	const Nametable& nametable = getNametable();

	const size_t offset = (8 * (Y / 4)) + (X / 4);

	uint8_t block = nametable[0x03C0 + offset];

	col_0 = vram_palettes[4 * (block & 0b00000011)];
	col_1 = vram_palettes[4 * ((block & 0b00110000) >> 4)];
	col_2 = vram_palettes[4 * ((block & 0b00001100) >> 2)];
	col_3 = vram_palettes[4 * ((block & 0b11000000) >> 6)];
}

void PPU::updateBuffer()
{
	const Nametable& nametable = getNametable();

	////////////////////
	// Clear buffer
	////////////////////

	for (size_t Y {}; Y < SCREEN_H; ++Y)
		for (size_t X {}; X < SCREEN_W; ++X)
			buffer[Y][X] = 0;

	////////////////////
	// Fill temp buffer
	////////////////////

	for (size_t Y {}; Y < NAMETABLE_H; ++Y)
	{
		for (size_t X {}; X < NAMETABLE_W; ++X)
		{
			uint8_t color_0 {};
			uint8_t color_1 {};
			uint8_t color_2 {};
			uint8_t color_3 {};

			getPalettes(X, Y, color_0, color_1, color_2, color_3);

			// Get tile
			const uint8_t tile_id = nametable[Y * 32 + X];
			const Tile tile = getTile(tile_id);

			for (size_t tile_Y {}; tile_Y < 8; ++tile_Y)
			{
				for (size_t tile_X {}; tile_X < 8; ++tile_X)
				{
					const uint8_t pixel = tile[8 * tile_Y + tile_X];

					switch (pixel)
					{
					case 0:
						buffer[Y * 8 + tile_Y][X * 8 + tile_X] = palettes[0x0f];
						break;
					case 1:
						buffer[Y * 8 + tile_Y][X * 8 + tile_X] = palettes[0x2c];
						break;
					case 2:
						buffer[Y * 8 + tile_Y][X * 8 + tile_X] = palettes[0x38];
						break;
					case 3:
						buffer[Y * 8 + tile_Y][X * 8 + tile_X] = palettes[0x12];
						break;
					default:
						buffer[Y * 8 + tile_Y][X * 8 + tile_X] = palettes[0x0f];
						break;
					}
				}
			}
		}
	}
}