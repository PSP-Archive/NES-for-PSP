#ifndef FILER_H
#define FILER_H

extern char LastPath[], FilerMsg[];

int getExtId(const char *szFilePath);
void getDir(const char *path);

int searchFile(const char *path, const char *name);
int getFilePath(char *out);

// —LŒø‚ÈŠg’£Žq
enum {
	EXT_NES,
	EXT_ZIP,
	EXT_UNKNOWN
};



#endif
