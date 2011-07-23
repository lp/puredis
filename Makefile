#######################################################################
# User configuration goes here

# Uncomment the particular line according to your platform
#PLATFORM = linux
PLATFORM = macosx
#PLATFORM = mingw

# If Pd is in an unusual place, put -I/path/to m_pd.h here:
PDINCLUDE = -I/Applications/Pd-extended.app/Contents/Resources/include
#PDINCLUDE = -I./
#PDINCLUDE = -I$(HOME)/include
#PDINCLUDE = -I/opt/puredata/include
#PDINCLUDE = -I/Applications/Pd-0.41-4.app/Contents/Resources/src

# build architecture: comment 2 lines to build in default 64bit on 64 bit platforms
ARCH = -arch i386
HIREDISARCH = 32bit

# End of user configuration
#######################################################################

# which Pd external to build
puredis_src = src/puredis.c
puredis_linux = src/puredis.pd_linux
puredis_macosx = src/puredis.pd_darwin
puredis_mingw = src/puredis.dll

# how to build a Pd external
# FIXME: Windows?
CFLAGS_linux = -shared
LIBS_linux =
CFLAGS_macosx = -bundle -undefined suppress -flat_namespace -arch i386
LIBS_macosx =
CFLAGS_mingw = -shared -DMSW
LIBS_mingw = pd.dll

# Hiredis setup
HIREDIS = hiredis
HIREDISD = antirez-hiredis-3cc6a7f
HIREDISTGZ = antirez-hiredis-v0.10.1-0-g3cc6a7f.tar.gz
HIREDISURL = https://github.com/antirez/hiredis/tarball/v0.10.1
HIREDISI = -I$(HIREDISD)
HIREDISL = $(HIREDISD)/libhiredis.a

CFLAGS = -ansi -Wall -O2 -fPIC -bundle -undefined suppress -flat_namespace $(ARCH) $(HIREDISI) $(PDINCLUDE)

# build
default: $(puredis_$(PLATFORM))

# clean up
clean:
	rm -f $(puredis_$(PLATFORM))

# install
install:
	echo "TODO"

# compile
$(puredis_$(PLATFORM)): $(puredis_src) $(HIREDISD)/build.stamp
	gcc $(CFLAGS) $(CFLAGS_$(PLATFORM)) -o $(puredis_$(PLATFORM)) $(puredis_src) $(HIREDISL) $(LIBS_$(PLATFORM))

# get hiredis: download, unpack, compile, install locally
$(HIREDISD)/build.stamp: $(HIREDISD)/unpack.stamp
	make -C $(HIREDISD) $(HIREDISARCH)
	touch $(HIREDISD)/build.stamp

$(HIREDISD)/unpack.stamp: $(HIREDISTGZ)
	tar xzf $(HIREDISTGZ)
	touch $(HIREDISD)/unpack.stamp

$(HIREDISTGZ):
	wget $(HIREDISURL)
