#include "mmap.hpp"

int main(int argc, char **argv) { 
	mmap kernelMapper;

	LOG("Trying to attach to Overwatch...");

	if (!kernelMapper.attach_to_process("Overwatch.exe"))
		return 1;

	if (!kernelMapper.load_dll("libow.dll"))
		return 1;

	if (!kernelMapper.inject())
		return 1;

	LOG("\nPress any key to close.");
	static_cast<void>(std::getchar());
	 
	return 0;
}