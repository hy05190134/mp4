#ifdef __cplusplus
extern "C"
{
#endif

#include "m4u8_ts_maker.h"

int generate();

int generate()
{
    const char *mp4filename = "http://10.0.5.112:1991/laki/HZW.mp4";
    const char *indexpath = "HZW.mp4.up.index";
    const char *tsname = "x";
    int seg_index = 4;

    M4u8TSMaker ts_maker;
    InitM4u8TSMaker(&ts_maker, mp4filename, indexpath, tsname);
    OnStart(seg_index, &ts_maker); 
    DestroyM4u8TSMaker(&ts_maker);

    return 0;
}

#ifdef __cplusplus
}
#endif
