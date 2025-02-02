#include <STDInclude.hpp>
#include "ConnectProtocol.hpp"
#include "Console.hpp"

#include <version.hpp>

namespace Components
{
	bool Singleton::FirstInstance = true;

	bool Singleton::IsFirstInstance()
	{
		return FirstInstance;
	}

	Singleton::Singleton()
	{
		if (Flags::HasFlag("version"))
		{
			printf("%s", "IW4x " VERSION " (built " __DATE__ " " __TIME__ ")\n");
			printf("%d\n", REVISION);
			std::exit(0);
		}

		Console::FreeNativeConsole();

		if (Loader::IsPerformingUnitTests() || Dedicated::IsEnabled() || ZoneBuilder::IsEnabled()) return;

		FirstInstance = (CreateMutexA(nullptr, FALSE, "iw4x_mutex") && GetLastError() != ERROR_ALREADY_EXISTS);

		if (!FirstInstance && !ConnectProtocol::Used() && MessageBoxA(nullptr, "Do you want to start another instance?\nNot all features will be available!", "Game already running", MB_ICONEXCLAMATION | MB_YESNO) == IDNO)
		{
			std::exit(0);
		}
	}
}
