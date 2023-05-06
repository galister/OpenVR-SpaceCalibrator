#pragma once

#include "Logging.h"
#include <map>
#include <string>
#include <sys/mman.h>
#include <stdexcept>
#include <vector>
#include <unistd.h>
#include <cstdint>

class IHook
{
public:
	const std::string name;

	IHook(const std::string &name) : name(name) { }
	virtual ~IHook() { }

	virtual void Destroy() = 0;

	static bool Exists(const std::string &name);
	static void Register(IHook *hook);
	static void Unregister(IHook *hook);
	static void DestroyAll();

private:
	static std::map<std::string, IHook *> hooks;
};

template <typename FuncType>
class Hook : public IHook
{
	void * obj = nullptr;
	int offset = 0;

public:
	FuncType originalFunc = nullptr;
	Hook(const std::string &name) : IHook(name) { }
  
  template<typename T>
	bool CreateHookInObjectVTable(void *object, int vtableOffset, T* detourFunction)
	{
    		long pageSize = sysconf(_SC_PAGESIZE);

		obj = object;
		offset = vtableOffset;
   		// For virtual objects, VC++ (and from what I can tell gcc)  adds a pointer to the vtable as the first member.
		// To access the vtable, we simply dereference the object.
		void **vtable = *((void ***)object);

		// The vtable itself is an array of pointers to member functions,
		// in the order they were declared in.
		originalFunc = (FuncType) vtable[vtableOffset];
		targetFunc = (void*) vtable[vtableOffset];

		if((uintptr_t) vtable % 8  != 0 )
		{
			obj = nullptr;
			originalFunc = nullptr;
			throw std::runtime_error("vtable entry not aligned to 8 byte pointer");
		}

		uintptr_t otherPage = (uintptr_t) vtable & ~(uintptr_t) (pageSize - 1);
		int err = mprotect((void*) otherPage, pageSize, PROT_READ | PROT_WRITE | PROT_EXEC);
		if(err){
			LOG("Failed to set memory protection %d", err);
		}
		else {
			//LOG("%s", "Setting vtable value");
			vtable[vtableOffset] = (void*) detourFunction;
			//LOG("%s", "Resetting permissions vtable value");
			mprotect((void*) otherPage, pageSize, PROT_READ | PROT_EXEC);
		}

		LOG("Enabled Linux hook for %s", name.c_str());
		enabled = true;
		return true;
	}

	void Destroy()
	{
    
		//redoing it is enough if it was done the first time.
		if(obj && originalFunc) CreateHookInObjectVTable(obj, offset, originalFunc);
		obj = nullptr;
		originalFunc = nullptr;
	}

private:
	bool enabled = false;
	void* targetFunc = nullptr;
};
