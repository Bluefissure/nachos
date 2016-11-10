#include "syscall.h"

int main()
{
	SpaceId pid;

	pid=Exec("../test/halt.noff");
	Halt();
	/*not reached*/
}
