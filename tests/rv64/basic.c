__attribute__ ((noreturn)) extern void _exit();

#ifdef INDIRECT
int get() {
    return 42;
}
#endif

__attribute__ ((noreturn)) void _start() {
#ifdef INDIRECT
    _exit(-get());
#else
    _exit(-42);
#endif
}
