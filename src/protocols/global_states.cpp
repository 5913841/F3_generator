#include "global_states.h"

global_states g_vars[MAX_PATTERNS];
__thread global_templates g_templates[MAX_PATTERNS];
int g_pattern_num = 0;