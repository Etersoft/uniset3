noinst_PROGRAMS = server client
server_LDADD = $(top_builddir)/lib/libUniSet3.la $(SIGC_LIBS) $(PROTOBUF_LIBS)
server_CPPFLAGS = -I$(top_builddir)/include -I$(top_builddir)/extensions/include $(SIGC_CFLAGS) $(PROTOBUF_CFLAGS)
server_SOURCES = server.cc server.grpc.pb.cc server.pb.cc

client_LDADD = $(top_builddir)/lib/libUniSet3.la $(SIGC_LIBS) $(PROTOBUF_LIBS)
client_CPPFLAGS = -I$(top_builddir)/include -I$(top_builddir)/extensions/include $(SIGC_CFLAGS) $(PROTOBUF_CFLAGS)
client_SOURCES = client.cc server.grpc.pb.cc server.pb.cc

BUILT_SOURCES = server.grpc.pb.cc server.grpc.pb.h server.pb.cc server.pb.h

server.grpc.pb.cc server.grpc.pb.h: server.proto
	${PROTOC} --grpc_out=. --plugin=protoc-gen-grpc=$(GRPC_CPP_PLUGIN) --proto_path=./ server.proto

server.pb.cc server.pb.h: server.proto
	${PROTOC} --cpp_out=. --proto_path=./ server.proto
