
BUILT_SOURCES=unet.pb.h unet.pb.cc

$(BUILT_SOURCES): unet.proto
	${PROTOC} --cpp_out=$(srcdir) unet.proto

proto_include_HEADERS = *.proto
proto_includedir = $(pkgincludedir)/extensions
