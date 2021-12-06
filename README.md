### Assignment 4

 This assignment was a simulation of a page replacement scheme for memory.
 There are three options for page replacement specified as a command line argument.
 --replace=fifo used a FIFO page replacement scheme, where the oldest page was
 chosen as the victim frame. --replace=lfu used a LFU page replacement scheme,
 meaning the page that had been used the least amount of times was chosen as the
 victim. In the case of a time, the oldest frame with the fewest interactions was
 chosen. Finally, --replace=clock used a two handed clock page replacement scheme,
 where a reference bit is set on a page whenever a hit occurs. The first clock
 hand goes through the table searching for a frame where the reference bit is not
 set, and the second hand goes through with a distance of 1/2 the table setting
 reference bits to 0. These two hands move at the same rate.
 The virtmem.c File was given as very minimal skeleton code, and all functional
 implementation was added by me.
