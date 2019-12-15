#include "libQccPack.h"
#include <stdio.h>

int m_calc_low_freq_len( int orig_len, int lev )
{
    int low_len = orig_len;
    for( int i = 0; i < lev; i++ )
        low_len = low_len % 2 == 0 ? low_len / 2 : (low_len + 1) / 2;
    
    return low_len;
}

int main()
{
    for( int orig_len = 60 ; orig_len < 1650; orig_len++ )
        for( int lev = 1; lev < 8; lev++ )
        {
            int qcc = QccWAVWaveletDWTSubbandLength( orig_len, lev, 0, 0, 0 );
            int sam = m_calc_low_freq_len( orig_len, lev );
            if( qcc != sam )
                printf("oops! qcc = %d, sam = %d\n", qcc, sam);
        }
}
