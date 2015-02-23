#Macro
CC = g++
CFLAGS = -Wall -O3
LDFLAGS =
INCLUDES = /home/sweet/m-morita/boost_1_33_0
LIBS = -lstdc++ -static
TARGET = ops
OBJS = topology.o simuframe.o packet.o basenetwork.o opsnetwork.o event.o opssimu.o main.o

#gen
all:	$(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $(OBJS) $(LIBS)

clean:
	-rm -f $(TARGET) $(OBJS) .nfs* *~ \#* core

.cpp.o:
	$(CC) $(CFLAGS) -I $(INCLUDES) -c $<

#make rule

topology.o:	topology.cpp
TOPOLOGY_H = topology.h
topology.o:	$(TOPOLOGY_H)

simuframe.o:	simuframe.cpp
SIMUFRAME_H = simuframe.h statable.h
simuframe.o:	$(SIMUFRAME_H)

packet.o:	packet.cpp
PACKET_H = packet.h mtrandom.h histogram.h $(TOPOLOGY_H) $(SIMUFRAME_H)
packet.o:	$(PACKET_H)

basenetwork.o:	basenetwork.cpp
BASENETWORK_H = basenetwork.h statable.h fixedstack.h
basenetwork.o:	$(BASENETWORK_H)

opsnetwork.o:	opsnetwork.cpp
OPSNETWORK_H = opsnetwork.h $(TOPOLOGY_H) $(BASENETWORK_H) $(PACKET_H) mtrandom.h
opsnetwork.o:	$(OPSNETWORK_H) event.h

event.o:	event.cpp
EVENT_H = event.h $(SIMUFRAME_H) $(OPSNETWORK_H)
event.o:	$(EVENT_H)

opssimu.o:	opssimu.cpp
OPSSIMU_H = opssimu.h $(SIMUFRAME_H) $(OPSNETWORK_H)
opssimu.o:	$(OPSSIMU_H)

main.o:	main.cpp
main.o:	$(OPSSIMU_H)
