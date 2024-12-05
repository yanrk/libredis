# arguments
platform = centos



project_home       = .
build_dir          = $(project_home)
bin_dir            = $(project_home)/bin/$(platform)
object_dir         = $(project_home)/.objs
libredis_home      = $(project_home)/..
hiredis_home       = $(libredis_home)/../gnu_libs/hiredis



# includes of libredis headers
libredis_inc_path  = $(libredis_home)/inc
libredis_includes  = -I$(libredis_inc_path)



# all includes that test solution needs
includes           = $(libredis_includes)



# source files of test solution

# source files of test project
test_src_path      = $(project_home)
test_source        = $(filter %.cpp, $(shell find $(test_src_path) -depth -name "*.cpp"))



# objects of test solution
test_objects       = $(test_source:$(project_home)%.cpp=$(object_dir)%.o)



# hiredis librarys
hiredis_lib_inc    = $(hiredis_home)/lib/$(platform)
hiredis_libs       = -L$(hiredis_lib_inc) -lhiredis

# libredis librarys
libredis_lib_inc   = $(libredis_home)/lib/$(platform)
libredis_libs      = -L$(libredis_lib_inc) -lredis

# test depends librarys
depend_libs        = $(libredis_libs)
depend_libs       += $(hiredis_libs)

# output execute
output_exec        = $(bin_dir)/test



# build flags for objects
build_obj_flags    = -std=c++11 -g -Wall -O1 -pipe -fPIC

# build flags for execution
build_exec_flags   = $(build_obj_flags)



# build targets
targets            = test

# let 'build' be default target, build all targets
build    : $(targets)

test     : $(test_objects)
	mkdir -p $(bin_dir)
	@echo "includes is $(includes)"
	@echo "@@@@@  start making test  @@@@@"
	g++ $(build_exec_flags) -o $(output_exec) $^ $(depend_libs)
	@echo "@@@@@  make test success  @@@@@"
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
