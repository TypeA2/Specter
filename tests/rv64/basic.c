__attribute__ ((noreturn)) extern void _exit();

__attribute__ ((noreturn)) void _start() {
    _exit(-42);
}
