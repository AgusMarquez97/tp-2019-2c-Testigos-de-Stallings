#include "Linuse.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

int main(void) {

	muse_init(getpid(),"127.0.0.1",5003);

	uint32_t xxi = muse_alloc(21);
	uint32_t ix = muse_alloc(9);
	uint32_t xvii = muse_alloc(17);
	uint32_t v = muse_alloc(5);
	muse_free(xvii);
	uint32_t x = muse_alloc(10);
	uint32_t xl = muse_alloc(40);
	muse_free(xxi);
	uint32_t c = muse_alloc(100);
	muse_free(x);
	muse_free(v);
	muse_free(c);
	muse_free(ix);
	muse_free(xl);
	uint32_t cxi = muse_alloc(111);
	muse_free(cxi);

	muse_close();

	return EXIT_SUCCESS;

}
