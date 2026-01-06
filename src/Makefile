# University of Florida EEL6528
# Tan F. Wong
# Aug 31, 2023

CXX = g++ -I/usr/include
CPPFLAGS = -O3 -DRXOTWSC16 -DRXFC32
LDFLAGS = -L/usr/lib -luhd -lboost_program_options
OBJDIR := ../build

common_hdrs = RX.h
OBJS = $(OBJDIR)/main.o $(OBJDIR)/RX.o $(OBJDIR)/energy_calculator.o $(OBJDIR)/energy_detector.o $(OBJDIR)/file_gen.o

all: exec

exec: $(common_hdrs) $(OBJDIR) $(OBJS)
	$(CXX) -o exec $(OBJS) $(LDFLAGS)

$(OBJDIR):
	mkdir -p $@

$(OBJDIR)/%.o: %.cpp $(common_hdrs)
	$(CXX) $(CPPFLAGS) -c $< -o $@

clean: 
	rm -rf exec $(OBJDIR)

