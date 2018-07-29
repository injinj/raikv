# defines a directory for build, for example, RH6_x86_64
lsb_dist     := $(shell if [ -x /usr/bin/lsb_release ] ; then lsb_release -is ; else echo Linux ; fi)
lsb_dist_ver := $(shell if [ -x /usr/bin/lsb_release ] ; then lsb_release -rs | sed 's/[.].*//' ; fi)
uname_m      := $(shell uname -m)

short_dist_lc := $(patsubst CentOS,rh,$(patsubst RedHat,rh,\
                   $(patsubst Fedora,fc,$(patsubst Ubuntu,ub,\
		     $(patsubst Debian,deb,$(patsubst SUSE,ss,$(lsb_dist)))))))
short_dist    := $(shell echo $(short_dist_lc) | tr a-z A-Z)
rpm_os        := $(short_dist_lc)$(lsb_dist_ver).$(uname_m)

# this is where the targets are compiled
build_dir ?= $(short_dist)$(lsb_dist_ver)_$(uname_m)$(port_extra)
bind      := $(build_dir)/bin
libd      := $(build_dir)/lib64
objd      := $(build_dir)/obj
dependd   := $(build_dir)/dep

# use 'make port_extra=-g' for debug build
ifeq (-g,$(findstring -g,$(port_extra)))
  DEBUG = true
endif

# the compiler and linker
CC          ?= gcc
CXX         ?= g++
cc          := $(CC)
cpp         := $(CXX)
cppflags    := -fno-rtti -fno-exceptions
arch_cflags := -march=corei7-avx -fno-omit-frame-pointer
cpplink     := gcc
gcc_wflags  := -Wall -Wextra -Werror -pedantic
fpicflags   := -fPIC
soflag      := -shared

ifdef DEBUG
default_cflags := -ggdb
else
default_cflags := -ggdb -O3
endif
CFLAGS ?= $(default_cflags)
# rpmbuild uses RPM_OPT_FLAGS, which uses the -fstack-protector-strong flag
#RPM_OPT_FLAGS ?= $(default_cflags)
#CFLAGS ?= $(RPM_OPT_FLAGS)
cflags := $(gcc_wflags) $(CFLAGS) $(arch_cflags)

# where to find the raikv/xyz.h files
INCLUDES    ?= -Iinclude
includes    := $(INCLUDES)
DEFINES     ?=
defines     := $(DEFINES)
#cpp_lnk     := -lsupc++
sock_lib    :=
math_lib    := -lm
thread_lib  := -pthread -lrt
malloc_lib  :=
#dynlink_lib := -ldl

# targets filled in below
all_exes    :=
all_libs    :=
all_dlls    :=
all_depends :=
major_num   := 1
minor_num   := 0
patch_num   := 0
build_num   := 32
version     := $(major_num).$(minor_num).$(patch_num)
ver_build   := $(version)-$(build_num)
defines     += -DKV_VER=$(ver_build)

# first target everything, target all: is at the end, after all_* are defined
.PHONY: everything
everything: all

libraikv_files := key_ctx ht_linear ht_cuckoo key_hash msg_ctx ht_stats \
                  ht_init scratch_mem util rela_ts radix_sort print
libraikv_objs  := $(addprefix $(objd)/, $(addsuffix .o, $(libraikv_files)))
libraikv_dbjs  := $(addprefix $(objd)/, $(addsuffix .fpic.o, $(libraikv_files)))
libraikv_deps  := $(addprefix $(dependd)/, $(addsuffix .d, $(libraikv_files))) \
                  $(addprefix $(dependd)/, $(addsuffix .fpic.d, $(libraikv_files)))
libraikv_spec  := $(version)-$(build_num)
libraikv_ver   := $(major_num).$(minor_num)

$(libd)/libraikv.a: $(libraikv_objs)
$(libd)/libraikv.so: $(libraikv_dbjs)

all_libs    += $(libd)/libraikv.a $(libd)/libraikv.so
all_depends += $(libraikv_deps)

kv_test_objs = $(objd)/test.o
kv_test_deps = $(dependd)/test.d
kv_test_libs = $(libd)/libraikv.a
#kv_test_lnk  = -lraikv
kv_test_lnk  = $(kv_test_libs)

$(bind)/kv_test: $(kv_test_objs) $(kv_test_libs)

hash_test_objs = $(objd)/hash_test.o
hash_test_deps = $(dependd)/hash_test.d
hash_test_libs = $(libd)/libraikv.so
hash_test_lnk  = -lraikv

$(bind)/hash_test: $(hash_test_objs) $(hash_test_libs)

ping_objs = $(objd)/ping.o
ping_deps = $(dependd)/ping.d
ping_libs = $(libd)/libraikv.so
ping_lnk  = -lraikv

$(bind)/ping: $(ping_objs) $(ping_libs)

kv_cli_objs = $(objd)/cli.o
kv_cli_deps = $(dependd)/cli.d
kv_cli_libs = $(libd)/libraikv.so
kv_cli_lnk  = -lraikv

$(bind)/kv_cli: $(kv_cli_objs) $(kv_cli_libs)

mcs_test_objs = $(objd)/mcs_test.o
mcs_test_deps = $(dependd)/mcs_test.d
mcs_test_libs = $(libd)/libraikv.so
mcs_test_lnk  = -lraikv

$(bind)/mcs_test: $(mcs_test_objs) $(mcs_test_libs)

kv_server_objs = $(objd)/server.o
kv_server_deps = $(dependd)/server.d
kv_server_libs = $(libd)/libraikv.so
kv_server_lnk  = -lraikv

$(bind)/kv_server: $(kv_server_objs) $(kv_server_libs)

load_objs = $(objd)/load.o
load_deps = $(dependd)/load.d
load_libs = $(libd)/libraikv.so
load_lnk  = -lraikv

$(bind)/load: $(load_objs) $(load_libs)

ctest_objs = $(objd)/ctest.o
ctest_deps = $(dependd)/ctest.d
ctest_libs = $(libd)/libraikv.so
ctest_lnk  = -lraikv

$(bind)/ctest: $(ctest_objs) $(ctest_libs)

rela_test_objs = $(objd)/rela_test.o
rela_test_deps = $(dependd)/rela_test.d
rela_test_libs = $(libd)/libraikv.so
rela_test_lnk  = -lraikv

$(bind)/rela_test: $(rela_test_objs) $(rela_test_libs)

pq_test_objs = $(objd)/pq_test.o
pq_test_deps = $(dependd)/pq_test.d

$(bind)/pq_test: $(pq_test_objs) $(pq_test_libs)

all_exes    += $(bind)/kv_test $(bind)/hash_test $(bind)/ping \
               $(bind)/kv_cli $(bind)/mcs_test $(bind)/kv_server \
	       $(bind)/load $(bind)/rela_test $(bind)/ctest \
	       $(bind)/pq_test
all_depends += $(kv_test_deps) $(hash_test_deps) $(ping_deps) \
               $(kv_cli_deps) $(mcs_test_deps) $(kv_server_deps) \
	       $(load_deps) $(rela_test_deps) $(ctest_deps) \
	       $(pq_test_deps)

all_dirs := $(bind) $(libd) $(objd) $(dependd)

# the default targets
.PHONY: all
all: $(all_libs) $(all_dlls) $(all_exes)

# create directories
$(dependd):
	@mkdir -p $(all_dirs)

# remove target bins, objs, depends
.PHONY: clean
clean:
	rm -r -f $(bind) $(libd) $(objd) $(dependd)
	if [ "$(build_dir)" != "." ] ; then rmdir $(build_dir) ; fi

# force a remake of depend using 'make -B depend'
.PHONY: depend
depend: $(dependd)/depend.make

$(dependd)/depend.make: $(dependd) $(all_depends)
	@echo "# depend file" > $(dependd)/depend.make
	@cat $(all_depends) >> $(dependd)/depend.make

.PHONY: dist_bins
dist_bins: $(all_libs) $(all_dlls) $(bind)/kv_cli $(bind)/kv_server $(bind)/kv_test

.PHONY: dist_rpm
dist_rpm:
	mkdir -p rpmbuild/{RPMS,SRPMS,BUILD,SOURCES,SPECS}
	sed -e "s/99999/${build_num}/" \
	    -e "s/999.999/${version}/" < rpm/raikv.spec > rpmbuild/SPECS/raikv.spec
	mkdir -p rpmbuild/SOURCES/raikv-${version}
	ln -sf ../../../src ../../../test ../../../include ../../../GNUmakefile rpmbuild/SOURCES/raikv-${version}/
	( cd rpmbuild/SOURCES && tar chzf raikv-${ver_build}.tar.gz --exclude=".*.sw*" raikv-${version} && rm -r -f raikv-${version} )
	( cd rpmbuild && rpmbuild --define "-topdir `pwd`" -ba SPECS/raikv.spec )

.PHONY: local_repo_update
local_repo_update: dist_rpm
	createrepo --update `pwd`/rpmbuild/RPMS/${uname_m}
	sudo dnf -y update raikv
	sudo dnf -y debuginfo-install raikv-debuginfo
	sudo dnf -y update raikv-debugsource

.PHONY: local_repo_create
local_repo_create: dist_rpm
	createrepo -v `pwd`/rpmbuild/RPMS/${uname_m}
	@echo "# Create this file: /etc/yum.repos.d/raikv.repo"
	@echo "[raikv]"
	@echo "name=My Local Repository"
	@echo "baseurl=file://`pwd`/rpmbuild/RPMS/${uname_m}"
	@echo "metadata_expire=1"
	@echo "gpgcheck=0"
	@echo "enabled=1"
	@echo "# Use this to install:"
	@echo "sudo dnf -y install raikv"
	@echo "sudo dnf -y debuginfo-install raikv-debuginfo"
	@echo "sudo dnf -y install raikv-debugsource"

# dependencies made by 'make depend'
-include $(dependd)/depend.make

$(objd)/%.o: src/%.cpp
	$(cpp) $(cflags) $(cppflags) $(includes) $(defines) $($(notdir $*)_includes) $($(notdir $*)_defines) -c $< -o $@

$(objd)/%.o: src/%.c
	$(cc) $(cflags) $(includes) $(defines) $($(notdir $*)_includes) $($(notdir $*)_defines) -c $< -o $@

$(objd)/%.fpic.o: src/%.cpp
	$(cpp) $(cflags) $(fpicflags) $(cppflags) $(includes) $(defines) $($(notdir $*)_includes) $($(notdir $*)_defines) -c $< -o $@

$(objd)/%.fpic.o: src/%.c
	$(cc) $(cflags) $(fpicflags) $(includes) $(defines) $($(notdir $*)_includes) $($(notdir $*)_defines) -c $< -o $@

$(objd)/%.o: test/%.cpp
	$(cpp) $(cflags) $(cppflags) $(includes) $(defines) $($(notdir $*)_includes) $($(notdir $*)_defines) -c $< -o $@

$(objd)/%.o: test/%.c
	$(cc) $(cflags) $(includes) $(defines) $($(notdir $*)_includes) $($(notdir $*)_defines) -c $< -o $@

$(libd)/%.a:
	ar rc $@ $($(*)_objs)

$(libd)/%.so:
	$(cpplink) $(soflag) $(cflags) -o $@.$($(*)_spec) -Wl,-soname=$(@F).$($(*)_ver) $($(*)_dbjs) $($(*)_dlnk) $(cpp_dll_lnk) $(sock_lib) $(math_lib) $(thread_lib) $(malloc_lib) $(dynlink_lib) && \
	cd $(libd) && ln -f -s $(@F).$($(*)_spec) $(@F).$($(*)_ver) && ln -f -s $(@F).$($(*)_ver) $(@F)

$(bind)/%:
	$(cpplink) $(cflags) -o $@ $($(*)_objs) -L$(libd) $($(*)_lnk) $(cpp_lnk) $(sock_lib) $(math_lib) $(thread_lib) $(malloc_lib) $(dynlink_lib)

$(dependd)/%.d: src/%.cpp
	gcc -x c++ $(arch_cflags) $(defines) $(includes) $($(notdir $*)_includes) $($(notdir $*)_defines) -MM $< -MT $(objd)/$(*).o -MF $@

$(dependd)/%.d: src/%.c
	gcc $(arch_cflags) $(defines) $(includes) $($(notdir $*)_includes) $($(notdir $*)_defines) -MM $< -MT $(objd)/$(*).o -MF $@

$(dependd)/%.fpic.d: src/%.cpp
	gcc -x c++ $(arch_cflags) $(defines) $(includes) $($(notdir $*)_includes) $($(notdir $*)_defines) -MM $< -MT $(objd)/$(*).fpic.o -MF $@

$(dependd)/%.fpic.d: src/%.c
	gcc $(arch_cflags) $(defines) $(includes) $($(notdir $*)_includes) $($(notdir $*)_defines) -MM $< -MT $(objd)/$(*).fpic.o -MF $@

$(dependd)/%.d: test/%.cpp
	gcc -x c++ $(arch_cflags) $(defines) $(includes) $($(notdir $*)_includes) $($(notdir $*)_defines) -MM $< -MT $(objd)/$(*).o -MF $@

$(dependd)/%.d: test/%.c
	gcc $(arch_cflags) $(defines) $(includes) $($(notdir $*)_includes) $($(notdir $*)_defines) -MM $< -MT $(objd)/$(*).o -MF $@

