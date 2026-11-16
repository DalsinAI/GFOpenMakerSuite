/* First in every OpenMake library's link, so the file's first code is
 * this: run as a program, a library does nothing.
 * MIT, Copyright (c) 2026 AmiCradle Team. */
int start(void);
int start(void) { return -1; }
