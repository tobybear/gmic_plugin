PLUGINNAME = gmic_ofx
PLUGINOBJECTS = ../gmic_plugin.o

SRCDIR = ../../..
PATHTOROOT = $(SRCDIR)/ThirdParty/OpenFX/Examples
include Makefile.master

RESOURCES = gmic_ofx.png

CXXFLAGS += \
-I$(SRCDIR)/Helpers \
-I$(SRCDIR)/ThirdParty/GMIC/src \
-I$(SRCDIR)/ThirdParty/OpenFX/include \
-I$(SRCDIR)/ThirdParty/OpenFX/Examples/include

LINKFLAGS += \
-lgmicstatic \
-L$(SRCDIR)/ThirdParty/GMIC/src

VPATH += \
$(SRCDIR)/ThirdParty/GMIC \
$(SRCDIR)/Helpers
