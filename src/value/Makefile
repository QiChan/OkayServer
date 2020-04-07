include ../Make.rule

BIN := value.so
BIN_PATH = ${OUTPUT_PATH}

CPP_FILES += $(wildcard *.cpp)
C_FILES += $(wildcard *.c)
H_FILES += $(wildcard *.h)

INC += 
LIBS += -L${LIBMYSQL_DIR} -lmysqlclient

CFLAGS += --share
CXXFLAGS += --share

OBJ_DIR = ./obj
$(shell if [ ! -d ${OBJ_DIR} ]; then mkdir ${OBJ_DIR}; fi;)

OBJS := $(patsubst %.cpp, ${OBJ_DIR}/%.o, ${CPP_FILES}) $(patsubst %.c, ${OBJ_DIR}/%.o, ${C_FILES})

TARGET = ${BIN_PATH}/${BIN}
TEST_TARGET := tests/libvalue.a

${TARGET}:${OBJS}
	@echo link... ${TARGET}
	${CXX} ${CXXFLAGS} -o $@ ${OBJS} ${INC} ${LIBS}

${OBJ_DIR}/%.o:%.c
	${CC} ${CFLAGS} -c -o $@ $< ${INC}

${OBJ_DIR}/%.o:%.cpp
	${CXX} ${CXXFLAGS} -c -o $@ $< ${INC}

test:${OBJS}
	ar -rc -o ${TEST_TARGET} ${OBJS}

clean:
	rm -f ${OBJS} ${TARGET}
