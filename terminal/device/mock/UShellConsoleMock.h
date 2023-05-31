#ifndef USHELL_TERMINAL_USHELLCONSOLEMOCK_H
#define USHELL_TERMINAL_USHELLCONSOLEMOCK_H

class UShellConsoleMock
{
      public:
	[[noreturn]] static void start(const std::string &path);
};

#endif // USHELL_TERMINAL_USHELLCONSOLEMOCK_H
