AM_CFLAGS =					\
	$(GROONGA_CFLAGS)

AM_LDFLAGS =					\
	-avoid-version				\
	-module					\
	-no-undefined

LIBS =						\
	$(GROONGA_LIBS)

commands_plugins_LTLIBRARIES =
commands_plugins_LTLIBRARIES += tag_synonym.la

commands_plugins_SOURCE =			\
	tag_synonym.c
