//
// Created by ken on 22.05.23.
//

#ifndef USHELL_TERMINAL_USHELLCMDINTERCEPTOR_HPP
#define USHELL_TERMINAL_USHELLCMDINTERCEPTOR_HPP

#include <string>
#include <filesystem>

extern "C" {
#include <helper_function_list.h>
}

struct InterceptionResult {
	int code;
	bool handled;
};

class UShellCmdInterceptor
{
      public:
	UShellCmdInterceptor(std::string ushellRoot,
			     std::string ushellHostMountPoint,
			     const HelperFunctionList *helperFunctionList);

	InterceptionResult intercept(const std::string &in);

      private:
	const std::filesystem::path ushellRoot;
	const std::filesystem::path ushellHostMountPoint;
	const HelperFunctionList *helperFunctionList;
};

#endif // USHELL_TERMINAL_USHELLCMDINTERCEPTOR_HPP
