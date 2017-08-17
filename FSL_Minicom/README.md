# Time_chk_minicom
modified minicom to measure the time target board requests

Has two timeval variables; start, end

When target board prints "~", dogettimeofday(&start,NULL)
When target board prints "!", dogettimeofday(&end,NULL), and prints (end-start) time
