
#ifndef _MEMMAPS_H_
#define _MEMMAPS_H_


#define MEMMAPSSPEC "<ETableSpecification cursor-mode=\"line\" selection-mode=\"single\" draw-focus=\"true\">
  <ETableColumn model_col=\"0\" _title=\"Filename\"   expansion=\"2.300562\" minimum_width=\"20\" resizable=\"true\" cell=\"string\" compare=\"string\"/> \
  <ETableColumn model_col=\"1\" _title=\"VM Start\"   expansion=\"0.783240\" minimum_width=\"20\" resizable=\"true\" cell=\"string\" compare=\"string\"/> \
  <ETableColumn model_col=\"2\" _title=\"VM End\"   expansion=\"0.783240\" minimum_width=\"20\" resizable=\"true\" cell=\"string\" compare=\"string\"/> \
  <ETableColumn model_col=\"3\" _title=\"Flags\"   expansion=\"0.783240\" minimum_width=\"20\" resizable=\"true\" cell=\"string\" compare=\"string\"/> \
  <ETableColumn model_col=\"4\" _title=\"VM offset\"   expansion=\"0.783240\" minimum_width=\"20\" resizable=\"true\" cell=\"string\" compare=\"string\"/> \
  <ETableColumn model_col=\"5\" _title=\"Device\"   expansion=\"0.783240\" minimum_width=\"20\" resizable=\"true\" cell=\"string\" compare=\"string\"/> \
  <ETableColumn model_col=\"6\" _title=\"Inode\"   expansion=\"0.783240\" minimum_width=\"20\" resizable=\"true\" cell=\"string\" compare=\"string\"/> \
        <ETableState> \
        	<column source=\"0\"/> \
        	<column source=\"1\"/> \
        	<column source=\"2\"/> \
        	<column source=\"3\"/> \
        	<column source=\"4\"/> \
        	<column source=\"5\"/> \
        	<column source=\"6\"/> \
        <grouping></grouping> \
        </ETableState> \
</ETableSpecification>"       

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

void		update_memmaps_dialog (ProcData *procdata);
void 		create_memmaps_dialog (ProcData *procdata);


#endif
