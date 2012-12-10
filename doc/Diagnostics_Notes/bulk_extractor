example process of functional testing from Simson, 7/3/12 5:32AM:

I'm currently doing some testing on bulk_extractor to explore the impacts on performance by changing the page size from 16MiB to 1MiB and the margin size from 1MiB to 15MiB. I can do that by using the -g and -G flags and swapping the arguments. George Dinolt had started on this work but didn't follow through.

My initial testing with the centos virtual machine found that the 1/15 approach led to 3x the time to run. If it recovers 3x the features, then that's good --- it means we had features that are too big to be found with the 1MiB margin. But I think that the centos disk is a special case, in that I think that the disk image contains a large number of very big compressed objects.  So I'm doing some comparison tests on one of the Terry disks.

During the course of the experiment I saw this happen:

 8:22:23 Offset 6643MB (32.37%) Done in  0:22:58 at 08:45:21
 8:22:30 Offset 6710MB (32.70%) Done in  0:22:51 at 08:45:21
 8:22:34 Offset 6777MB (33.02%) Done in  0:22:39 at 08:45:13
 8:22:43 Offset 6845MB (33.35%) Done in  0:22:38 at 08:45:21
zero length feature at (|6528524288)
 8:22:53 Offset 6912MB (33.68%) Done in  0:22:39 at 08:45:32
 8:23:02 Offset 6979MB (34.00%) Done in  0:22:36 at 08:45:38
 8:23:16 Offset 7046MB (34.33%) Done in  0:22:43 at 08:45:59
 8:23:27 Offset 7113MB (34.66%) Done in  0:22:44 at 08:46:11


The zero length feature is a problem. I'm not sure what is causing it, but it may be symptomatic of a deeper bug. So I'm going to re-run bulk_extractor under the debugger and set a break point at the zero length feature printout. I'm then going to determine why it happened and fix the rule or whatever is the cause of the problem.

This is what I mean by testing. Detailed testing, looking for weird things that the program does, and then fixing them. 

At the same time, I am taking notes of the differences in the timing tests. I'm not keeping as many notes as I should and I'm running too many experiments without recording the results. But in this case I'll store the results in the TESTING.txt file in a manner that others can benefit from the results.

More later.

As the group gets larger, we need to get more systematic and more professional in the testing so that work is not duplicated and so that more cases are explored. My believe is that either Tony or Bruce should be the lead on testing and the other should be the lead on the documentation. I request that the two of you have a phone call sometime today, discuss the various tasks that need to be done, and present me with a plan of action and a timetable for both the documentation and the testing by the end of July 5.

------------------------------------------------------------
