
#ifndef _MEMMAPS_H_
#define _MEMMAPS_H_


    

typedef struct _MemmapsInfo MemmapsInfo;

enum
{
	COL_FILENAME,
	COL_VMSTART,
	COL_VMEND,
	COL_FLAGS,
	COL_VMOFFSET,
	COL_DEVICE,
	COL_INODE,
};

struct _MemmapsInfo
{
	gchar                   *filename;
        gchar			*vmstart;
        gchar			*vmend;
        gchar			*flags;
        gchar			*vmoffset;
        gchar			*device;
        gchar			*inode;
};

void 		create_memmaps_dialog (ProcData *procdata);


#endif
