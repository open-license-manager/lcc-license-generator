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

/**
 * Version at the beginning of license file.
 */
#define LICENSE_FILE_VERSION 200

/*
 * command line parameters
 */
#define PARAM_BASE64 "base64"
#define PARAM_LICENSE_OUTPUT "output-file-name"
#define PARAM_FEATURE_NAMES "feature-names"
#define PARAM_PROJECT_FOLDER "project-folder"
#define PARAM_PRIMARY_KEY "primary-key"

// license file parameters -- copy this block to open-license-manager
#define PARAM_BEGIN_DATE "valid-from"
#define PARAM_CLIENT_SIGNATURE "client-signature"
#define PARAM_EXPIRY_DATE "valid-to"
#define PARAM_VERSION_FROM "start-version"
#define PARAM_VERSION_TO "end-version"
#define PARAM_EXTRA_DATA "extra-data"
// license file extra entries
#define LICENSE_SIGNATURE "sig"
#define LICENSE_VERSION "lic_ver"
#define PARAM_MAGIC_NUMBER \
	"magic-num"  // this parameter must matched with the magic number passed in by the
				 // application
// license file parameters -- copy this block to open-license-manager

typedef enum { FUNC_RET_OK, FUNC_RET_NOT_AVAIL, FUNC_RET_ERROR, FUNC_RET_BUFFER_TOO_SMALL } FUNCTION_RETURN;

#ifdef __cplusplus
}
#endif

#endif
