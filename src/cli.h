#ifndef CLI_LINESZ
#define CLI_LINESZ 256
#endif

[[nodiscard("false is returned in case of error")]]
bool cli_readline(char[static CLI_LINESZ]);

bool cli_println(const char *, ...);


