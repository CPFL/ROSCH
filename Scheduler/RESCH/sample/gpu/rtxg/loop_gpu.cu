#include <stdint.h>
#include <cuda.h>
__global__
void loop(uint32_t *a, uint32_t *b, uint32_t *c, uint32_t n)
{
    int i, j, k;

    for (k = 0; k < 10; k ++){
			for (i = 0; i < n; i++) {
	    	for (j = 0; j < n; j++) {
				a[i] = i + n + k;
	    	}
			}
    }
}
