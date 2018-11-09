#include "gpio.h"

using namespace std;

GPIO::GPIO(sc_core::sc_module_name, unsigned int_gpio_base) : int_gpio_base(int_gpio_base)
	{
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

	SC_METHOD(fireInterrupt);	//this does not work?
	sensitive << asyncEvent;
	dont_initialize();		//dont know why, copied from example...

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
	if(r.vptr == &value)
	{
		if(r.write)
		{
			cerr << "[GPIO] write to value register is ignored!" << endl;
			return;
		}
		if(r.read)
		{

		}
	}
	r.fn();
	if(r.write)
	{
		if(r.vptr == &port)
		{
			cout << "[GPIO] new Port value: ";
			bitPrint(reinterpret_cast<unsigned char*>(&port), sizeof(uint32_t));

			//value and server.state might differ, if a bit is changed by client
			//and the interrupt was not fired yet.
			value = (value & ~output_en) | (port & output_en);
			server.state = (server.state & ~output_en) | (port & output_en);
		}
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

	asyncEvent.notify();
}

void GPIO::fireInterrupt() {
   cout << "[GPIO] Interrupts to fire!" << endl;

   GpioCommon::Reg serverSnapshot = server.state;
   uint32_t diff = (serverSnapshot ^ value) & input_en;

   bitPrint(reinterpret_cast<unsigned char*>(&value), 4);
   bitPrint(reinterpret_cast<unsigned char*>(&serverSnapshot), 4);
   bitPrint(reinterpret_cast<unsigned char*>(&input_en), 4);
   bitPrint(reinterpret_cast<unsigned char*>(&diff), 4);

   if(diff == 0)
   {
	   cout << "server and value do not differ." << endl;
	   return;
   }
   cout << "server and value differ." << endl;
   for(uint8_t i = 0; i < 32; i++)
   {
	   if(diff & (1l << i))
	   {
		   cout << "bit " << (unsigned) i << " changed ";
		   if(serverSnapshot & (1l << i))
		   {
			   cout << "to 1 ";
			   if(rise_intr_en & (1l << i))
			   {
				   cout << "and interrupt is enabled ";
				   if(rise_intr_pending & (1l << i))
				   {
					   cout << "but not yet consumed" << endl;
				   }
				   else
				   {
					   //todo: fire interrupt
					   cout << "and is being fired at " << int_gpio_base + i << endl;
				   }
			   }
			   else
			   {
				   cout << "but no interrupt is registered." << endl;
			   }
		   }
		   else
		   {
			   cout << "to 0 " << endl;
			   if(fall_intr_en & (1l << i))
			   {
				   cout << "and interrupt is enabled ";
				   if(fall_intr_pending & (1l << i))
				   {
					   cout << "but not yet consumed" << endl;
				   }
				   else
				   {
					   //todo: fire interrupt
					   cout << "and is being fired at " << int_gpio_base + i << endl;
				   }
			   }
			   else
			   {
				   cout << "but no interrupt is registered." << endl;
			   }
		   }
	   }
   }
   value = (serverSnapshot & input_en) | (port & output_en);
}
