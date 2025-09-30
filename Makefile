CXX=g++
DEV_FLAGS = -std=c++23 -g -O1 -MD -MP  -DWLR_USE_UNSTABLE -fsanitize=address,undefined

PKGS =   wayland-server libinput xkbcommon wlroots-0.19

INCLUDES =  -Iinclude -Iinclude/wayland-protocols -Iinclude/inputs -Iinclude/memory_reader `pkg-config --cflags $(PKGS)`
LINCLUDES = `pkg-config --libs $(PKGS)`

CPPFILES = $(shell find * -type f -name "*.cpp" -not -path '*/.*')

OFILES = $(patsubst src/%.cpp, build/interrim/%.o, $(CPPFILES))
DFILES = $(patsubst src/%.cpp, build/interrim/%.d, $(CPPFILES))

BIN = build/bin/programController

files:
	@echo "OFILES: $(OFILES)"
	@echo "DFILES: $(DFILES)"
	@echo "CPPFILES: $(CPPFILES)"
	@echo "package includes: $(INCLUDES)" 
	@echo "package link includes: $(LINCLUDES)" 


all: dirs $(BIN)

$(BIN): $(OFILES) $(LOFILES)
	@echo "Compiling '$^' into $@"
	$(CXX) $(DEV_FLAGS) -o $(BIN) $^  $(LINCLUDES)

build/interrim/%.o: src/%.cpp
	@echo "Compiling '$<' into '$@'"
	$(CXX) $(DEV_FLAGS)  -o $@ $< -c $(INCLUDES) $(LINCLUDES)

dirs:
	mkdir -p build/bin build/interrim build/interrim/inputs build/interrim/memory_reader


clean:
	rm -rf build

-include $(DFILES)
