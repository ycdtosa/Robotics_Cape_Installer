CC		:= gcc
LINKER		:= gcc
CWARNINGS	:= -Wall -Wextra
# achitecture flags enables the NEON hardware SIMD floating point unit
ARCFLAGS	:= -mfpu=neon -mfloat-abi=hard -march=armv7-a -mtune=cortex-a8 
# enable O3 optimization and vectorized math
FFLAGS		:= -O3 -ffast-math -ftree-vectorize
CFLAGS		:= -c -fPIC
LFLAGS		:= -lm -lrt -lpthread -shared -Wl,-soname,$(TARGET)
INCLUDE		:= -I../../include

SOURCES		:= $(shell find ./ -name '*.c')
OBJECTS		:= $(SOURCES:$%.c=$%.o)

prefix		:= /usr
RM		:= rm -f -r
INSTALL		:= install -m 755 
INSTALLDIR	:= install -d -m 755

LIBDIR		:= ../../lib

# linking Objects
$(TARGET): $(OBJECTS)
	@$(LINKER) $(LFLAGS) -o $(TARGET) $(OBJECTS)
	@echo " "
	@echo "Linked $(TARGET)"
	@echo " "

$(OBJECTS): %.o : %.c
	$(CC) $(CFLAGS) $(CWARNINGS) $(INCLUDE) $(FFLAGS) $(ARCFLAGS) $(DEBUGFLAG) $(DEFS) -c $< -o $(@)
	@echo "Compiled: "$<

clean:
	@$(RM) */*.o
	@$(RM) *.o
	@$(RM) *.so
	@echo " "
	@echo "$(TARGET) Clean Complete"
	@echo " "
