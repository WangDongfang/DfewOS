

#include "mtdtypes.h"

int tick_get(void);

static int timestamp = 0;
static ulong __GulTick = 0;

void reset_timer (void)
{
	timestamp = 0;
	__GulTick = tick_get();
}

ulong get_timer (ulong base)
{
	timestamp = tick_get()-__GulTick;
	return (timestamp - base);
}
#if 0
void set_timer (ulong t)
{
}
#endif



