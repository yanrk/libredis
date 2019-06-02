# arguments
platform = linux/x64



project_home       = ..
build_dir          = $(project_home)/sln
bin_dir            = $(project_home)/lib/$(platform)
object_dir         = $(project_home)/.objs
hiredis_home       = $(project_home)/../tools/gnu_libs/hiredis



# includes of hiredis headers
hiredis_inc_path   = $(hiredis_home)/inc
hiredis_includes   = -I$(hiredis_inc_path)

# includes of libredis headers
libredis_inc_path  = $(project_home)/inc
libredis_includes  = -I$(libredis_inc_path)



# all includes that libredis solution needs
includes           = $(hiredis_includes)
includes          += $(libredis_includes)



# source files of libredis solution

# source files of libredis project
libredis_src_path  = $(project_home)/src
libredis_source    = $(filter %.cpp, $(shell find $(libredis_src_path) -depth -name "*.cpp"))



# objects of libredis solution
libredis_objects   = $(libredis_source:$(project_home)%.cpp=$(object_dir)%.o)



# output librarys
libredis_output_lib= $(bin_dir)/libredis.a



# build flags for objects
build_obj_flags    = -std=c++11 -g -Wall -O1 -pipe -fPIC

# build flags for execution
build_exec_flags   = $(build_obj_flags)

# build flags for dynamic link librarys
build_so_flags     = -std=c++11 -shared

# build flags for static link librarys
build_a_flags      = -rv



# build targets
targets            = redis

# let 'build' be default target, build all targets
build    : $(targets)

redis    : $(libredis_objects)
	@echo
	@echo "@@@@@  start making libredis  @@@@@"
	ar $(build_a_flags) $(libredis_output_lib) $^
	@echo "@@@@@  make libredis success  @@@@@"
	@echo

# build all objects
$(object_dir)/%.o:$(project_home)/%.cpp
	@dir=`dirname $@`;      \
    if [ ! -d $$dir ]; then \
        mkdir -p $$dir;     \
    fi
	g++ -c $(build_obj_flags) $(includes) -o $@ $<

clean    :
	rm -rf $(object_dir) $(bin_dir)/lib*

rebuild  : clean build
