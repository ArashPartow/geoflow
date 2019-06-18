#pragma once

#include <iostream>
#include <dlfcn.h>
#include "../IDLLoader.h"

namespace dlloader
{
	template <class T>
	class DLLoader : public IDLLoader<T>
	{

	private:

		void			*_handle;
		std::string		_pathToLib;
		std::string		_allocClassSymbol;
		std::string		_deleteClassSymbol;

	public:

		DLLoader(std::string const &pathToLib,
			std::string const &allocClassSymbol = "allocator",
			std::string const &deleteClassSymbol = "deleter") :
			_handle(nullptr), _pathToLib(pathToLib),
			_allocClassSymbol(allocClassSymbol), _deleteClassSymbol(deleteClassSymbol)
		{
		}

		~DLLoader() = default;

		void DLOpenLib() override
		{
			if (!(_handle = dlopen(_pathToLib.c_str(), RTLD_GLOBAL | RTLD_LAZY))) {
				std::cerr << dlerror() << std::endl;
			}
		}

		std::shared_ptr<T> DLGetInstance() override
		{
			using allocClass = T *(*)();
			using deleteClass = void (*)(T *);


				auto allocFunc = reinterpret_cast<allocClass>(
					dlsym(_handle, _allocClassSymbol.c_str()));
				if(!allocFunc)
					std::cerr << dlerror() << std::endl;
				auto deleteFunc = reinterpret_cast<deleteClass>(
					dlsym(_handle, _deleteClassSymbol.c_str()));
				if(!deleteFunc)
					std::cerr << dlerror() << std::endl;

			if (!allocFunc || !deleteFunc) {
				DLCloseLib();
			}

			return std::shared_ptr<T>(
					allocFunc(),
					[deleteFunc](T *p){ deleteFunc(p); });
		}

		void DLSetImGuiContext(ImGuiContext* ctx) override {
			using SetCtxfunc = void (*)(ImGuiContext*);
			
			auto Func = reinterpret_cast<SetCtxfunc>(
				dlsym(_handle, "SetImGuiContext"));
			if (!Func) {
				std::cerr << dlerror() << std::endl;
				DLCloseLib();
			}
			Func(ctx);
		}

		void DLCloseLib() override
		{
			if (dlclose(_handle) != 0) {
				std::cerr << dlerror() << std::endl;
			}
		}

	};
}
