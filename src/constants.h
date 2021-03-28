#ifndef CONSTANTS_H
#define CONSTANTS_H

extern PyObject *ConstantUnicodeHTTP;
extern PyObject *ConstantUnicodeHTTPS;
extern PyObject *ConstantUnicode_mount;

int Constants_Init(void);
void Constants_Free(void);

#endif
