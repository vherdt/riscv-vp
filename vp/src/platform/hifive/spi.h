#ifndef RISCV_VP_SPI_H
#define RISCV_VP_SPI_H

#include <systemc>

#include <tlm_utils/simple_target_socket.h>

#include "core/common/irq_if.h"
#include "util/tlm_map.h"

#include <functional>
#include <thread>
#include <queue>
#include <mutex>
#include <map>

template<typename T>

//Only safe if one producer and one consumer
class LockQueue
{
	std::queue<T> queue;
	mutable std::mutex mutex;
public:
	void push(T elem){
		mutex.lock();
		queue.push(elem);
		mutex.unlock();
	}

	bool empty(){
		return queue.empty();
	}

	size_t size(){
		return queue.size();
	}

	T pop(bool& valid){
		mutex.lock();
		valid = !empty();
		T elem = queue.front();
		queue.pop();
		mutex.unlock();
		return elem;
	}
};

struct SpiInterface
{
	LockQueue<uint8_t> rx;
	LockQueue<uint8_t> tx;

	bool enabled = false;
	std::function<void()> enableCallback = nullptr;
	std::thread callback;

	~SpiInterface()
	{
		disable();
		if (callback.joinable()){
			callback.join();
		}
	}
	void enable(){ enabled = true; if(enableCallback) { callback = std::thread(enableCallback); } }
	void disable(){ enabled = false;}
};

typedef uint32_t Pin;

struct SPI : public sc_core::sc_module {
	tlm_utils::simple_target_socket<SPI> tsock;

	std::map<Pin, SpiInterface*> targets;

	// memory mapped configuration registers
	uint32_t sckdiv = 0;
	uint32_t sckmode = 0;
	uint32_t csid = 0;
	uint32_t csdef = 1;
	uint32_t csmode = 0;
	uint32_t delay0 = 0;
	uint32_t delay1 = 0;
	uint32_t fmt = 0;
	uint32_t txdata = 0;
	uint32_t rxdata = 0;
	uint32_t txmark = 0;
	uint32_t rxmark = 0;
	uint32_t fctrl = 1;
	uint32_t ffmt = 0;
	uint32_t ie = 0;
	uint32_t ip = 0;

	enum {
		SCKDIV_REG_ADDR  = 0x00,
		SCKMODE_REG_ADDR = 0x04,
		CSID_REG_ADDR    = 0x10,
		CSDEF_REG_ADDR   = 0x14,
		CSMODE_REG_ADDR  = 0x18,
		DELAY0_REG_ADDR  = 0x28,
		DELAY1_REG_ADDR  = 0x2C,
		FMT_REG_ADDR     = 0x40,
		TXDATA_REG_ADDR  = 0x48,
		RXDATA_REG_ADDR  = 0x4C,
		TXMARK_REG_ADDR  = 0x50,
		RXMARK_REG_ADDR  = 0x54,
		FCTRL_REG_ADDR   = 0x60,
		FFMT_REG_ADDR    = 0x64,
		IE_REG_ADDR      = 0x70,
		EP_REG_ADDR      = 0x74,
	};

	vp::map::LocalRouter router = {"SPI"};

	SPI(sc_core::sc_module_name) {
		tsock.register_b_transport(this, &SPI::transport);

		router
		    .add_register_bank({
		        {SCKDIV_REG_ADDR , &sckdiv},
				{SCKMODE_REG_ADDR, &sckmode},
				{CSID_REG_ADDR   , &csid},
				{CSDEF_REG_ADDR  , &csdef},
				{CSMODE_REG_ADDR , &csmode},
				{DELAY0_REG_ADDR , &delay0},
				{DELAY1_REG_ADDR , &delay1},
				{FMT_REG_ADDR    , &fmt},
				{TXDATA_REG_ADDR , &txdata},
				{RXDATA_REG_ADDR , &rxdata},
				{TXMARK_REG_ADDR , &txmark},
				{RXMARK_REG_ADDR , &rxmark},
				{FCTRL_REG_ADDR  , &fctrl},
				{FFMT_REG_ADDR   , &ffmt},
				{IE_REG_ADDR     , &ie},
				{EP_REG_ADDR     , &ip},
		    })
		    .register_handler(this, &SPI::register_access_callback);
	}

	void register_access_callback(const vp::map::register_access_t &r) {
		if(r.read)
		{
			if(r.vptr == &rxdata){
				bool isDataValid = false;
				auto target = targets.find(csid);
				if(target != targets.end()){
					rxdata = target->second->rx.pop(isDataValid);
				} else {
					std::cerr << "Read on unregistered Chip-Select " << csid << std::endl;
				}
				if(!isDataValid)
				{
					rxdata = 1 << 31;
				}
				std::cout << "read on rxdata " << std::hex << rxdata << std::endl;
			}
		}

		if(r.write)
		{
			if(r.vptr == &csid){
				auto target = targets.find(csid);
				if(target != targets.end()){
					target->second->disable();
				}
			}
		}

		r.fn();

		if(r.write)
		{
			if(r.vptr == &csid){
				std::cout << "Chip select " << csid << std::endl;
				auto target = targets.find(csid);
				if(target != targets.end()){
					target->second->enable();
				}
			}else if(r.vptr == &txdata){
				std::cout << std::hex << txdata << " ";
				auto target = targets.find(csid);
				if(target != targets.end()){
					target->second->tx.push(txdata);
				} else {
					std::cerr << "Write on unregistered Chip-Select " << csid << std::endl;
				}
				txdata = 0;
			}
		}
	}

	void transport(tlm::tlm_generic_payload &trans, sc_core::sc_time &delay) {
		router.transport(trans, delay);
	}

	void connect(Pin cs, SpiInterface& interface)
	{
		targets.insert(std::pair<const Pin,SpiInterface*>(cs, &interface));
		if(cs == csid)
			interface.enable();
	}
};

#endif  // RISCV_VP_SPI_H
