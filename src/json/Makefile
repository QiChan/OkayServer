include ../Make.rule

BIN := libjson.a
BIN_PATH = .

CPP_FILES += $(wildcard *.cpp)
C_FILES += $(wildcard *.c)
H_FILES += $(wildcard *.h)

INC += -I..
LIBS += 

CFLAGS += 
CXXFLAGS += 

OBJ_DIR = ./obj
$(shell if [ ! -d ${OBJ_DIR} ]; then mkdir ${OBJ_DIR}; fi;)

OBJS := $(patsubst %.cpp, ${OBJ_DIR}/%.o, ${CPP_FILES}) $(patsubst %.c, ${OBJ_DIR}/%.o, ${C_FILES})

TARGET = ${BIN_PATH}/${BIN}

${TARGET}:${OBJS}
	ar -rc -o $@ ${OBJS}

${OBJ_DIR}/%.o:%.c
	${CC} ${CFLAGS} -c -o $@ $< ${INC}

${OBJ_DIR}/%.o:%.cpp
	${CXX} ${CXXFLAGS} -c -o $@ $< ${INC}

clean:
	rm -f ${OBJS} ${TARGET}
