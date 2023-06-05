#ifndef USHELL_TERMINAL_USHELLCONSOLEMOCK_HPP
#define USHELL_TERMINAL_USHELLCONSOLEMOCK_HPP

class UShellConsoleMock
{
      public:
	[[noreturn]] static void start(const std::string &path);
};

#endif // USHELL_TERMINAL_USHELLCONSOLEMOCK_HPP
