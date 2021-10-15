CXX      := -g++ -Wwrite-strings -pthread 
BUILD    := ./build
INCLUDE  := -I include/
OBJ_DIR  := obj
APP_DIR  := bin

# WebCliente
TARGET_0 := WebCliente
SRC_0    := src/WebCliente.cpp src/Socket.cpp src/FileSystem.cpp src/Utilidades.cpp
OBJ_0    := $(SRC_0:%.cpp=$(OBJ_DIR)/%.o)

# WebServidor
TARGET_1 := WebServidor
SRC_1    := src/WebServidor.cpp src/Socket.cpp src/InterpreteHTTP.cpp src/FileSystem.cpp src/Utilidades.cpp
OBJ_1    := $(SRC_1:%.cpp=$(OBJ_DIR)/%.o)

# Balanceador
TARGET_2 := Balanceador
SRC_2    := src/Balanceador.cpp src/Socket.cpp src/InterpreteHTTP.cpp src/FileSystem.cpp src/Utilidades.cpp
OBJ_2    := $(SRC_2:%.cpp=$(OBJ_DIR)/%.o)

WebCliente: build $(APP_DIR)/$(TARGET_0)

$(OBJ_DIR)/%.o: %.cpp
	@mkdir -p $(@D)
	$(CXX) $(INCLUDE) -c $< -o $@ 

$(APP_DIR)/$(TARGET_0): $(OBJ_0)
	@mkdir -p $(@D)
	$(CXX) -o $(APP_DIR)/$(TARGET_0) $^ 

WebServidor: build $(APP_DIR)/$(TARGET_1)

$(OBJ_DIR)/%.o: %.cpp
	@mkdir -p $(@D)
	$(CXX) $(INCLUDE) -c $< -o $@ 

$(APP_DIR)/$(TARGET_1): $(OBJ_1)
	@mkdir -p $(@D)
	$(CXX) -o $(APP_DIR)/$(TARGET_1) $^ 

Balanceador: build $(APP_DIR)/$(TARGET_2)

$(OBJ_DIR)/%.o: %.cpp
	@mkdir -p $(@D)
	$(CXX) $(INCLUDE) -c $< -o $@ 

$(APP_DIR)/$(TARGET_2): $(OBJ_2)
	@mkdir -p $(@D)
	$(CXX) -o $(APP_DIR)/$(TARGET_2) $^ 

# Makefile
build:
	@mkdir -p $(APP_DIR)
	@mkdir -p $(OBJ_DIR)

debug: CXXFLAGS += -g -DDEBUG 
debug: all

release: CXXFLAGS += -O2
release: all

clean:
	-@rm -rvf $(OBJ_DIR)/*