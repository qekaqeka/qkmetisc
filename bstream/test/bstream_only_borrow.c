#include "bstream.h"
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include "log.h"

#define FAIL() exit(EXIT_FAILURE)
#define PASS() exit(EXIT_SUCCESS)

int main() {
    log_set_file(stderr);
    log_set_flags(LOG_INFO);

    static const char payload[] = "QWERTYUIOPASDFGHJKLZXCVBNM\n\0\0\0\n";
    struct bstream *bst = bstream_create();
    if ( bst == NULL ) FAIL();

    bstream_write_borrow(bst, payload, sizeof(payload));
    bstream_write_borrow(bst, payload, sizeof(payload));
    bstream_write_borrow(bst, payload, sizeof(payload));
    
    char buf[sizeof(payload)];

    size_t written = bstream_read_mem(bst, buf, sizeof(payload)); //full read check
    if ( written != sizeof(payload) ) FAIL();
    if ( memcmp(buf, payload, sizeof(buf)) ) FAIL();

    size_t part = sizeof(payload) - 9; //partial read check
    written = bstream_read_mem(bst, buf, part);
    if ( written != part ) FAIL();
    if ( memcmp(buf, payload, part) ) FAIL();

    written = bstream_read_mem(bst, buf, sizeof(payload) - part);
    if ( written != (sizeof(payload) - part) ) FAIL();
    if ( memcmp(buf, payload + part, sizeof(payload) - part) ) exit(EXIT_FAILURE);

    written = bstream_read_mem(bst, buf, sizeof(payload)); //full read check
    if ( written != sizeof(payload) ) FAIL();
    if ( memcmp(buf, payload, sizeof(buf)) ) FAIL();

    if ( bstream_len(bst) != 0 ) FAIL();

    PASS();
}
