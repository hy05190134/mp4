
#include "mp4_ts_maker.h"

int main()
{
    const char *mp4filename = "http://10.0.5.112:1991/laki/HZW.mp4";
    const char *indexpath = "HZW.mp4.up.index";
    const char *tsname = "";
    int seg_index = 2;

    M4u8TSMaker ts_maker;
    InitM4u8TSMaker(mp4filename, indexpath, tsname);
    OnStart(seg_index, ts_maker); 
    DestroyM4u8TSMaker(ts_maker);

    return 0;
}
