/*
 * Include file for url_fopen.c
 */

#ifndef URL_FOPEN
#define URL_FOPEN

/* forware declaration */
typedef struct fcurl_data URL_FILE;

/* exported functions */
URL_FILE *url_fopen(char *url,const char *operation);
int       url_setverbose(URL_FILE *file, int verbose);
int       url_setprogress(URL_FILE* file, int progress);
int       url_fclose(URL_FILE *file);
int       url_feof(URL_FILE *file);
size_t    url_fread(void *ptr, size_t size, size_t nmemb, URL_FILE *file);
char *    url_fgets(char *ptr, int size, URL_FILE *file);
void      url_rewind(URL_FILE *file);

#endif /* URL_FOPEN */
