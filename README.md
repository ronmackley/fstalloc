# FSTALLOC - C++ memory allocator.


This is a toy from early in my career to try to do better than the `g++ operator new()`.

It tries to maintain groups of pages of various power of 2 bytes in size, plus an overflow to hold allocations larger than the largest pool size. A call to `operator new()` finds the appropriate slot based on the binary root of the size of the chunk requested and returns an available slot based what is available.

# Knuth

  > Programmers waste enormous amounts of time thinking about, or worrying about, the speed of noncritical parts of their programs, and these attempts at efficiency actually have a strong negative impact when debugging and maintenance are considered. We should forget about small efficiencies, say about 97% of the time: premature optimization is the root of all evil. Yet we should not pass up our opportunities in that critical 3%.
>
> Donald Knuth

It worked but is sub-optimal. This project was shelved before the optimal code was identified.

# Copyright

Copyright (c) 2001-2020 Ron Mackley All Rights Reserved.
