INCPROTODIR=$(abs_top_builddir)/proto

# Получения списков генерируемых файлов
HHTARG=$(patsubst %.proto, ${HHDIR}/%.pb.h, ${PROTOFILES})
CCTARG=$(patsubst %.proto, ${CCDIR}/%.pb.cc, ${PROTOFILES})

########################################################################


all: ${HHTARG} ${CCTARG}
	

dynamic: all
	

${CCTARG}: ${PROTOFILES}
	for i in $^; do ${PROTOC} -I$(INCPROTODIR) --cpp_out=. --proto_path=./ ${PROTOFLAGS} $$i; done
	for i in $^; do ${PROTOC} -I$(INCPROTODIR) --grpc_out=. --plugin=protoc-gen-grpc=$(GRPC_CPP_PLUGIN) --proto_path=./ ${PROTOFLAGS} $$i; done
	mv --target-directory=${CCDIR} *.cc



${HHTARG}: ${PROTOFILES}
	for i in $^; do ${PROTOC} -I$(INCPROTODIR) --cpp_out=. --proto_path=./ ${PROTOFLAGS} $$i; done
	for i in $^; do ${PROTOC} -I$(INCPROTODIR) --grpc_out=. --plugin=protoc-gen-grpc=$(GRPC_CPP_PLUGIN) --proto_path=./ ${PROTOFLAGS} $$i; done
	mv --target-directory=${HHDIR} *.h


.PHONY: clean depend
clean:
	${RM} ${HHTARG} ${CCTARG}

depend:
	

install:
	
