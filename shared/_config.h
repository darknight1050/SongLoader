#pragma once

#define SONGLOADER_EXPORT __attribute__((visibility("default")))
#ifdef __cplusplus
#define SONGLOADER_EXPORT_FUNC extern "C" SONGLOADER_EXPORT
#else
#define SONGLOADER_EXPORT_FUNC SONGLOADER_EXPORT
#endif
