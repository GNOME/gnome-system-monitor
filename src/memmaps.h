#ifndef _PROCMAN_MEMMAPS_H_
#define _PROCMAN_MEMMAPS_H_

typedef struct _MemmapsInfo MemmapsInfo;

enum
{
	COL_FILENAME,
	COL_VMSTART,
	COL_VMEND,
	COL_VMSZ,
	COL_FLAGS,
	COL_VMOFFSET,
	COL_DEVICE,
	COL_INODE,
	NUM_MMAP_COL
};

struct _MemmapsInfo
{
	gchar                   *filename;
        gchar			*vmstart;
        gchar			*vmend;
        gchar			*vmsize;
        gchar			*flags;
        gchar			*vmoffset;
        gchar			*device;
        gchar			*inode;
};

void 		create_memmaps_dialog (ProcData *procdata);

#endif /* _PROCMAN_MEMMAPS_H_ */
