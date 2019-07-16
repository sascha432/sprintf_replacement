
#pragma once

typedef int(*snprintf_func_t)(char *, size_t, const char *, ...);

void run_tests(snprintf_func_t func, bool extra = false);
