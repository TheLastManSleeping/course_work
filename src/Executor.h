#ifndef RISCV_SIM_EXECUTOR_H
#define RISCV_SIM_EXECUTOR_H

#include "Instruction.h"
#include <functional>

class Executor
{
public:

	void Execute(InstructionPtr& instr, Word ip)
	{
		switch (instr->_type)
		{
		case IType::Alu: {
			Word res = AluProc(instr);
			instr->_data = res;
			instr->_nextIp = ip + 4;
			break;
		}
		case IType::Ld:
		{
			instr->_addr = AluProc(instr);
			instr->_nextIp = ip + 4;
			break;
		}
		case IType::St:
		{
			instr->_addr = AluProc(instr);
			instr->_data = instr->_src2Val;
			instr->_nextIp = ip + 4;
			break;
		}
		case IType::Csrw:
		{
			instr->_data = instr->_src1Val;
			instr->_nextIp = ip + 4;
			break;
		}
		case IType::Csrr:
		{
			instr->_data = instr->_csrVal;
			instr->_nextIp = ip + 4;
			break;
		}
		case IType::J:
		{
			instr->_data = ip + 4;
		}
		case IType::Br:
		{
			bool res = BranchProc(instr);
			if (res)
				instr->_nextIp = ip + *instr->_imm;
			else
				instr->_nextIp = ip + 4;
			break;
		}
		case IType::Jr:
		{
			instr->_data = ip + 4;

			bool res = BranchProc(instr);
			if (res)
				instr->_nextIp = *instr->_imm + instr->_src1Val;
			else
				instr->_nextIp = ip + 4;
			break;
		}
		case IType::Auipc:
		{
			instr->_data = ip + *instr->_imm;
			instr->_nextIp = ip + 4;
			break;
		}
		}
	}

	Executor()
	{
		// writing up alu functions
		_aluFuncs[AluFunc::Add] = [this](Word op1, Word op2) { return op1 + op2; };
		_aluFuncs[AluFunc::Sll] = [this](Word op1, Word op2) { return op1 << (op2 % 32); };
		_aluFuncs[AluFunc::Slt] = [this](Word op1, Word op2) { return (Word)((int)op1 < (int)op2); };
		_aluFuncs[AluFunc::Sltu] = [this](Word op1, Word op2) { return (Word)(op1 < op2); };
		_aluFuncs[AluFunc::Xor] = [this](Word op1, Word op2) { return op1 ^ op2; };
		_aluFuncs[AluFunc::And] = [this](Word op1, Word op2) { return op1 & op2; };
		_aluFuncs[AluFunc::Or] = [this](Word op1, Word op2) { return op1 | op2; };
		_aluFuncs[AluFunc::Sub] = [this](Word op1, Word op2) { return op1 - op2; };
		_aluFuncs[AluFunc::Sra] = [this](Word op1, Word op2) { return (Word)((int)op1 >> (op2 % 32)); };
		_aluFuncs[AluFunc::Srl] = [this](Word op1, Word op2) { return op1 >> (op2 % 32); };
		// writing up branch functions
		_brFuncs[BrFunc::Eq] = [this](Word op1, Word op2) { return (Word)(op1 == op2); };
		_brFuncs[BrFunc::Neq] = [this](Word op1, Word op2) { return (Word)(op1 != op2); };
		_brFuncs[BrFunc::Lt] = [this](Word op1, Word op2) { return (Word)((int)op1 < (int)op2); };
		_brFuncs[BrFunc::Ltu] = [this](Word op1, Word op2) { return (Word)(op1 < op2); };
		_brFuncs[BrFunc::Ge] = [this](Word op1, Word op2) { return (Word)((int)op1 >= (int)op2); };
		_brFuncs[BrFunc::Geu] = [this](Word op1, Word op2) { return (Word)(op1 >= op2); };
		_brFuncs[BrFunc::AT] = [this](Word op1, Word op2) { return 1; };
		_brFuncs[BrFunc::NT] = [this](Word op1, Word op2) { return 0; };
	}

private:
	std::map<AluFunc, std::function<Word(Word, Word)>> _aluFuncs;
	std::map<BrFunc, std::function<Word(Word, Word)>> _brFuncs;

	Word AluProc(InstructionPtr& instr)
	{
		Word operand_1, operand_2;
		if (instr->_src1.has_value() && (instr->_imm.has_value() || instr->_src2.has_value()))
		{
			operand_1 = instr->_src1Val;
			operand_2 = instr->_imm.value_or(instr->_src2Val);
			return _aluFuncs[instr->_aluFunc](operand_1, operand_2);
		}
		return Word();
	}

	bool BranchProc(InstructionPtr& instr)
	{
		Word operand_1, operand_2;
		if (instr->_src1) 
		{
			operand_1 = instr->_src1Val;
		}
		if (instr->_src2)
		{
			operand_2 = instr->_src2Val;
		}
		return _brFuncs[instr->_brFunc](operand_1, operand_2);
	}
};

#endif // RISCV_SIM_EXECUTOR_H