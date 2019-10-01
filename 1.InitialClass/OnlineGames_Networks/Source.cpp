#include <thread>
#include <Windows.h>
#include <iostream>
#include <mutex>

void function(int x, int y, int z) {
	Sleep(1000);
	std::cout << x;
	std::cout << y;
	std::cout << z;
}

int counter = 0; // Global var
std::mutex mutex;

void incrementCounter(int iterations) {
	for (int i = 0; i < iterations; ++i) {
		std::unique_lock<std::mutex> lock(mutex); /* if the thread enters here, it must finish the  "counter++" 
												  otherwise it is locked, to avoid the change of thread in the
		                                          middle of the assembler subdivision task of the "counter++" execution */
		counter++;
	}
}

int main() {
	std::thread t1(incrementCounter, 100000); // Execute 'increment' in a thread
	std::thread t2(incrementCounter, 100000); // Execute 'increment' in a thread
	t1.join(); // Wait for t1
	t2.join(); // Wait for t2
	std::cout << "Counter = " << counter << std::endl;
	getchar();
	return 0;}