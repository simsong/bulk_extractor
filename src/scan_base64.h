#ifndef SCAN_BASE64_H
#define SCAN_BASE64_H
void base64array_initialize();
bool sbuf_line_is_base64(const sbuf_t &sbuf,size_t start,size_t len,bool &found_equal);
sbuf_t *decode_base64(const sbuf_t &sbuf, size_t start, size_t src_len);
#endif
