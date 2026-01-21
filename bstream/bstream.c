#include "bstream.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "list.h"
#include "utils.h"

enum bstream_buffer_type {
    BSTREAM_BUFFER_BORROWED,
    BSTREAM_BUFFER_COPIED,
    BSTREAM_BUFFER_TYPES_NR
};

#define BSTREAM_BUFFER_COPIED_SIZE 256

struct bstream_buffer {
    union {
        struct bstream_buffer_borrowed {
            const void *borrowed_bytes;
            size_t offset;   // Already read bytes count
            size_t len;      // Available to read bytes count
        } borrowed;
        struct bstream_buffer_copied {
            void *bytes;
            size_t offset;       // Already read bytes count
            size_t len;          // Available to read bytes count
            size_t free_space;   // Available to write bytes count

            // offset + len + free_space == BSTREAM_BUFFER_COPIED_SIZE
        } copied;
    } buffer;

    enum bstream_buffer_type type;

    struct list chain;
};

typedef size_t (*bstream_buffer_write_t)(struct bstream_buffer *bb, bstream_reader_t reader, void *arg, size_t len);
typedef size_t (*bstream_buffer_read_t)(struct bstream_buffer *bb, bstream_writer_t writer, void *arg, size_t len);
typedef bool (*bstream_buffer_readable_t)(struct bstream_buffer *bb);
typedef bool (*bstream_buffer_writable_t)(struct bstream_buffer *bb);
typedef void (*bstream_buffer_destroy_t)(struct bstream_buffer *bb);

struct bstream_buffer_ops {
    bstream_buffer_write_t write;
    bstream_buffer_read_t read;
    bstream_buffer_readable_t readable;
    bstream_buffer_writable_t writable;
    bstream_buffer_destroy_t destroy;
};


static struct bstream_buffer *bstream_buffer_copied_create() {
    struct bstream_buffer *bb = malloc(sizeof(struct bstream_buffer));
    if ( bb == NULL ) return NULL;

    list_init(bb, chain);
    bb->type = BSTREAM_BUFFER_COPIED;

    struct bstream_buffer_copied *bbc = &bb->buffer.copied;
    bbc->bytes = malloc(BSTREAM_BUFFER_COPIED_SIZE);
    if ( bbc->bytes == NULL ) {
        free(bb);
        return NULL;
    }

    bbc->free_space = BSTREAM_BUFFER_COPIED_SIZE;
    bbc->len = 0;
    bbc->offset = 0;

    return bb;
}

static struct bstream_buffer *bstream_buffer_borrowed_create(const void *data, size_t len) {
    struct bstream_buffer *bb = malloc(sizeof(struct bstream_buffer));
    if ( bb == NULL ) return NULL;

    list_init(bb, chain);
    bb->type = BSTREAM_BUFFER_BORROWED;

    struct bstream_buffer_borrowed *bbb = &bb->buffer.borrowed;
    bbb->len = len;
    bbb->borrowed_bytes = data;
    bbb->offset = 0;

    return bb;
}

static size_t bstream_buffer_copied_write(struct bstream_buffer *bb, bstream_reader_t reader, void *arg, size_t len);
static size_t bstream_buffer_copied_read(struct bstream_buffer *bb, bstream_writer_t writer, void *arg, size_t len);
static bool bstream_buffer_copied_writable(struct bstream_buffer *bb);
static bool bstream_buffer_copied_readable(struct bstream_buffer *bb);
static void bstream_buffer_copied_destroy(struct bstream_buffer *bb);

static size_t bstream_buffer_borrowed_write(struct bstream_buffer *bb, bstream_reader_t reader, void *arg, size_t len);
static size_t bstream_buffer_borrowed_read(struct bstream_buffer *bb, bstream_writer_t writer, void *arg, size_t len);
static bool bstream_buffer_borrowed_writable(struct bstream_buffer *bb);
static bool bstream_buffer_borrowed_readable(struct bstream_buffer *bb);
static void bstream_buffer_borrowed_destroy(struct bstream_buffer *bb);

static struct bstream_buffer_ops bbops[BSTREAM_BUFFER_TYPES_NR] = {
    [BSTREAM_BUFFER_COPIED] =
        {
            .write = bstream_buffer_copied_write,
            .read = bstream_buffer_copied_read,
            .writable = bstream_buffer_copied_writable,
            .readable = bstream_buffer_copied_readable,
            .destroy = bstream_buffer_copied_destroy  
        },
    [BSTREAM_BUFFER_BORROWED] = {
            .write = bstream_buffer_borrowed_write,
            .read = bstream_buffer_borrowed_read,
            .writable = bstream_buffer_borrowed_writable,
            .readable = bstream_buffer_borrowed_readable,
            .destroy = bstream_buffer_borrowed_destroy
    }
};

static size_t bstream_buffer_write(struct bstream_buffer *bb, bstream_reader_t reader, void *arg, size_t len) {
    return bbops[bb->type].write(bb, reader, arg, len);
}

static size_t bstream_buffer_read(struct bstream_buffer *bb, bstream_writer_t writer, void *arg, size_t len) {
    return bbops[bb->type].read(bb, writer, arg, len);
}

static bool bstream_buffer_writable(struct bstream_buffer *bb) {
    return bbops[bb->type].writable(bb);
}

static bool bstream_buffer_readable(struct bstream_buffer *bb) {
    return bbops[bb->type].readable(bb);
}

static void bstream_buffer_destroy(struct bstream_buffer *bb) {
    return bbops[bb->type].destroy(bb);
}


static size_t bstream_buffer_copied_write(struct bstream_buffer *bb, bstream_reader_t reader, void *arg, size_t len) {
    if ( !bstream_buffer_copied_writable(bb) ) return 0;

    struct bstream_buffer_copied *bbc = &bb->buffer.copied;

    size_t bytes_to_write = min(len, bbc->free_space);

    size_t written = reader(shiftptr(bbc->bytes, bbc->offset + bbc->len), arg, bytes_to_write);

    bbc->free_space -= written;
    bbc->len += written;

    assert(bbc->len + bbc->offset + bbc->free_space == BSTREAM_BUFFER_COPIED_SIZE);

    return written;
}

static size_t bstream_buffer_copied_read(struct bstream_buffer *bb, bstream_writer_t writer, void *arg, size_t len) {
    struct bstream_buffer_copied *bbc = &bb->buffer.copied;

    size_t bytes_to_read = min(len, bbc->len);

    size_t read = writer(arg, shiftptr(bbc->bytes, bbc->offset), bytes_to_read);

    bbc->offset += read;
    bbc->len -= read;

    assert(bbc->len + bbc->offset + bbc->free_space == BSTREAM_BUFFER_COPIED_SIZE);

    return read;
}

static bool bstream_buffer_copied_writable(struct bstream_buffer *bb) {
    struct bstream_buffer_copied *bbc = &bb->buffer.copied;
    return bbc->free_space > 0;
}


static bool bstream_buffer_copied_readable(struct bstream_buffer *bb) {
    struct bstream_buffer_copied *bbc = &bb->buffer.copied;
    return bbc->len > 0;
}

static void bstream_buffer_copied_destroy(struct bstream_buffer *bb) {
    struct bstream_buffer_copied *bbc = &bb->buffer.copied;
    free(bbc->bytes);
    free(bb);
}

static size_t bstream_buffer_borrowed_write(struct bstream_buffer *bb, bstream_reader_t reader, void *arg, size_t len) {
    unused(bb);
    unused(reader);
    unused(arg);
    unused(len);

    return 0;
}

static size_t bstream_buffer_borrowed_read(struct bstream_buffer *bb, bstream_writer_t writer, void *arg, size_t len) {
    struct bstream_buffer_borrowed *bbb = &bb->buffer.borrowed;

    size_t bytes_to_read = min(len, bbb->len);

    size_t read = writer(arg, shiftptr(bbb->borrowed_bytes, bbb->offset), bytes_to_read);

    bbb->offset += read;
    bbb->len -= read;

    return read;
}

static bool bstream_buffer_borrowed_writable(struct bstream_buffer *bb) {
    unused(bb);

    return false;
}

static bool bstream_buffer_borrowed_readable(struct bstream_buffer *bb) {
    struct bstream_buffer_borrowed *bbb = &bb->buffer.borrowed;
    return bbb->len > 0;
}

static void bstream_buffer_borrowed_destroy(struct bstream_buffer *bb) {
    free(bb);
}

struct bstream {
    struct bstream_buffer *head;
    struct bstream_buffer *tail;
    size_t total_len;
};

struct bstream *bstream_create() {
    struct bstream *bs = malloc(sizeof(struct bstream));
    if ( bs == NULL ) return NULL;

    list_init_head_tail(bs, head, tail);
    bs->total_len = 0;

    return bs;
}

void bstream_destroy(struct bstream *bs) {
    assert(bs);

    if ( bs->head != NULL ) {
        list_foreach_safe(bs->head, chain, iter) {
            if ( iter->type == BSTREAM_BUFFER_COPIED ) free(iter->buffer.copied.bytes);
            free(iter);
        }
    }
    free(bs);
}

size_t bstream_write_borrow(struct bstream *bs, const void *buf, size_t len) {
    size_t new_total_len = bs->total_len;
    if ( ckd_add(&new_total_len, new_total_len, len) ) return 0;

    struct bstream_buffer *bb = bstream_buffer_borrowed_create(buf, len);
    if ( bb == NULL ) return 0;

    list_add_item_back(&bs->head, &bs->tail, bb, chain);

    bs->total_len = new_total_len;

    return len;
}

size_t bstream_write(struct bstream *bs, bstream_reader_t reader, void *arg, size_t len) {
    size_t new_total_len = bs->total_len;
    if ( ckd_add(&new_total_len, new_total_len, len) ) return 0;   // Check overflow

    struct bstream_buffer *lbb = bs->tail;
    size_t written = 0;
    if ( lbb != NULL ) { //fill last buffer
        written += bstream_buffer_write(lbb, reader, arg, len);
        if ( bstream_buffer_writable(lbb) || written == len ) {
            // The last buffer can be writable for two reasons:
            // 1. All the data was stored there
            // 2. Reader error occured
            return written;
        }
    }

    size_t remain = len - written;

    while ( true ) {
        struct bstream_buffer *bb = bstream_buffer_copied_create();
        if ( bb == NULL ) {
            bs->total_len += written;
            return written;
        }

        list_add_item_back(&bs->head, &bs->tail, bb, chain);

        size_t res = bstream_buffer_copied_write(bb, reader, arg, remain);
        if ( bstream_buffer_copied_writable(bb) || res == remain ) {
            // The last buffer can be writable for two reasons:
            // 1. All the data was stored there
            // 2. Reader error occured
            written += res;
            bs->total_len += written;
            return written;
        }

        remain -= res;
        written += res;
    }
}

size_t bstream_memreader(void *dest, void *arg, size_t len) {
    void **src = (void *)arg;
    memcpy(dest, *src, len);
    *src = shiftptr(*src, len);
    return len;
}

size_t bstream_fpreader(void *dest, void *arg, size_t len) {
    FILE *fp = arg;
    size_t written = fread(dest, 1, len, fp);
    return written;
}

#if _POSIX_C_SOURCE >= 1
    #include <unistd.h>

size_t bstream_fdreader(void *dest, void *arg, size_t len) {
    int fd = *(int *)arg;
    ssize_t written = read(fd, dest, len);
    if ( written == -1 ) return 0;

    return written;
}
#endif

size_t bstream_write_mem(struct bstream *bs, const void *src, size_t len) {
    return bstream_write(bs, bstream_memreader, &src, len);
}

size_t bstream_write_fp(struct bstream *bs, FILE *fp, size_t len) {
    return bstream_write(bs, bstream_fpreader, fp, len);
}

#if _POSIX_C_SOURCE >= 1
size_t bstream_write_fd(struct bstream *bs, int fd, size_t len) {
    return bstream_write(bs, bstream_fdreader, &fd, len);
}
#endif


size_t bstream_memwriter(void *arg, const void *src, size_t len) {
    void **dest = arg;
    memcpy(*dest, src, len);
    *dest = shiftptr(*dest, len);
    return len;
}

size_t bstream_fpwriter(void *arg, const void *src, size_t len) {
    FILE *fp = arg;
    size_t written = fwrite(src, 1, len, fp);
    return written;
}

#if _POSIX_C_SOURCE >= 1
size_t bstream_fdwriter(void *arg, const void *src, size_t len) {
    int fd = *(int *)arg;
    ssize_t written = write(fd, src, len);
    if ( written == -1 ) return 0;
    return written;
}
#endif

size_t bstream_read(struct bstream *bs, bstream_writer_t writer, void *arg, size_t len) {
    size_t read = 0;
    size_t remain = len;

    if ( !list_no_items(&bs->head, &bs->tail) ) {
        list_foreach_safe(bs->head, chain, iter) {
            size_t res = bstream_buffer_read(iter, writer, arg, remain);
            read += res;

            if ( bstream_buffer_readable(iter) ) {
                bs->total_len -= read;
                return read;
            }

            list_remove_item(&bs->head, &bs->tail, iter, chain);
            bstream_buffer_destroy(iter);

            if ( res == remain ) {
                bs->total_len -= read;
                return read;
            }
        }
    }

    bs->total_len -= read;
    return read;
}

size_t bstream_read_mem(struct bstream *bs, void *buf, size_t len) {
    return bstream_read(bs, bstream_memwriter, &buf, len);
}

size_t bstream_read_fp(struct bstream *bs, FILE *fp, size_t len) {
    return bstream_read(bs, bstream_fpwriter, fp, len);
}

#if _POSIX_C_SOURCE >= 1
size_t bstream_read_fd(struct bstream *bs, int fd, size_t len) {
    return bstream_read(bs, bstream_fdwriter, &fd, len);
}
#endif

size_t bstream_len(const struct bstream *bs) {
    assert(bs);
    return bs->total_len;
}

void bstream_flush(struct bstream *bs) {
    assert(bs);
    if ( bs->head != NULL ) {
        list_foreach_safe(bs->head, chain, iter) {
            bstream_buffer_destroy(iter);
        }
    }

    list_init_head_tail(bs, head, tail);
    bs->total_len = 0;
}

bool bstream_empty(const struct bstream *bs) {
    return bs->total_len == 0;
}
