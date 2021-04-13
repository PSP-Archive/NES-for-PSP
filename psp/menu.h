#ifndef MENU_H
#define MENU_H

void	load_config(void);
void	load_menu_bg();
long	load_rom(const char *pszFile);
void	menu_frame(const char *msg0, const char *msg1);
void	menu(void);

typedef struct
{
	char vercnf[16];
	int vsync;
	int sound;
	int screensize;
	int cpuclock;
	int frameskip;
	int key_config[16]; //—]•ª‚ÉŠm•Û
	unsigned long color[4];
	int bgbright;
} SETTING;

enum{
	DEF_COLOR0=0x9063,
	DEF_COLOR1=RGB(85,85,95),
	DEF_COLOR2=RGB(105,105,115),
	DEF_COLOR3=0xffff,
};

extern char CurPath[], szRomName[], szSaveName[];
extern SETTING setting;
extern unsigned char rom_image[];

#endif
