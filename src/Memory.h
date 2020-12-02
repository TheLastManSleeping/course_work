
#ifndef RISCV_SIM_DATAMEMORY_H
#define RISCV_SIM_DATAMEMORY_H

#include "Instruction.h"
#include <iostream>
#include <fstream>
#include <elf.h>
#include <cstring>
#include <vector>
#include <cassert>
#include <map>
#include <list>
#include <algorithm>


//static constexpr size_t memSize = 4*1024*1024; // memory size in 4-byte words
static constexpr size_t memSize = 1024 * 1024; // memory size in 4-byte words

static constexpr size_t lineSizeBytes = 64;
static constexpr size_t lineSizeWords = lineSizeBytes / sizeof(Word);
using Line = std::array<Word, lineSizeWords>;
static Word ToWordAddr(Word addr) { return addr >> 2u; }
static Word ToLineAddr(Word addr) { return addr & ~(lineSizeBytes - 1); }
static Word ToLineOffset(Word addr) { return ToWordAddr(addr) & (lineSizeWords - 1); }

class MemoryStorage {
public:

	MemoryStorage()
	{
		_mem.resize(memSize);
	}

	bool LoadElf(const std::string& elf_filename) {
		std::ifstream elffile;
		elffile.open(elf_filename, std::ios::in | std::ios::binary);

		if (!elffile.is_open()) {
			std::cerr << "ERROR: load_elf: failed opening file \"" << elf_filename << "\"" << std::endl;
			return false;
		}

		elffile.seekg(0, elffile.end);
		size_t buf_sz = elffile.tellg();
		elffile.seekg(0, elffile.beg);

		// Read the entire file. If it doesn't fit in host memory, it won't fit in the risc-v processor
		std::vector<char> buf(buf_sz);
		elffile.read(buf.data(), buf_sz);

		if (!elffile) {
			std::cerr << "ERROR: load_elf: failed reading elf header" << std::endl;
			return false;
		}

		if (buf_sz < sizeof(Elf32_Ehdr)) {
			std::cerr << "ERROR: load_elf: file too small to be a valid elf file" << std::endl;
			return false;
		}

		// make sure the header matches elf32 or elf64
		Elf32_Ehdr* ehdr = (Elf32_Ehdr*)buf.data();
		unsigned char* e_ident = ehdr->e_ident;
		if (e_ident[EI_MAG0] != ELFMAG0
			|| e_ident[EI_MAG1] != ELFMAG1
			|| e_ident[EI_MAG2] != ELFMAG2
			|| e_ident[EI_MAG3] != ELFMAG3) {
			std::cerr << "ERROR: load_elf: file is not an elf file" << std::endl;
			return false;
		}

		if (e_ident[EI_CLASS] == ELFCLASS32) {
			// 32-bit ELF
			return this->LoadElfSpecific<Elf32_Ehdr, Elf32_Phdr>(buf.data(), buf_sz);
		}
		else if (e_ident[EI_CLASS] == ELFCLASS64) {
			// 64-bit ELF
			return this->LoadElfSpecific<Elf64_Ehdr, Elf64_Phdr>(buf.data(), buf_sz);
		}
		else {
			std::cerr << "ERROR: load_elf: file is neither 32-bit nor 64-bit" << std::endl;
			return false;
		}
	}

	Word Read(Word ip)
	{
		return _mem[ToWordAddr(ip)];
	}

	void Write(Word ip, Word data)
	{
		_mem[ToWordAddr(ip)] = data;
	}

private:
	template <typename Elf_Ehdr, typename Elf_Phdr>
	bool LoadElfSpecific(char* buf, size_t buf_sz) {
		// 64-bit ELF
		Elf_Ehdr* ehdr = (Elf_Ehdr*)buf;
		Elf_Phdr* phdr = (Elf_Phdr*)(buf + ehdr->e_phoff);
		if (buf_sz < ehdr->e_phoff + ehdr->e_phnum * sizeof(Elf_Phdr)) {
			std::cerr << "ERROR: load_elf: file too small for expected number of program header tables" << std::endl;
			return false;
		}
		auto memptr = reinterpret_cast<char*>(_mem.data());
		// loop through program header tables
		for (int i = 0; i < ehdr->e_phnum; i++) {
			if ((phdr[i].p_type == PT_LOAD) && (phdr[i].p_memsz > 0)) {
				if (phdr[i].p_memsz < phdr[i].p_filesz) {
					std::cerr << "ERROR: load_elf: file size is larger than memory size" << std::endl;
					return false;
				}
				if (phdr[i].p_filesz > 0) {
					if (phdr[i].p_offset + phdr[i].p_filesz > buf_sz) {
						std::cerr << "ERROR: load_elf: file section overflow" << std::endl;
						return false;
					}

					// start of file section: buf + phdr[i].p_offset
					// end of file section: buf + phdr[i].p_offset + phdr[i].p_filesz
					// start of memory: phdr[i].p_paddr
					std::memcpy(memptr + phdr[i].p_paddr, buf + phdr[i].p_offset, phdr[i].p_filesz);
				}
				if (phdr[i].p_memsz > phdr[i].p_filesz) {
					// copy 0's to fill up remaining memory
					size_t zeros_sz = phdr[i].p_memsz - phdr[i].p_filesz;
					std::memset(memptr + phdr[i].p_paddr + phdr[i].p_filesz, 0, zeros_sz);
				}
			}
		}
		return true;
	}

	std::vector<Word> _mem;
};


class IMem
{
public:
	IMem() = default;
	virtual ~IMem() = default;
	IMem(const IMem&) = delete;
	IMem(IMem&&) = delete;

	IMem& operator=(const IMem&) = delete;
	IMem& operator=(IMem&&) = delete;

	virtual void Request(Word ip) = 0;
	virtual std::optional<Word> Response() = 0;
	virtual void Request(InstructionPtr& instr) = 0;
	virtual bool Response(InstructionPtr& instr) = 0;
	virtual void Clock() = 0;
};


class UncachedMem : public IMem
{
public:
	explicit UncachedMem(MemoryStorage& amem)
		: _mem(amem)
	{

	}

	void Request(Word ip)
	{
		_requestedIp = ip;
		_waitCycles = latency;
	}

	std::optional<Word> Response()
	{
		if (_waitCycles > 0)
			return std::optional<Word>();
		return _mem.Read(_requestedIp);
	}

	void Request(InstructionPtr& instr)
	{
		if (instr->_type != IType::Ld && instr->_type != IType::St)
			return;

		Request(instr->_addr);
	}

	bool Response(InstructionPtr& instr)
	{
		if (instr->_type != IType::Ld && instr->_type != IType::St)
			return true;

		if (_waitCycles != 0)
			return false;

		if (instr->_type == IType::Ld)
			instr->_data = _mem.Read(instr->_addr);
		else if (instr->_type == IType::St)
			_mem.Write(instr->_addr, instr->_data);

		return true;
	}

	void Clock()
	{
		if (_waitCycles > 0)
			--_waitCycles;
	}
private:
	static constexpr size_t latency = 120;
	Word _requestedIp = 0;
	size_t _waitCycles = 0;
	MemoryStorage& _mem;
};

class CachedMem : public IMem
{
public:

	explicit CachedMem(MemoryStorage& amem)
		: _mem(amem)
	{

	}

	void Request(Word ip)
	{
		_requestedIp = ip;
		Word tag = ToLineAddr(_requestedIp);
		if (std::find(_code_cache.last_used.begin(), _code_cache.last_used.end(), tag) != _code_cache.last_used.end())
		{
			_code_cache.last_used.remove(tag);

		}

		else
		{
			int i = 0;
			while (i < lineSizeWords) {
				Word word = _mem.Read(tag + i * 4);
				line[ToLineOffset(tag + i * 4)] = word;
				i++;
			}
			_code_cache.tables[tag] = line;

			if (_code_cache.tables.size() == _code_lines)
			{
				Word erase_tag = _code_cache.last_used.back();
				_code_cache.last_used.remove(erase_tag);
				_code_cache.tables.erase(erase_tag);
			}
			_waitCycles = latency;
		}
		_code_cache.last_used.push_front(tag);
	}

	std::optional<Word> Response()
	{
		if (_waitCycles > 0)
			return std::optional<Word>();
		return _code_cache.tables[ToLineAddr(_requestedIp)][ToLineOffset(_requestedIp)];
	}

	void Request(InstructionPtr& instr)
	{
		if (instr->_type != IType::Ld && instr->_type != IType::St)
		{
			return;
		}
		else {
			_requestedIp = instr->_addr;
			Word tag = ToLineAddr(_requestedIp);
			if (std::find(_data_cache.last_used.begin(), _data_cache.last_used.end(), tag) != _data_cache.last_used.end())
			{
				_data_cache.last_used.remove(tag);
				_waitCycles += 3;

			}

			else
			{
				int i = 0;
				while (i < lineSizeWords) {
					Word word = _mem.Read(tag + i * 4);
					line[ToLineOffset(tag + i * 4)] = word;
					i++;
				}
				_data_cache.tables[tag] = line;

				if (_data_cache.tables.size() == _data_lines)
				{
					Word erase_tag = _data_cache.last_used.back();
					_data_cache.last_used.remove(erase_tag);
					int i = 0;
					while (i < lineSizeWords) {
						_mem.Write(erase_tag + i * 4, _data_cache.tables[erase_tag][ToLineOffset(erase_tag + i * 4)]);
						i++;
					}
					_data_cache.tables.erase(erase_tag);
				}
				_waitCycles = latency;
			}
			_data_cache.last_used.push_front(tag);
		}
		return;
	}

	bool Response(InstructionPtr& instr)
	{
		if (instr->_type != IType::Ld && instr->_type != IType::St)
			return true;

		if (_waitCycles != 0)
			return false;

		if (instr->_type == IType::Ld)
			instr->_data = _data_cache.tables[ToLineAddr(instr->_addr)][ToLineOffset(instr->_addr)];
		else
		{
			_data_cache.tables[ToLineAddr(instr->_addr)][ToLineOffset(instr->_addr)] = instr->_data;
			_mem.Write(instr->_addr, instr->_data);
		}
		return true;
	}


	void Clock()
	{
		if (_waitCycles > 0)
			--_waitCycles = 0;
	}
private:
	static constexpr size_t latency = 136;
	Word _requestedIp = 0;
	size_t _waitCycles = 0;
	MemoryStorage& _mem;
	const int _data_lines = 64;
	const int _code_lines = 8;
	std::map<Word, Word> line;

	struct Cache
	{
		std::list<Word> last_used;
		std::map<Word, std::map<Word, Word>> tables;
	};
	Cache _code_cache;
	Cache _data_cache;

};

#endif //RISCV_SIM_DATAMEMORY_H