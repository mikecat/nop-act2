int _start(void* arg1) {
	volatile int marker = 0xDEADBEEF;
	for (;;);
	/* avoid unused warnings */
	(void)arg1;
	(void)marker;
}
