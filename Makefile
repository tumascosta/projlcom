PROG = projeto
SRCS = proj.c video.c mouse.c kbc.c timer.c utils.c

DPADD += ${LIBLCF}
LDADD += -llcf

.include <minix.lcom.mk>