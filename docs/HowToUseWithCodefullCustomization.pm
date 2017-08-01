
Add in makefile.wntx64 after
all : avaFoundation_project_all

XFY_BASE_PATH      = $(BASE_PATH)$(PS)src$(PS)server$(PS)YOURSoaDIR$(PS)xfy
XFY_CXXS           = $(XFY_BASE_PATH)\trace\*.c*

Append in 
libYOURSoaDIR\makefile

$(XFY_CXXS)

to

LIBYOURSOADIR_CXXS       = $(LIBYOURSOADIR_SRC)$(PS)*.cxx $(LIBYOURSOADIR_DRV)$(PS)*.cxx $(XFY_CXXS)

e.g.
LIBYOURSOADIR_CXXS       = $(LIBYOURSOADIR_SRC)$(PS)*.cxx $(LIBYOURSOADIR_DRV)$(PS)*.cxx


XFY_BASE_PATH      = $(PROJECT_HOME)$(PS)src$(PS)server$(PS)Avf2SoaFoundation$(PS)xfy
XFY_CXXS           = $(XFY_BASE_PATH)\trace\*.c*


LIBAVF2SOAFOUNDATION_CXXS       = $(LIBAVF2SOAFOUNDATION_SRC)$(PS)*.cxx $(LIBAVF2SOAFOUNDATION_DRV)$(PS)*.cxx $(XFY_CXXS)
