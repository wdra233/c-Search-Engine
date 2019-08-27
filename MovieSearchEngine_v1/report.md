# A9: Multi-Thread


## Author: Yawang Wang since Apr 4, 2019
## Completed on Apr 8.
## Total estimation of completing this assignment is 9-10 hours. 

### To answer in Step1: 
### To clarify, I run benchmarker on data foler, which consists of movietitles_aa, movietitles_ab
### movietitles_ac, movietitles_ad, movietitles_ae, movietitles_ah files.
* Which is bigger, the TypeIndex or the OffsetIndex? Why?
  
  OffsetIndex is bigger than TypeIndex. Building OffsetIndex takes up about 38MB memory, while TypeIndex take up
  about 20MB memory. I think TypeIndex is mainly built up by using LinkedList, which takes smaller space,
  whereas OffsetIndex is maily built up by using hashTable, which really takes up more memory when it
  compares to TypeIndex.
    
* Which will be faster to build, the TypeIndex or the OffsetIndex? Why?
  
  It took only 2.680707 seconds to build the TypeIndex. As for the OffsetIndex, it took 6.446252 seconds to
  execute. So it's clearly that building the TypeIndex is much "cheaper" and faster. Because we all know that
  building hashtalbe is time-consuming. In the last assignment, the number of buckets in hashtalbe is set to be
  1000, which in turn consumes time and space. But for LinkedList, it will be faster to build up. 
* Which will be faster to query, the OffsetIndexer or the TypeIndex? For what kinds of queries? Why?
  
  To make the query comparison fair, I did the following experiments:
  ex1. Use an OffsetIndex, get a list of movies that has Seattle in the title, search through that list
     and find the ones that are Comedy.
  ex2. Use a TypeIndex, get a list of movies that are Comedy, find the ones that have Seattle in the title.

  It took 0.2 seconds on average to execute ex1, while it took 0.3 seconds on average to execute ex2.
  Thus, Using a TypeIndex to query is a little slower than using an OffsetIndex. The main reason is that
  hashTable is easy to search because you can use a "Key" to find the value. However, for LinkedList,
  you have to iterate through the list from beginning to end to search one certain item, which really
  takes up too much time!

### To answer in Step2:
### To clarify, I run benchmarker on data foler, which consists of movietitles_aa, movietitles_ab
### movietitles_ac, movietitles_ad, movietitles_ae, movietitles_ah files.  
* Something abou multiThread
  
  This time, I create 5 new threads to run the function IndexTheFile_MT.
  I run benchmarker on data_tiny and data_small respectively. It seems like my computer won't get much benefit 
  when using multi-threads on these 2 folers.
  When I run multi-thread on data_tiny folder, it took 0.004229 seconds to
  Similarly, it took 0.311592 seconds to execute the program using multi-thread, whereas it only took
  0.291488 seconds to execute on data_small foler.
  
  Interestingly, if I run Benchmarker on data foler, which is much larger than previous two files, things will
  be different.
  It took 6.548564 seconds to execute the program using multi-thread, while single thread took 6.649418
  seconds to execute. We can easily see that the running time is almost the same.
  
* Next, I'll do is to swith the number of cores to 2 on virtual box.
  
  It took 6.591819 seconds to execute on single thread, whereas on multi-thread, it took 6.166188 seconds to
  execute.

  EXCITING!
  Seems like our computer will get benefit when using multi threads on large data and multiple cores!
  Done! 
  
* I'll use one core and single thread when executing single, simple task. I'll definitely use multi threads
  when executing multiply, large data-driven tasks.


