include ../Make.rule

BIN = libpb.a
BIN_PATH = .

CPP_FILES +=$(wildcard *.cpp)
C_FILES +=$(wildcard *.cc)
H_FILES += $(wildcard *.h)

INC +=  
LIBS += #-Wl,-soname,libpb.so.1

CFLAGS +=
CXXFLAGS +=

OBJ_DIR = ./obj
$(shell if [ ! -d ${OBJ_DIR} ]; then mkdir ${OBJ_DIR}; fi;)

OBJS := $(patsubst %.cpp, $(OBJ_DIR)/%.o, $(CPP_FILES)) $(patsubst %.cc, $(OBJ_DIR)/%.o, $(C_FILES))

TARGET = $(BIN_PATH)/$(BIN)

$(TARGET):$(OBJS)
	ar -rc -o $@ $(OBJS) 

$(OBJ_DIR)/%.o:%.cc
	$(CC) $(CFLAGS) -c -o $@ $< $(INC)
	
$(OBJ_DIR)/%.o:%.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $< $(INC)

clean:
	rm -f $(OBJS) $(TARGET) *.h *.cc
