#ifndef RISCV_SIM_CPU_H
#define RISCV_SIM_CPU_H

#include "Memory.h"
#include "Decoder.h"
#include "RegisterFile.h"
#include "CsrFile.h"
#include "Executor.h"

class Cpu
{
public:
	Cpu(IMem& mem)
		: _mem(mem)
	{
		phase = 0;
	}

	void Clock()
	{
		_csrf.Clock();
		switch (phase)
		{
		case 0:
			{
				_mem.Request(_ip);
				phase++;
			}
		case 1:
			{
				std::optional<Word> resp = _mem.Response();
				if (!resp.has_value())
				{
					return;
				}
				Word instr = resp.value();
				instrDec = _decoder.Decode(instr);
				_rf.Read(instrDec);
				_csrf.Read(instrDec);
				_exe.Execute(instrDec, _ip);
				_mem.Request(instrDec->_addr, instrDec->_type);
				phase++;
			}
		case 2:
			{
				if (!_mem.Response(instrDec->_addr, instrDec->_type, instrDec->_data))
				{
					return;
				}
				_rf.Write(instrDec);
				_csrf.Write(instrDec);
				_csrf.InstructionExecuted();
				_ip = instrDec->_nextIp;
				phase = 0;
			}
		}
	}

	void Reset(Word ip)
	{
		_csrf.Reset();
		_ip = ip;
	}

	std::optional<CpuToHostData> GetMessage()
	{
		return _csrf.GetMessage();
	}



private:
	Reg32 _ip;
	Decoder _decoder;
	RegisterFile _rf;
	CsrFile _csrf;
	Executor _exe;
	IMem& _mem;
	// Add your code here, if needed
	int phase;
	InstructionPtr instrDec;

};


#endif //RISCV_SIM_CPU_H
