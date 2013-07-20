pkglibexec_PROGRAMS = gsm-renice gsm-kill

AM_CPPFLAGS = \
	$(SCRIPTS_CFLAGS)

gsm_renice_SOURCES = gsm_execute_helper.c
gsm_renice_LDADD = $(SCRIPTS_LIBS)
gsm_renice_CFLAGS = -DCOMMAND=\"renice\"

gsm_kill_SOURCES = gsm_execute_helper.c
gsm_kill_LDADD = $(SCRIPTS_LIBS)
gsm_kill_CFLAGS = -DCOMMAND=\"kill\"
