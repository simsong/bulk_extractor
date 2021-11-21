- [ ] Legacy ETA display if stdout is a pipe (not a tty); switch
between the two modes.
- [ ] Python scanner.

- [ ] Generic approach for setting the carve mode for a scanner.
- [ ] Automatically set carve mode for in the main.cpp abstractly for every feature recorder based on name.
- [ ] Use leaks to check for leaks in both BE1.6 and BE2.0

# dfxml:
build environment:
- [ ] Missing CPPFLAGS, CXXFLAGS, LDFLAGS, LIBS and CFLAGS.
- [ ] Missing git commit
- [ ] sbuf_read -> debug:work_start; add t=
- [ ] sbuf_delete -> debug:work_end; add time=;
- [ ] missing <hashdigest> inside <source.>
- [ ] Does not have the time that each thread spent waiting.
- [ ] <total_bytes> is larger than it should be.
- [ ] instead of <ns>, perhaps print <seconds> ?

- [ ] scan_find.
- [ ] searches not working with regular expression to prune thme.


Threading problems:
```
(lldb) bt
* thread #4, stop reason = signal SIGABRT
  * frame #0: 0x00007ff80f77b112 libsystem_kernel.dylib`__pthread_kill + 10
    frame #1: 0x00007ff80f7b1233 libsystem_pthread.dylib`pthread_kill + 263
    frame #2: 0x00007ff80f6fdd10 libsystem_c.dylib`abort + 123
    frame #3: 0x00007ff80f76e0b2 libc++abi.dylib`abort_message + 241
    frame #4: 0x00007ff80f75f1e5 libc++abi.dylib`demangling_terminate_handler() + 242
    frame #5: 0x00007ff80f65c511 libobjc.A.dylib`_objc_terminate() + 104
    frame #6: 0x00007ff80f76d4d7 libc++abi.dylib`std::__terminate(void (*)()) + 8
    frame #7: 0x00007ff80f76fd55 libc++abi.dylib`__cxxabiv1::failed_throw(__cxxabiv1::__cxa_exception*) + 27
    frame #8: 0x00007ff80f76fd1c libc++abi.dylib`__cxa_throw + 116
    frame #9: 0x00007ff80f71aac1 libc++.1.dylib`std::__1::__throw_system_error(int, char const*) + 77
    frame #10: 0x00007ff80f7128ad libc++.1.dylib`std::__1::mutex::lock() + 29
    frame #11: 0x0000000100033ef3 test_be`std::__1::lock_guard<std::__1::mutex>::lock_guard(this=0x0000700004b29520, __m=0x00007ff7bfefd990) at __mutex_base:91:27
    frame #12: 0x0000000100033e9d test_be`std::__1::lock_guard<std::__1::mutex>::lock_guard(this=0x0000700004b29520, __m=0x00007ff7bfefd990) at __mutex_base:91:21
    frame #13: 0x0000000100029301 test_be`atomic_map<std::__1::basic_string<char, std::__1::char_traits<char>, std::__1::allocator<char> >, feature_recorder>::find(this=0x00007ff7bfefd990, key="alerts") const at atomic_map.h:89:43
    frame #14: 0x0000000100029894 test_be`feature_recorder_set::named_feature_recorder(this=0x00007ff7bfefd988, name=<unavailable>) const at feature_recorder_set.cpp:190:19
    frame #15: 0x0000000100029a99 test_be`feature_recorder_set::get_alert_recorder(this=0x00007ff7bfefd988) const at feature_recorder_set.cpp:201:12
    frame #16: 0x000000010009afea test_be`scanner_set::process_sbuf(this=0x00007ff7bfefd6e8, sbufp=0x0000600003904000) at scanner_set.cpp:854:20
    frame #17: 0x00000001000b91de test_be`worker::run(this=0x0000600000c08f60) at threadpool.cpp:133:15
    frame #18: 0x00000001000b889d test_be`worker::start_worker(arg=0x0000600000c08f60) at threadpool.cpp:94:15
    frame #19: 0x00000001000bd5d5 test_be`decltype(__f=(0x0000600000206328), __args=0x0000600000206330)(void*)>(fp)(std::__1::forward<void*>(fp0))) std::__1::__invoke<void* (*)(void*), void*>(void* (*&&)(void*), void*&&) at type_traits:3694:1
    frame #20: 0x00000001000bd54e test_be`void std::__1::__thread_execute<std::__1::unique_ptr<std::__1::__thread_struct, std::__1::default_delete<std::__1::__thread_struct> >, void* (*)(void*), void*, 2ul>(__t=size=3, (null)=__tuple_indices<2> @ 0x0000700004b29f68)(void*), void*>&, std::__1::__tuple_indices<2ul>) at thread:286:5
    frame #21: 0x00000001000bcd25 test_be`void* std::__1::__thread_proxy<std::__1::tuple<std::__1::unique_ptr<std::__1::__thread_struct, std::__1::default_delete<std::__1::__thread_struct> >, void* (*)(void*), void*> >(__vp=0x0000600000206320) at thread:297:5
    frame #22: 0x00007ff80f7b1514 libsystem_pthread.dylib`_pthread_start + 125
    frame #23: 0x00007ff80f7ad02f libsystem_pthread.dylib`thread_start + 15
    ```

```
Target 0: (test_be) stopped.
(lldb) bt
* thread #2, stop reason = EXC_BAD_ACCESS (code=1, address=0x98967f)
  * frame #0: 0x000000000098967f
    frame #1: 0x0000000100231f0d test_be`yyemail_lex(yyscanner=0x0000600003904000) at scan_email.flex:192:26
    frame #2: 0x0000000100236fc4 test_be`::scan_email(sp=0x0000700000954cb8) at scan_email.flex:384:13
    frame #3: 0x000000010009abf7 test_be`scanner_set::process_sbuf(this=0x00007ff7bfefd6e8, sbufp=0x0000600003908000) at scanner_set.cpp:830:13
    frame #4: 0x00000001000b91de test_be`worker::run(this=0x0000600000c00480) at threadpool.cpp:133:15
    frame #5: 0x00000001000b889d test_be`worker::start_worker(arg=0x0000600000c00480) at threadpool.cpp:94:15
    frame #6: 0x00000001000bd5d5 test_be`decltype(__f=(0x0000600000201188), __args=0x0000600000201190)(void*)>(fp)(std::__1::forward<void*>(fp0))) std::__1::__invoke<void* (*)(void*), void*>(void* (*&&)(void*), void*&&) at type_traits:3694:1
    frame #7: 0x00000001000bd54e test_be`void std::__1::__thread_execute<std::__1::unique_ptr<std::__1::__thread_struct, std::__1::default_delete<std::__1::__thread_struct> >, void* (*)(void*), void*, 2ul>(__t=size=3, (null)=__tuple_indices<2> @ 0x0000700000954f68)(void*), void*>&, std::__1::__tuple_indices<2ul>) at thread:286:5
    frame #8: 0x00000001000bcd25 test_be`void* std::__1::__thread_proxy<std::__1::tuple<std::__1::unique_ptr<std::__1::__thread_struct, std::__1::default_delete<std::__1::__thread_struct> >, void* (*)(void*), void*> >(__vp=0x0000600000201180) at thread:297:5
    frame #9: 0x00007ff80f7b1514 libsystem_pthread.dylib`_pthread_start + 125
    frame #10: 0x00007ff80f7ad02f libsystem_pthread.dylib`thread_start + 15
    (lldb)

(lldb) bt
* thread #3, stop reason = signal SIGABRT
  * frame #0: 0x00007ff80f77b112 libsystem_kernel.dylib`__pthread_kill + 10
    frame #1: 0x00007ff80f7b1233 libsystem_pthread.dylib`pthread_kill + 263
    frame #2: 0x00007ff80f6fdd10 libsystem_c.dylib`abort + 123
    frame #3: 0x00007ff80f76e0b2 libc++abi.dylib`abort_message + 241
    frame #4: 0x00007ff80f75f1e5 libc++abi.dylib`demangling_terminate_handler() + 242
    frame #5: 0x00007ff80f65c511 libobjc.A.dylib`_objc_terminate() + 104
    frame #6: 0x00007ff80f76d4d7 libc++abi.dylib`std::__terminate(void (*)()) + 8
    frame #7: 0x00007ff80f76fd55 libc++abi.dylib`__cxxabiv1::failed_throw(__cxxabiv1::__cxa_exception*) + 27
    frame #8: 0x00007ff80f76fd1c libc++abi.dylib`__cxa_throw + 116
    frame #9: 0x00007ff80f71aac1 libc++.1.dylib`std::__1::__throw_system_error(int, char const*) + 77
    frame #10: 0x00007ff80f7128ad libc++.1.dylib`std::__1::mutex::lock() + 29
    frame #11: 0x0000000100033ef3 test_be`std::__1::lock_guard<std::__1::mutex>::lock_guard(this=0x0000700002720520, __m=0x00007ff7bfefd990) at __mutex_base:91:27
    frame #12: 0x0000000100033e9d test_be`std::__1::lock_guard<std::__1::mutex>::lock_guard(this=0x0000700002720520, __m=0x00007ff7bfefd990) at __mutex_base:91:21
    frame #13: 0x0000000100029301 test_be`atomic_map<std::__1::basic_string<char, std::__1::char_traits<char>, std::__1::allocator<char> >, feature_recorder>::find(this=0x00007ff7bfefd990, key="alerts") const at atomic_map.h:89:43
    frame #14: 0x0000000100029894 test_be`feature_recorder_set::named_feature_recorder(this=0x00007ff7bfefd988, name=<unavailable>) const at feature_recorder_set.cpp:190:19
    frame #15: 0x0000000100029a99 test_be`feature_recorder_set::get_alert_recorder(this=0x00007ff7bfefd988) const at feature_recorder_set.cpp:201:12
    frame #16: 0x000000010009afea test_be`scanner_set::process_sbuf(this=0x00007ff7bfefd6e8, sbufp=0x0000600003900000) at scanner_set.cpp:854:20
    frame #17: 0x00000001000b91de test_be`worker::run(this=0x0000600000c0c9f0) at threadpool.cpp:133:15
    frame #18: 0x00000001000b889d test_be`worker::start_worker(arg=0x0000600000c0c9f0) at threadpool.cpp:94:15
    frame #19: 0x00000001000bd5d5 test_be`decltype(__f=(0x000060000020a7c8), __args=0x000060000020a7d0)(void*)>(fp)(std::__1::forward<void*>(fp0))) std::__1::__invoke<void* (*)(void*), void*>(void* (*&&)(void*), void*&&) at type_traits:3694:1
    frame #20: 0x00000001000bd54e test_be`void std::__1::__thread_execute<std::__1::unique_ptr<std::__1::__thread_struct, std::__1::default_delete<std::__1::__thread_struct> >, void* (*)(void*), void*, 2ul>(__t=size=3, (null)=__tuple_indices<2> @ 0x0000700002720f68)(void*), void*>&, std::__1::__tuple_indices<2ul>) at thread:286:5
    frame #21: 0x00000001000bcd25 test_be`void* std::__1::__thread_proxy<std::__1::tuple<std::__1::unique_ptr<std::__1::__thread_struct, std::__1::default_delete<std::__1::__thread_struct> >, void* (*)(void*), void*> >(__vp=0x000060000020a7c0) at thread:297:5
    frame #22: 0x00007ff80f7b1514 libsystem_pthread.dylib`_pthread_start + 125
    frame #23: 0x00007ff80f7ad02f libsystem_pthread.dylib`thread_start + 15



    ```

It really does seem like the scanner_set map is a problem. It is not threadsafe.
