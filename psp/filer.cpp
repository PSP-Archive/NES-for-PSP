#include "pspsdk.h"
#include "../InfoNes.h"
#include "filer.h"
#include "menu.h"

extern u32 new_pad;

dirent_t files[MAX_ENTRY];
int nfiles;

////////////////////////////////////////////////////////////////////////
// クイックソート
int cmpFile(dirent_t *a, dirent_t *b)
{
	char ca, cb;
	int i, n, ret;
	
	if(a->type==b->type){
		n=strlen(a->name);
		for(i=0; i<=n; i++){
			ca=a->name[i]; cb=b->name[i];
			if(ca>='a' && ca<='z') ca-=0x20;
			if(cb>='a' && cb<='z') cb-=0x20;
			
			ret = ca-cb;
			if(ret!=0) return ret;
		}
		return 0;
	}
	
	if(a->type & TYPE_DIR)	return -1;
	else					return 1;
}

void sort(dirent_t *a, int left, int right) {
	dirent_t tmp, pivot;
	int i, p;
	
	if (left < right) {
		pivot = a[left];
		p = left;
		for (i=left+1; i<=right; i++) {
			if (cmpFile(&a[i],&pivot)<0){
				p=p+1;
				tmp=a[p];
				a[p]=a[i];
				a[i]=tmp;
			}
		}
		a[left] = a[p];
		a[p] = pivot;
		sort(a, left, p-1);
		sort(a, p+1, right);
	}
}

// 拡張子管理用
const struct {
	char *szExt;
	int nExtId;
} stExtentions[] = {
 "nes",EXT_NES,
 "zip",EXT_ZIP,
 NULL, EXT_UNKNOWN
};

int getExtId(const char *szFilePath) {
	char *pszExt;
	int i;
	if((pszExt = strrchr(szFilePath, '.'))) {
		pszExt++;
		for (i = 0; stExtentions[i].nExtId != EXT_UNKNOWN; i++) {
			if (!stricmp(stExtentions[i].szExt,pszExt)) {
				return stExtentions[i].nExtId;
			}
		}
	}
	return EXT_UNKNOWN;
}



void getDir(const char *path) {
	int fd, b=0;
	char *p;
	
	nfiles = 0;
	
	if(strcmp(path,"ms0:/")){
		strcpy(files[nfiles].name,"..");
		files[nfiles].type = TYPE_DIR;
		nfiles++;
		b=1;
	}
	
	fd = sceIoDopen(path);
	while(nfiles<MAX_ENTRY){
		if(sceIoDread(fd, &files[nfiles])<=0) break;
		if(files[nfiles].name[0] == '.') continue;
		if(files[nfiles].type == TYPE_DIR){
			strcat(files[nfiles].name, "/");
			nfiles++;
			continue;
		}
		if(getExtId(files[nfiles].name) != EXT_UNKNOWN) nfiles++;
	}
	sceIoDclose(fd);
	if(b)
		sort(files+1, 0, nfiles-2);
	else
		sort(files, 0, nfiles-1);
}

char LastPath[MAX_PATH];
char FilerMsg[256];
int getFilePath(char *out)
{
	unsigned long color;
	int sel=0, rows=21, top=0, x, y, h, i, len, bMsg=0, up=0;
	char path[MAX_PATH], oldDir[MAX_NAME], *p;
	
	strcpy(path, LastPath);
	if(FilerMsg[0])
		bMsg=1;
	
	getDir(path);
	for(;;){
		pspReadPad();
//		readpad();
		if(new_pad)
			bMsg=0;
		if(new_pad & CTRL_CIRCLE){
			if(files[sel].type == TYPE_DIR){
				if(!strcmp(files[sel].name,"..")){
					up=1;
				}else{
					strcat(path,files[sel].name);
					getDir(path);
					sel=0;
				}
			}else{
				strcpy(out, path);
				strcat(out, files[sel].name);
				strcpy(LastPath,path);
				return 1;
			}
		}else if(new_pad & CTRL_CROSS){
			return 0;
		}else if(new_pad & CTRL_TRIANGLE){
			up=1;
		}else if(new_pad & CTRL_UP){
			sel--;
		}else if(new_pad & CTRL_DOWN){
			sel++;
		}else if(new_pad & CTRL_LEFT){
			sel-=10;
		}else if(new_pad & CTRL_RIGHT){
			sel+=10;
		}
		
		if(up){
			if(strcmp(path,"ms0:/")){
				p=strrchr(path,'/');
				*p=0;
				p=strrchr(path,'/');
				p++;
				strcpy(oldDir,p);
				strcat(oldDir,"/");
				*p=0;
				getDir(path);
				sel=0;
				for(i=0; i<nfiles; i++) {
					if(!strcmp(oldDir, files[i].name)) {
						sel=i;
						top=sel-3;
						break;
					}
				}
			}
			up=0;
		}
		
		if(top > nfiles-rows)	top=nfiles-rows;
		if(top < 0)				top=0;
		if(sel >= nfiles)		sel=nfiles-1;
		if(sel < 0)				sel=0;
		if(sel >= top+rows)		top=sel-rows+1;
		if(sel < top)			top=sel;
		
		if(bMsg)
			menu_frame(FilerMsg,"○：OK　×：CANCEL　△：UP");
		else
			menu_frame(path,"○：OK　×：CANCEL　△：UP");
		
		// スクロールバー
		if(nfiles > rows){
			h = 219;
			pspDrawFrame(445,25,446,248,setting.color[1]);
			pspFillBox(448, h*top/nfiles + 27,
				460, h*(top+rows)/nfiles + 27,setting.color[1]);
		}
		
		x=28; y=32;
		for(i=0; i<rows; i++){
			if(top+i >= nfiles) break;
			if(top+i == sel) color = setting.color[2];
			else			 color = setting.color[3];
			pspTextOut(x, y, color, (const unsigned char*)files[top+i].name);
			y+=10;
		}
		
		pspScreenFlipV();
	}
}
