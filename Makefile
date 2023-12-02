##==========================================
## partial Makefile provided to students
##

CFLAGS = -std=c11 -Wall -Wpedantic -g

# a bit more checks if you'd like to (uncomment
# CFLAGS += -Wextra -Wfloat-equal -Wshadow                         \
# -Wpointer-arith -Wbad-function-cast -Wcast-align -Wwrite-strings \
# -Wconversion -Wunreachable-code

# uncomment if you want to add DEBUG flag
# CPPFLAGS += -DDEBUG

# ---------------------------------------------------------------------- 
# feel free to update/modifiy this part as you wish

# all those libs are required on Debian, feel free to adapt it to your box

LDLIBS += -lcheck -lm -lrt -pthread -lsubunit

all:: test-tlb_hrchy
	gcc -g -std=c11 -MM *.c $(LDLIBS) -DDEBUG


test-tlb_hrchy: test-tlb_hrchy.o tests.h error.h util.h addr_mng.h commands.h memory.h  tlb_hrchy_mng.h tlb_hrchy.h addr.h tlb_hrchy_mng.o commands.o error.o addr_mng.o  list.o memory.o page_walk.o

addr_mng.o: addr_mng.c addr_mng.h addr.h error.h 
commands.o: commands.c commands.h mem_access.h addr.h error.h addr_mng.h
error.o: error.c error.h
list.o: list.c list.h error.h
page_walk.o: page_walk.c page_walk.h addr_mng.h error.h memory.h
memory.o: memory.c memory.h page_walk.h addr_mng.h util.h error.h
tlb_hrchy_mng.o:tlb_hrchy_mng.c tlb_hrchy_mng.h tlb_hrchy.h addr.h page_walk.h memory.h addr_mng.h error.h
test-tlb_hrchy.o:test-tlb_hrchy.c error.h util.h addr_mng.h commands.h memory.h tlb_hrchy.h tlb_hrchy_mng.h 


# ----------------------------------------------------------------------
# This part is to make your life easier. See handouts how to make use of it.

clean::
	-@/bin/rm -f *.o *~ $(CHECK_TARGETS)

new: clean all

static-check:
	scan-build -analyze-headers --status-bugs -maxloop 64 make CC=clang new

style:
	astyle -n -o -A8 *.[ch]

# all those libs are required on Debian, adapt to your box
$(CHECK_TARGETS): LDLIBS += -lcheck -lm -lrt -pthread -lsubunit

check:: $(CHECK_TARGETS)
$(foreach target,$(CHECK_TARGETS),./$(target);)

# target to run tests
check:: all
	@if ls tests/*.*.sh 1> /dev/null 2>&1; then \
      for file in tests/*.*.sh; do [ -x $$file ] || echo "Launching $$file"; ./$$file || exit 1; done; \
    fi

IMAGE=arashpz/feedback
feedback:
	@docker run -it --rm -v ${PWD}:/home/tester/done $(IMAGE)

SUBMIT_SCRIPT=../provided/submit.sh
submit1: $(SUBMIT_SCRIPT)
	@$(SUBMIT_SCRIPT) 1

submit2: $(SUBMIT_SCRIPT)
	@$(SUBMIT_SCRIPT) 2

submit:
	@printf 'what "make submit"??\nIt'\''s either "make submit1" or "make submit2"...\n'

