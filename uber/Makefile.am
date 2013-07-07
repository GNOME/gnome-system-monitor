
lib_LTLIBRARIES=libuber-1.0.la

INST_H_FILES =				\
	uber.h			\
	uber-graph.h		\
	uber-frame-source.h	\
	uber-heat-map.h		\
	uber-line-graph.h		\
	uber-label.h		\
	uber-range.h		\
	uber-scale.h		\
	uber-scatter.h		\
	uber-timeout-interval.h

NOINST_H_FILES =			\
	uber-window.h		\
	g-ring.h

libuber_1_0_la_SOURCES = 		\
	$(INST_H_FILES)			\
	$(NOINST_H_FILES)		\
	uber-graph.c		\
	uber-frame-source.c	\
	uber-heat-map.c		\
	uber-line-graph.c		\
	uber-label.c		\
	uber-range.c		\
	uber-scale.c		\
	uber-scatter.c		\
	uber-timeout-interval.c	\
	uber-window.c		\
	g-ring.c

libuber_1_0_la_CFLAGS =
libuber_1_0_la_CFLAGS += $(UBER_CFLAGS)

libuber_1_0_la_LIBADD =
libuber_1_0_la_LIBADD += $(UBER_LIBS)
