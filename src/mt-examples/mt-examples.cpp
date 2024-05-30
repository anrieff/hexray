#include <SDL.h>
#include <math.h>
#include <mutex>
#include "../threading.h"

const int N = 2e6;

const char* USAGE = R"(USAGE: mt-examples <case#>

Where case number can be

0 - single threaded
1 - trivial paralellisation with a race condition
2 - prevent the race via a mutex
3 - add to an array (w/ false sharing)
4 - correct (minimize false sharing)
)";

ThreadPool pool(std::thread::hardware_concurrency());

double sum;

void test_0_single_threaded()
{
	for (int x = N; x >= 1; x--)
		sum += 1.0 / ((double) x * x);
}

void test_1_trivial_parallelisation()
{
	pool.run([] (int threadIdx, int threadCount) {
		for (int x = N-threadIdx; x >= 1; x -= threadCount)
			sum += 1.0 / ((double) x * x);
	});
}

void test_2_with_mutex()
{
	std::mutex lk;
	pool.run([&lk] (int threadIdx, int threadCount) {
		for (int x = N-threadIdx; x >= 1; x -= threadCount) {
			lk.lock();
			sum += 1.0 / ((double) x * x);
			lk.unlock();
		}
	});
}

void test_3_false_sharing()
{
	std::vector<double> sums(std::thread::hardware_concurrency(), 0.0);
	pool.run([&sums] (int threadIdx, int threadCount) {
		for (int x = N-threadIdx; x >= 1; x -= threadCount)
			sums[threadIdx] += 1.0 / ((double) x * x);
	});
	for (double& x: sums) sum += x;
}

void test_4_correct()
{
	std::vector<double> sums(std::thread::hardware_concurrency(), 0.0);
	pool.run([&sums] (int threadIdx, int threadCount) {
		double mySum = 0.0;
		for (int x = N-threadIdx; x >= 1; x -= threadCount)
			mySum += 1.0 / ((double) x * x);
		sums[threadIdx] = mySum;
	});
	for (double& x: sums) sum += x;
}

int main(int argc, char** argv)
{
	int testCase;
	if (argc != 2 || 1 != sscanf(argv[1], "%d", &testCase) || testCase < 0 || testCase > 4) {
		printf(USAGE);
		return -1;
	}
	unsigned ticks = SDL_GetTicks();
	switch (testCase) {
		case 0: test_0_single_threaded(); break;
		case 1: test_1_trivial_parallelisation(); break;
		case 2: test_2_with_mutex(); break;
		case 3: test_3_false_sharing(); break;
		case 4: test_4_correct(); break;
	}
	printf("pi = %.10f\n", sqrt(sum * 6));
	printf("Time required: %d ms\n", SDL_GetTicks() - ticks);
	return 0;
}
