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
        uint32_t xxvA = muse_alloc(25);
        uint32_t xxx = muse_alloc(30);
        uint32_t vii = muse_alloc(7);
        muse_free(xxx);
        uint32_t xxvB = muse_alloc(25);

        muse_cpy(xxvA,"toda la looz!!\n", 16);
        muse_cpy(xxvB,"cortaste ", 10);
        muse_cpy(vii,"MAMA ", 7);

        char* buffer_xxvA = malloc(16);
        char* buffer_xxvB = malloc(10);
        char* buffer_vii = malloc(7);

        muse_get(buffer_vii, vii, 7);
        muse_get(buffer_xxvB, xxvB, 10);
        muse_get(buffer_xxvA, xxvA, 16);

        muse_free(xxvA);
        muse_free(xxvB);
        muse_free(vii);

        printf("%s",buffer_vii);
        printf("%s",buffer_xxvB);
        printf("%s",buffer_xxvA);

        free(buffer_xxvA);
        free(buffer_xxvB);
        free(buffer_vii);

        size_t tamArchivoMax = 250;
        char* homero = malloc(tamArchivoMax);
        char* bart = malloc(tamArchivoMax);
        char* maggie = malloc(tamArchivoMax);
        void* todos = malloc(tamArchivoMax*3);

        uint32_t direccionMemoria1 = muse_map("archivos/homero.txt", tamArchivoMax, MUSE_MAP_SHARED);
        uint32_t direccionMemoria2 = muse_map("archivos/bart.txt", tamArchivoMax, MUSE_MAP_SHARED);
        uint32_t direccionMemoria3 = muse_map("archivos/maggie.txt", tamArchivoMax, MUSE_MAP_SHARED);
        uint32_t direccionMemoria4 = muse_map("archivos/todos.txt", tamArchivoMax*3, MUSE_MAP_SHARED);

        uint32_t h = muse_alloc(250);
        uint32_t b = muse_alloc(250);
        uint32_t m = muse_alloc(250);

        muse_get(homero, direccionMemoria1, tamArchivoMax);
        muse_get(bart, direccionMemoria2, tamArchivoMax);
        muse_get(maggie, direccionMemoria3, tamArchivoMax);

        muse_get(todos, direccionMemoria1, tamArchivoMax);
        muse_get(todos + tamArchivoMax, direccionMemoria2, tamArchivoMax);
        muse_get(todos + (tamArchivoMax*2), direccionMemoria3, tamArchivoMax);

        muse_cpy(h, homero, 250);
        muse_cpy(b, bart, 250);
        muse_cpy(m, maggie, 250);

        muse_cpy(direccionMemoria1, bart, strlen(bart));
        muse_cpy(direccionMemoria2, maggie, strlen(maggie));
        muse_cpy(direccionMemoria3, homero, strlen(homero));

        muse_cpy(direccionMemoria4, todos, tamArchivoMax*3);

        muse_sync(direccionMemoria1, strlen(bart));
        muse_sync(direccionMemoria2, strlen(maggie));
        muse_sync(direccionMemoria3, strlen(homero));

        muse_sync(direccionMemoria4, tamArchivoMax*3);

        memset(homero, 0, 250);
        memset(bart, 0, 250);
        memset(maggie, 0, 250);

        muse_get(homero, h, 250);
        muse_get(bart, b, 250);
        muse_get(maggie, m, 250);

        printf("%s\n", homero);
        printf("%s\n", bart);
        printf("%s\n", maggie);

        muse_unmap(direccionMemoria1);
        muse_unmap(direccionMemoria2);
        muse_unmap(direccionMemoria3);
        muse_unmap(direccionMemoria4);

        free(homero);
        free(bart);
        free(maggie);
        free(todos);

        muse_free(h);
        muse_free(b);
        muse_free(m);

        muse_close();

        return EXIT_SUCCESS;

}
