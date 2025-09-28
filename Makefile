CXX=g++
DEV_FLAGS = -std=c++23 -g -O1 -MD -MP  -DWLR_USE_UNSTABLE -fsanitize=address,undefined

PKGS =   wayland-server libinput xkbcommon wlroots-0.19

INCLUDES =  -Iinclude -Iinclude/inputs -Iinclude/wayland-protocols `pkg-config --cflags $(PKGS)`
LINCLUDES = `pkg-config --libs $(PKGS)`

CPPFILES = $(wildcard src/*.cpp)
LIBFILES = $(shell find lib -type f -name '*.cpp')

OFILES = $(patsubst src/%.cpp, build/interrim/%.o, $(CPPFILES))
LOFILES = $(patsubst lib/%.cpp, build/interrim/lib/%.o, $(LIBFILES))
DFILES = $(patsubst src/%.cpp, build/interrim/%.d, $(CPPFILES))
LDFILES = $(patsubst lib/%.cpp, build/interrim/lib/%.d, $(LIBFILES))

BIN = build/bin/programController

files:
	@echo "OFILES: $(OFILES)"
	@echo "LOFILES: $(LOFILES)"
	@echo "DFILES: $(DFILES)"
	@echo "LDFILES: $(LDFILES)"
	@echo "CPPFILES: $(CPPFILES)"
	@echo "LIBFILES: $(LIBFILES)"
	@echo "package includes: $(INCLUDES)" 
	@echo "package link includes: $(LINCLUDES)" 


all: dirs $(BIN)

$(BIN): $(OFILES) $(LOFILES)
	@echo "Compiling '$^' into $@"
	$(CXX) $(DEV_FLAGS) -o $(BIN) $^  $(LINCLUDES)

build/interrim/%.o: src/%.cpp
	@echo "Compiling '$<' into '$@'"
	$(CXX) $(DEV_FLAGS)  -o $@ $< -c $(INCLUDES) $(LINCLUDES)

build/interrim/lib/%.o: lib/%.cpp
	@echo "Compiling '$<' into '$@'"
	$(CXX) $(DEV_FLAGS)  -o $@ $< -c $(INCLUDES) $(LINCLUDES)

dirs:
	mkdir -p build/bin build/interrim build/interrim/lib/inputs


clean:
	rm -rf build

-include $(DFILES) $(LDFILES)
