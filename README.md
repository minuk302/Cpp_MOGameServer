# MOGameServer

* Progress
  * Doing
    * Basic IOCP recieve and send
    * log system.
      * to file
        * change file on set time
        * change file when exceed set size
      * exception
  * To do
    * Echo server and client, testing
    * Adding and parsing config file
    * Dispatcher
    * Session management system
    * Test client
    * Pooling for instances used to recv
    * DB sp call,receive
    * dump making system
    * build automation on push
    * ipv4/ipv6 compatibility(ipv4 for now)
    * reference/learn
      * boost asio
      * .net core
      * thread queue between network, db threads, through iocp,  Inter-Thread Communication
        * not to waste cpu cycle, by making notify
	* PostQueuedCompletionStatus
  * Done
    * log system
      * to log debbug window
      * to file

 * Etc
   * IDE : VS2017
   * OS : Windows 10