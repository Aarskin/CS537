char* ParseWord(char** cmd, int* pos, int cmd_size, bool* last);
int CheckOverwriteRedirect(char** args);
int CheckAppendRedirect(char** args);
int CheckPipe(char** args);
void ReadCommand(char** args, char* cmd, size_t cmd_size);
