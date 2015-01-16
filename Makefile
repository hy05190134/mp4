all: libmp4fetch.dylib main

libmp4fetch.dylib: *.c
	gcc -shared -fPIC -o libmp4fetch.dylib generate.c m4u8_ts_maker.c mp4u8_index.c mpegts_muxer.c netutil.c common/byte_stream.c -I. -I./common -lcstl

main: libmp4fetch.dylib
	gcc -o main main.c -L. -lmp4fetch
