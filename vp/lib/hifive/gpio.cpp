#include "gpio.h"

using namespace std;

GPIO::GPIO(sc_core::sc_module_name) {
	tsock.register_b_transport(this, &GPIO::transport);

	router.add_register_bank({
							{PIN_VALUE_ADDR, &value},
							{INPUT_EN_REG_ADDR, &input_en},
							{OUTPUT_EN_REG_ADDR, &output_en},
							{PORT_REG_ADDR, &port},
							{PULLUP_EN_ADDR, &pullup_en},
							{PIN_DRIVE_STNGTH, &pin_drive_strength},
							{RISE_INTR_EN, &rise_intr_en},
							{RISE_INTR_PEND, &rise_intr_pending},
							{FALL_INTR_EN, &fall_intr_en},
							{FALL_INTR_PEND, &fall_intr_pending},
							{HIGH_INTR_EN, &high_intr_en},
							{HIGH_INTR_PEND, &high_intr_pending},
							{LOW_INTR_EN, &low_intr_en},
							{LOW_INTR_PEND, &low_intr_pending},
							{IOF_EN_REG_ADDR, &iof_en},
							{IOF_SEL_REG_ADDR, &iof_sel},
							{OUT_XOR_REG_ADDR, &out_xor},
	 }).register_handler(this, &GPIO::register_access_callback);

	//SC_METHOD(fireInterrupt);
	server.setupConnection("1339");
	server.registerOnChange(bind(&GPIO::asyncOnchange, this, placeholders::_1, placeholders::_2));
	serverThread = thread(bind(&GpioServer::startListening, &server));
}

GPIO::~GPIO()
{
	server.quit();
	serverThread.join();
}

void GPIO::register_access_callback(const vp::map::register_access_t &r)
{
	if(r.write)
	{
		if(r.vptr == &value)
		{
			cerr << "[GPIO] write to value register is ignored!" << endl;
			return;
		}
	}
	r.fn();
	if(r.write)
	{
		//cout << "Write to GPIO reg. no " << (r.vptr - &pin_value);
		if(r.vptr == &port)
		{
			cout << "[GPIO] new Port value: ";
			for(unsigned i = 0; i < sizeof(uint32_t) * 8; i++)
			{
				if(i > 1 && (i % 8 == 0))
					cout << " ";
				printf("%c", port & 1 << (32 - i) ? '1' : '0');
			}

			//value and server.state might differ, if a bit is changed by client
			//and the interrupt was not fired yet.
			value = (value & ~output_en) | (port & output_en);
			server.state = (server.state & ~output_en) | (port & output_en);
		}
		cout << endl;
	}
}

void GPIO::transport(tlm::tlm_generic_payload &trans, sc_core::sc_time &delay) {
	router.transport(trans, delay);
}

void GPIO::asyncOnchange(uint8_t bit, GpioCommon::Tristate val)
{
	if(((server.state & (1l << bit)) >> bit) == val)
	{
		cout << "[GPIO] Bit " << (unsigned) bit << " still at " << (unsigned) val << endl;
		return;
	}
	if(val == 0)
	{
		server.state &= ~(1l << bit);
	}
	else if(val == 1)
	{
		server.state |= 1l << bit;
	}
	else
	{
		cout << "[GPIO] Ignoring tristate for now\n";
		return;
	}
	cout << "[GPIO] Bit " << (unsigned) bit << " changed to " << (unsigned) val << endl;
}

void GPIO::fireInterrupt() {
   cout << "[GPIO] Interrupts to fire!" << endl;
}
