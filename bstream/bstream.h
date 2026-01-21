#pragma once

#include <stddef.h>
#include <stdio.h>

struct bstream;

extern struct bstream *bstream_create();
extern void bstream_destroy(struct bstream *bs);


typedef size_t (*bstream_reader_t)(void *dest, void *arg, size_t len);

extern size_t bstream_memreader(void *dest, void *arg, size_t len);
extern size_t bstream_fpreader(void *dest, void *arg, size_t len);

#if _POSIX_C_SOURCE >= 1
extern size_t bstream_fdreader(void *dest, void *arg, size_t len);
#endif

extern size_t bstream_write_borrow(struct bstream *bs, const void *buf, size_t len);
extern size_t bstream_write(struct bstream *bs, bstream_reader_t reader, void *arg, size_t len);
extern size_t bstream_write_mem(struct bstream *bs, const void *src, size_t len);
extern size_t bstream_write_fp(struct bstream *bs, FILE *fp, size_t len);

#if _POSIX_C_SOURCE >= 1
extern size_t bstream_write_fd(struct bstream *bs, int fd, size_t len);
#endif


typedef size_t (*bstream_writer_t)(void *arg, const void *src, size_t len);

extern size_t bstream_memwriter(void *arg, const void *src, size_t len);
extern size_t bstream_fpwriter(void *arg, const void *src, size_t len);

#if _POSIX_C_SOURCE >= 1
extern size_t bstream_fdwriter(void *arg, const void *src, size_t len);
#endif


extern size_t bstream_read(struct bstream *bs, bstream_writer_t writer, void *arg, size_t len);
extern size_t bstream_read_mem(struct bstream *bs, void *dest, size_t len);
extern size_t bstream_read_fp(struct bstream *bs, FILE *fp, size_t len);

#if _POSIX_C_SOURCE >= 1
extern size_t bstream_read_fd(struct bstream *bs, int fd, size_t len);
#endif


extern size_t bstream_len(const struct bstream *bs);
extern void bstream_flush(struct bstream *bs);
