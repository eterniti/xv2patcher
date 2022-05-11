#pragma once

#define LOOKUP_SIZE	0x1000

extern "C"
{

extern uint8_t auto_btl_portrait_lookup[LOOKUP_SIZE];	
extern bool test_144;

const std::string &GetSlotsData();	
	
}