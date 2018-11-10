
#include <stdio.h>
#include <omp.h>

long long fib(long long n)
{
	if (n < 2) {
		return 1;
	}
	return fib(n - 2) + fib(n - 1);
}

int main(int argc, char ** argv)
{
	long long n = 0;

	#pragma omp parallel private(n)
	{
		#pragma omp for schedule(dynamic, 1)
		for (n = 0; n <= 50; n++) {
			printf("Fib(%lld): %lld\n", n, fib(n));
		}

	}

	return 0;
}
