OBLIVCC=../../../bin/oblivcc -D _Float128=double
RUNTIME=../../../_build/libobliv.a
PORT=1234
REMOTE_HOST=localhost
CONNECTION = $(REMOTE_HOST):$(PORT)
CILPATH=../../../
CFLAGS=-g -Wall
LOADLIBES=-lm
testName=verify

./a.out: $(testName).oc $(testName).c $(RUNTIME)
	$(OBLIVCC) $(CFLAGS) -I . $(testName).oc $(testName).c $(LOADLIBES)

clean:
	rm -f a.out $(testName).oc.cil.i $(testName).oc.i $(testName).oc.cil.c