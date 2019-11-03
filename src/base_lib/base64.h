#ifndef BASE64_H
#define BASE64_H

namespace license {

unsigned char* unbase64(const char *ascii, int len, int *flen);
char* base64(const void *binaryData, size_t len, size_t *flen);

}

#endif
