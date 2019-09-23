/*
 * oled.hpp
 *
 *  Created on: 20 Sep 2019
 *      Author: dwd
 *
 * This class models the SH1106 oled Display driver.
 */

#pragma once
#include "spi.h"
#include "util/elegantEnums.hpp"

#include "inttypes.h"
#include <map>
#include <functional>

namespace ss1106
{

//This is sorted by less strict match_mask
DECLARE_ENUM(Operator,
		COL_LOW,
		COL_HIGH,
		PUMP_VOLTAGE,
		DISPLAY_START_LINE,
		CONTRAST_MODE_SET,		//Double Command
		SEGMENT_REMAP,
		ENTIRE_DISPLAY,
		NORMAL_INVERSE,
		MULTIPLEX_RATIO,		//Double command
		DC_DC_VOLTAGE,
		DISPLAY_ON,
		PAGE_ADDR,
		COMMON_OUTPUT_DIR,
		DISPLAY_OFFSET,			//Double command
		DISPLAY_DIVIDE_RATIO,	//Double command
		DIS_PRE_CHARGE_PERIOD,	//Double command
		COMMON_PADS_STUFF,		//DC
		VCOM_DESELECT,			//DC
		RMW,
		RMW_END,
		NOP
	);

extern std::map<Operator, uint8_t> opcode;

struct Command
{
	Operator op;
	uint8_t payload;
};

uint8_t mask(Operator op);


};

class SS1106 : public SpiInterface  {
public:
	static constexpr uint8_t width  = 132;
	static constexpr uint8_t height = 64;
	static_assert(height%8 == 0);

private:
	enum class Mode : uint_fast8_t
	{
		normal,
		second_arg
	} mode = Mode::normal;

	struct State
	{
		uint8_t changed:1;	//beh, this is for syncing

		uint8_t column;
		uint8_t page;
		uint8_t pump_voltage:2;
		uint8_t display_startline:6;
		uint8_t contrast;
		uint8_t segment_remap:1;
		uint8_t entire_disp_on:1;
		uint8_t invert_color:1;
		uint8_t multiplex_whatever:6;
		uint8_t display_on:1;
		uint8_t frame[(height / 8)][width];
	} state = {0};

	ss1106::Command last_cmd = ss1106::Command{ss1106::Operator::NOP, 0};

	ss1106::Command match(uint8_t cmd);

	std::function<bool()> getDCPin;

	void createSM();

public:
	SS1106(std::function<bool()> getDCPin);
	~SS1106();

	uint8_t write(uint8_t byte) override;
};
