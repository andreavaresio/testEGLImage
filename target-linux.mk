#versione 4 - Andrea

#bisogna definire CFG = Debug | Release | Profiling | Testing altrimenti genera Debug
#bisogna definire PLATFORM = LINUX altrimenti usa LINUX

#----------------------------------------------
#Makefile per librerie dinamiche, statiche e eseguibili
#----------------------------------------------

ifeq ($(CFG),)
CFG=Debug
PRINTCFG = No configuration specified. Defaulting to $(CFG) 
endif 
ifneq ($(CFG),Release)
ifneq ($(CFG),Debug)
ifneq ($(CFG),Profiling)
ifneq ($(CFG),Testing)
error:
	@echo "ERROR An invalid configuration is specified."
	@echo " Possible choices for configuration are:"
	@echo "  Release"
	@echo "  Debug"
	@echo "  Profiling"
	@echo "  Testing"
endif
endif
endif
endif

ifeq ($(PLATFORM),)
PLATFORM=LINUX
PRINTCFG = No platform specified. Defaulting to $(PLATFORM) 
endif 
ifneq ($(PLATFORM),LINUX)
ifneq ($(PLATFORM),RASPBERRY)
error:
	@echo "ERROR An invalid platform is specified."
	@echo " Possible choices for configuration are:"
	@echo "  LINUX"
	@echo "  RASPBERRY"
endif
endif

PRINTCFGh = "***************************************************" 
PRINTCFG  = Using configuration $(CFG) and platform $(PLATFORM)
PRINTCFGt = "***************************************************"

CPP_FLAGS += -D_LINUX_ -D__$(PLATFORM)__ 
CPP_FLAGS += -DUNICODE -D_FILE_OFFSET_BITS=64 -Wno-write-strings 

LD_FLAGS +=

ifeq ($(PLATFORM),LINUX)
CPP = g++
CPP_C = gcc
AR = ar
CPP_FLAGS += 
endif 

ifeq ($(PLATFORM),RASPBERRY)
#percorso del cross-compilatore
CPP = g++
CPP_C = gcc
AR = ar
CPP_FLAGS += 
endif

#================================================================================================
# RELEASE
#================================================================================================
ifeq ($(CFG),Release)
CPP_FLAGS += -O3 -DNDEBUG
LD_FLAGS += -Wl,-s

#================================================================================================
# DEBUG
#================================================================================================
else
ifeq ($(CFG),Debug)
CPP_FLAGS += -O0 -g -D_DEBUG 

#================================================================================================
# PROFILING (non uso -pg perché non funziona, solo -g per dare i simboli ad OProfile)
#================================================================================================
else
ifeq ($(CFG),Profiling)
CPP_FLAGS += -O3 -g -DNDEBUG

#================================================================================================
# TESTING (qui corrisponde a PROFILING)
#================================================================================================
else
ifeq ($(CFG),Testing)
CPP_FLAGS += -O3 -g -DNDEBUG

endif
endif
endif
endif





#================================================================================================
# DO JOB
#================================================================================================

OBJDIR = $(BASEOUTDIR)$(TARGET)/$(PLATFORM)/$(CFG)
TARGETDIR = $(BASEOUTDIR)bin/$(PLATFORM)/$(CFG)

OBJS = $(patsubst %.c,$(OBJDIR)/%.o,$(filter %.c,$(SOURCES)))
OBJS += $(patsubst %.cpp,$(OBJDIR)/%.o,$(filter %.cpp,$(SOURCES)))

.PHONY: ALL
ALL: lib $(TARGETDIR)/$(TARGET)

.PHONY: clean
clean:
	rm -f $(OBJDIR)/*.o $(OBJDIR)/*.d $(TARGETDIR)/$(TARGET)

GCC_MAJOR    := $(shell $(CPP) -v 2>&1 | grep " version " | cut -d' ' -f3  | cut -d'.' -f1)
GCC_MINOR    := $(shell $(CPP) -v 2>&1 | grep " version " | cut -d' ' -f3  | cut -d'.' -f2)

#the dependencies file is created on the fly...
#note that there is a difference between GCC 2.95 and GCC 3.x
#GCC 2.95 has no -MF option and the destination of the .d file
#is the current directory and not the object file's one
$(OBJDIR)/%.o : %.cpp
	@mkdir -p $(OBJDIR)
	@mkdir -p $(TARGETDIR)
	@echo Making $@...
ifeq ($(GCC_MAJOR),3)
		@$(CPP) -c -MF $(OBJDIR)/$*.d.tmp -MMD $(CPP_FLAGS) $(CPP_FLAGS_CONFIG) $(INCLUDE) -o $@ $<
else
		@$(CPP) -c -MMD $(CPP_FLAGS) $(CPP_FLAGS_CONFIG) $(INCLUDE) -o $@ $<
		@mv -f $(OBJDIR)/$*.d $(OBJDIR)/$*.d.tmp
endif
	@set -e; cat $(OBJDIR)/$*.d.tmp | sed 's!\($(*F)\)\.o[ :]*!\1.o $(OBJDIR)/$*.d : !g' > $(OBJDIR)/$*.d; [ -s $(OBJDIR)/$*.d ] || rm -f $(OBJDIR)/$*.d
	@rm -f $(OBJDIR)/$*.d.tmp

$(OBJDIR)/%.o : %.c
	@mkdir -p $(OBJDIR)
	@mkdir -p $(TARGETDIR)
	@echo Making $@...
ifeq ($(GCC_MAJOR),3)
		@$(CPP_C) -c -MF $(OBJDIR)/$*.d.tmp -MMD $(CPP_FLAGS) $(CPP_FLAGS_CONFIG) $(INCLUDE) -o $@ $<
else
		@$(CPP_C) -c -MMD $(CPP_FLAGS) $(CPP_FLAGS_CONFIG) $(INCLUDE) -o $@ $<
		@mv -f $(OBJDIR)/$*.d $(OBJDIR)/$*.d.tmp
endif
	@set -e; cat $(OBJDIR)/$*.d.tmp | sed 's!\($(*F)\)\.o[ :]*!\1.o $(OBJDIR)/$*.d : !g' > $(OBJDIR)/$*.d; [ -s $(OBJDIR)/$*.d ] || rm -f $(OBJDIR)/$*.d
	@rm -f $(OBJDIR)/$*.d.tmp

-include $(patsubst %.c,$(OBJDIR)/%.d,$(filter %.c,$(SOURCES))) dummy
-include $(patsubst %.cpp,$(OBJDIR)/%.d,$(filter %.cpp,$(SOURCES))) dummy

BASENAME_TARGET = $(basename $(TARGET))

$(TARGETDIR)/$(BASENAME_TARGET).so: $(OBJS) $(LIBRARYDEPENDENCY) 
	@echo $(PRINTCFGh)
	@echo $(PRINTCFG)
	@echo Generating $(TARGET)
	@echo $(PRINTCFGt)
	@$(CPP) -shared -Wl,-soname,$(TARGET) $(CPP_FLAGS_CONFIG) $(LD_FLAGS) $(LD_FLAGS_CONFIG) -o $(TARGETDIR)/$(TARGET) $(OBJS) -Wl,--start-group $(LIBS) $(LIBRARYDEPENDENCY) -Wl,--end-group 
	@echo $(PRINTCFGh)
	@echo $(PRINTCFG)
	@echo Ended $(TARGET)
	@echo $(PRINTCFGt)

$(TARGETDIR)/$(BASENAME_TARGET).a: $(OBJS) $(LIBRARYDEPENDENCY) 
	@echo $(PRINTCFGh)
	@echo $(PRINTCFG)
	@echo Generating $(TARGET)
	@echo $(PRINTCFGt)
	@$(AR) rcs $(TARGETDIR)/$(TARGET) $(OBJS)   
	@echo $(PRINTCFGh)
	@echo $(PRINTCFG)
	@echo Ended $(TARGET)
	@echo $(PRINTCFGt)

$(TARGETDIR)/$(BASENAME_TARGET).ex: $(OBJS) $(LIBRARYDEPENDENCY) 
	@echo $(PRINTCFGh)
	@echo $(PRINTCFG)
	@echo $(PRINTCFGt)
	@echo Generating $@
	$(CPP) -o $(TARGETDIR)/$(TARGET) $(OBJS) $(LIBRARYDEPENDENCY) $(CPP_FLAGS) $(LD_FLAGS) $(LIBS)

