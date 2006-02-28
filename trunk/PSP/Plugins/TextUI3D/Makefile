BASE_DIR = ../..
include $(BASE_DIR)/include.mak

TARGET = $(KXPLOIT_PRIMARY_DIR)/TextUI3D

EXPORTED_OBJS = $(PSPRADIO_DIR)/PSPRadioExports.o 
#$(PSPRADIO_DIR)/pnglibExports.o

OBJS =	$(PSPRADIO_DIR)/IPSPRadio_UI.o $(EXPORTED_OBJS) \
		main.o TextUI3D.o TextUI3D_Panel.o TextUI3D_WindowManager_HSM.o \
		jsaVRAMManager.o jsaTextureCache.o

# Define to build this as a prx (instead of a static elf)
BUILD_PRX=1
PRX_EXPORTS=$(PLUGINS_DIR)/UI_Exports.exp

INCLUDES = -I./ -I$(BASE_DIR) -I$(PSPRADIO_DIR) -I$(SHAREDLIB_DIR)/PSPApp -L$(SHAREDLIB_DIR)/lib \
		 -I$(SHAREDLIB_DIR)/libpspnet -I$(SHAREDLIB_DIR)/libiniparser \
		 -I$(SHAREDLIB_DIR)/Tools -I$(SHAREDLIB_DIR)/Logging \
		 -I$(PSPRADIO_DIR)/ScreenHandler 

		 
CFLAGS = $(GLOBAL_CFLAGS) -O2 -g -Wall $(INCLUDES) 
CXXFLAGS = $(CFLAGS) -fno-exceptions -fno-rtti
ASFLAGS = $(CFLAGS)

LIBDIR =

LIBS =  -lstdc++ -liniparser -lTools  \
		-lpspgum -lpspgu -lpng -lz -lm

all: $(TARGET).prx
	mkdir -p $(TARGET)
	mv -vf $(TARGET).elf .
	cp -vf *.cfg $(TARGET)
	cp -vf Graphics/*.png $(TARGET)

PSPSDK=$(shell psp-config --pspsdk-path)
include $(PSPSDK)/lib/build.mak
