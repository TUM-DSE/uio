#ifndef USHELL_TERMINAL_MOCKUSHELLCONSOLE_H
#define USHELL_TERMINAL_MOCKUSHELLCONSOLE_H

class MockUShellConsole
{
      public:
	[[noreturn]] static void start(const std::string &path);
};

#endif // USHELL_TERMINAL_MOCKUSHELLCONSOLE_H
