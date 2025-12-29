#ifndef COMPLETION_H
#define COMPLETION_H

char* command_generator(const char* text, int state);
char** command_completion(const char* text, int start, int end);

#endif // COMPLETION_H
