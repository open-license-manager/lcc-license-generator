#ifndef BASE_H_
#define BASE_H_

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __unix__
#include <limits.h>
#define DllExport
#ifndef MAX_PATH
#define MAX_PATH PATH_MAX
#endif

#else  // windows
#include <windows.h>
#define DllExport __declspec(dllexport)

#ifndef __cplusplus
#ifndef _MSC_VER
#include <stdbool.h>
#else
typedef int bool;
#define false 0
#define true - 1
#endif
#endif

#endif

#define PRIVATE_KEY_FNAME "private_key.rsa"
#define PUBLIC_KEY_INC_FNAME "public_key.h"

/*
 * command line parameters
 */
#define PARAM_BASE64 "base64"
#define PARAM_EXPIRY_DATE "expiry-date"
#define PARAM_LICENSE_NAME "license-name"
#define PARAM_PRODUCT_NAME "product-name"
#define PARAM_PROJECT_FOLDER "project-folder"
#define PARAM_PRIMARY_KEY "primary-key"

typedef enum { FUNC_RET_OK, FUNC_RET_NOT_AVAIL, FUNC_RET_ERROR, FUNC_RET_BUFFER_TOO_SMALL } FUNCTION_RETURN;

#ifdef __cplusplus
}
#endif

#endif
