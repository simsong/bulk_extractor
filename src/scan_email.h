#ifndef SCAN_EMAIL_H
#define SCAN_EMAIL_H
bool extra_validate_email(const char *email);
ssize_t find_host_in_email(const sbuf_t &sbuf);
ssize_t find_host_in_url(const sbuf_t &sbuf, size_t *domain_len);
#endif
