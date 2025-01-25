/**
 * @file  cfgpath.h
 * @brief Cross platform methods for obtaining paths to configuration files.
 *
 * Copyright (C) 2013 Adam Nielsen <malvineous@shikadi.net>
 *
 * This code is placed in the public domain.  You are free to use it for any
 * purpose.  If you add new platform support, please contribute a patch!
 *
 * Example use:
 *
 * char cfgdir[256];
 * get_user_config_file(cfgdir, sizeof(cfgdir), "myapp");
 * if (cfgdir[0] == 0) {
 *     printf("Unable to find home directory.\n");
 *     return 1;
 * }
 * printf("Saving configuration file to %s\n", cfgdir);
 *
 * A number of constants are also defined:
 *
 *  - MAX_PATH: Maximum length of a path, in characters.  Used to allocate a
 *      char array large enough to hold the returned path.
 *
 *  - PATH_SEPARATOR_CHAR: The separator between folders.  This will be either a
 *      forward slash or a backslash depending on the platform.  This is a
 *      character constant.
 *
 *  - PATH_SEPARATOR_STRING: The same as PATH_SEPARATOR_CHAR but as a C string,
 *      to make it easier to append to other string constants.
 */
#ifndef CFGPATH_H_
#define CFGPATH_H_

#ifdef __cplusplus
extern "C" {
#endif

extern void get_user_config_file(char *out, size_t maxlen, const char *appname);
extern void get_user_config_folder(char *out, size_t maxlen, const char *appname);
extern void get_user_data_folder(char *out, size_t maxlen, const char *appname);
extern void get_user_cache_folder(char *out, size_t maxlen, const char *appname);

#ifdef __cplusplus
}
#endif

#endif /* CFGPATH_H_ */
