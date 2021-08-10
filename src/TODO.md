- [ ] BE_DEBUG = print_steps to print the steps
- [ ] Make sure that process() and recurse() and anything else that's
  going to free an sbuf always get an sbuf with no parent, becuase
  otherwise the parent might be freed during the recursion, to allow
  for asynchronouse recursion.
- [ ] Generic approach for setting the carve mode for a scanner.
- [ ] remove the file iterator from the image iterator, run it separately.
      Just put one file on the list, vs. many.
      map the entire file but process it with pages and margins using overlapping sbufs. Free it when finished?

- [ ] When carving tests/len6192.jpg, the JPEG should only be 6192 bytes.
- [ ] When scanning tests/testfilex.docx, the JPEG should only be 6192 bytes
- [ ] Automatically set carve mode for in the main.cpp abstractly for
  every feature recorder based on name.

# scan_net
- [ ] test against v1.6 with IPv4 UDP
- [ ] test against v1.6 with IPv4 TCP
- [ ] test against v1.6 with IPv6 UDP
- [ ] test against v1.6 with IPv6 TCP

# dfxml:
build environment:
- [ ] Missing CPPFLAGS, CXXFLAGS, LDFLAGS, LIBS and CFLAGS.
- [ ] Missing git commit
- [ ] sbuf_read -> debug:work_start; add t=
- [ ] sbuf_delete -> debug:work_end; add time=;
- [ ] missing <hashdigest> inside <source.>
- [ ] Does not have the time that each thread spent waiting.
- [ ] <total_bytes> is larger than it should be.
