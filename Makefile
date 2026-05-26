PROG = Tomb of MINIX

SRCS = proj.c lab5/video.c lab4/mouse.c kbc.c timer.c

CFLAGS += -I. -I./lab5 -I./lab4 -Wall

.include <minix.lcom.mk>