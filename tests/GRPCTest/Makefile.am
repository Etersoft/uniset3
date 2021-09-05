noinst_PROGRAMS = server client async_server async_client async_consumer
server_LDADD = $(top_builddir)/lib/libUniSet3.la $(SIGC_LIBS) $(PROTOBUF_LIBS)
server_CPPFLAGS = -I$(top_builddir)/include -I$(top_builddir)/extensions/include $(SIGC_CFLAGS) $(PROTOBUF_CFLAGS)
server_SOURCES = server.cc server.grpc.pb.cc server.pb.cc

async_server_LDADD = $(top_builddir)/lib/libUniSet3.la $(SIGC_LIBS) $(PROTOBUF_LIBS)
async_server_CPPFLAGS = -I$(top_builddir)/include -I$(top_builddir)/extensions/include $(SIGC_CFLAGS) $(PROTOBUF_CFLAGS)
async_server_SOURCES = async_server.cc async_server.grpc.pb.cc async_server.pb.cc

client_LDADD = $(top_builddir)/lib/libUniSet3.la $(SIGC_LIBS) $(PROTOBUF_LIBS)
client_CPPFLAGS = -I$(top_builddir)/include -I$(top_builddir)/extensions/include $(SIGC_CFLAGS) $(PROTOBUF_CFLAGS)
client_SOURCES = client.cc server.grpc.pb.cc server.pb.cc

async_client_LDADD = $(top_builddir)/lib/libUniSet3.la $(SIGC_LIBS) $(PROTOBUF_LIBS)
async_client_CPPFLAGS = -I$(top_builddir)/include -I$(top_builddir)/extensions/include $(SIGC_CFLAGS) $(PROTOBUF_CFLAGS)
async_client_SOURCES = async_client.cc async_server.grpc.pb.cc async_server.pb.cc

async_consumer_LDADD = $(top_builddir)/lib/libUniSet3.la $(SIGC_LIBS) $(PROTOBUF_LIBS)
async_consumer_CPPFLAGS = -I$(top_builddir)/include -I$(top_builddir)/extensions/include $(SIGC_CFLAGS) $(PROTOBUF_CFLAGS)
async_consumer_SOURCES = async_consumer.cc

BUILT_SOURCES = server.grpc.pb.cc server.grpc.pb.h server.pb.cc server.pb.h async_server.grpc.pb.cc async_server.grpc.pb.h async_server.pb.cc async_server.pb.h

server.grpc.pb.cc server.grpc.pb.h: server.proto
	${PROTOC} --grpc_out=. --plugin=protoc-gen-grpc=$(GRPC_CPP_PLUGIN) --proto_path=./ server.proto

server.pb.cc server.pb.h: server.proto
	${PROTOC} --cpp_out=. --proto_path=./ server.proto

async_server.grpc.pb.cc async_server.grpc.pb.h: async_server.proto
	${PROTOC} --grpc_out=. --plugin=protoc-gen-grpc=$(GRPC_CPP_PLUGIN) --proto_path=./ async_server.proto

async_server.pb.cc async_server.pb.h: async_server.proto
	${PROTOC} --cpp_out=. --proto_path=./ async_server.proto
