include ../../Make.rule

CPP_FILES += $(wildcard *.cpp)
C_FILES += $(wildcard *.c)
H_FILES += $(wildcard *.h)

PB_FILES += $(wildcard *.pb.cc)

INC +=
LIBS += -L./ -lroom -lgtest
#LIBS = -lgtest -lpthread -L${TOOLS_DIR} -ltools -L${PB_DIR} -lpb -L${LIBPB34_DIR} -lprotobuf -L${COMM_DIR} -lcommon -lglog

OBJ_DIR = ./obj
$(shell if [ ! -d ${OBJ_DIR} ]; then mkdir ${OBJ_DIR}; fi;)

OBJS := $(patsubst %.cpp, ${OBJ_DIR}/%.o, ${CPP_FILES}) $(patsubst %.c, ${OBJ_DIR}/%.o, ${C_FILES})
PB_OBJS := $(patsubst %.cc, ${OBJ_DIR}/%.o, ${PB_FILES})

BIN := $(patsubst %.o, %, $(notdir ${OBJS}))

TARGET := $(foreach proc, ${BIN}, ${TESTS_OUTPUT_PATH}/${proc})

.PHONY: all clean check

all: lib ${TARGET}
	@echo Target... ${BIN}

lib:
	${MAKE} test -C ../

${TARGET}: ${OBJS} ${H_FILES} ${PB_OBJS}
	@echo link... $@
	${CXX} ${CXXFLAGS} -o $@ ${OBJ_DIR}/$(notdir $@).o ${INC} ${LIBS} ${PB_OBJS}

${OBJ_DIR}/%.o:%.c
		${CC} ${CFLAGS} -c -o $@ $< ${INC}

${OBJ_DIR}/%.o:%.cpp
		${CXX} ${CXXFLAGS} -c -o $@ $< ${INC}

${OBJ_DIR}/%.o:%.cc
		${CXX} ${CXXFLAGS} -c -o $@ $< ${INC}
		
clean:
	rm -f ${TARGET} ${OBJS} ${PB_OBJS} libroom.a

check:
#	$(foreach proc, ${TARGET}, valgrind --tool=memcheck --leak-check=full --show-reachable=yes --undef-value-errors=yes --track-fds=yes --track-origins=yes ${proc} && ) true
	$(foreach proc, ${TARGET}, ${proc} && ) true 
