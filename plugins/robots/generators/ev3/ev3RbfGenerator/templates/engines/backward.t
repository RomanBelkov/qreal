﻿DATA32 @@RANDOM_ID@@
MOVE32_32(@@POWER@@, @@RANDOM_ID@@)
MUL32(@@RANDOM_ID@@, -1, @@RANDOM_ID@@)
OUTPUT_POWER(0, @@PORT@@, @@RANDOM_ID@@)
OUTPUT_START(0, @@PORT@@)
