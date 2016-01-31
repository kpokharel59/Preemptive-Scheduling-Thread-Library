all:
	gcc mythread.c -c
	ar rc libmythread.a mythread.o
	ranlib libmythread.a
