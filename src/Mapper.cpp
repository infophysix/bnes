#include "Mapper.hpp"

#include "Cartridge.hpp"

Mapper::Mapper(Cartridge *cart_ref)
	: cartridge { cart_ref }
{
}

Mapper::~Mapper()
{
}