include Make.rule

CODE_DIRS = log			\
			main		\
			monitor		\
			master		\
			persistence	\
			watchdog	\
			agent		\
			roomrouter	\
			roomsvr		\
			room		\
			roomlist	\
			value		\
			dbhelper	\
			club		\
			mail

LIB_DIRS = common	\
		   pb		\
		   json		\
		   tools	\
		   aliyunlog

TEST_DIRS = tools/tests		\
			common/tests	\
			room/tests		

ROBOT_DIRS = robotwatchdog	\
			 robot

CODE_PATH = $(foreach path, ${CODE_DIRS}, ${SRC_PATH}/${path})
LIB_PATH = $(foreach path, ${LIB_DIRS}, ${SRC_PATH}/${path})
TEST_PATH = $(foreach path, ${TEST_DIRS}, ${SRC_PATH}/${path})
ROBOT_PATH = $(foreach path, ${ROBOT_DIRS}, ${SRC_PATH}/${path})

.PHONY: all lib 

all: lib
	$(foreach src, ${CODE_PATH}, ${MAKE} -C ${src} && ) true

test: lib
	$(foreach src, ${TEST_PATH}, ${MAKE} -C ${src} && ) true

robot: lib
	$(foreach src, ${ROBOT_PATH}, ${MAKE} -C ${src} && ) true

lib: proto
	$(foreach src, ${LIB_PATH}, ${MAKE} -C ${src} && ) true

clean:
	$(foreach src, ${CODE_PATH}, ${MAKE} -C ${src} clean && ) true
	$(foreach src, ${LIB_PATH}, ${MAKE} -C ${src} clean && ) true
	$(foreach src, ${TEST_PATH}, ${MAKE} -C ${src} clean && ) true
	$(foreach src, ${ROBOT_PATH}, ${MAKE} -C ${src} clean && ) true

proto:
	cd ./pb/proto/ && ./protoc.sh ../../../common/proto

check:
	$(foreach src, ${TEST_PATH}, ${MAKE} -C ${src} check && ) true

md5:
	ls ${OUTPUT_PATH}/*.so | xargs md5sum > ${OUTPUT_PATH}/md5_check
