# FIFO_IPC
一个比较基本的IPC通信层Demo.
创建两个fifo, 两个moodycamel的无锁阻塞队列. 
异步的收发fifo里面的消息, 十几分钟写好就直接用了, 没有搞析构, 没有测试, 没有时间验证.
打算用作客户端的前置程序文件(FIFO)交互接口, 讲道理可以规避一些监管问题.
# TODO
1. 多写点注释, 做个人吧?!
2. 写个Demo.
3. 收拾收拾代码, 不能因为快就写的不太规范.
4. 也没啥了, 就是传点东西, 别让Git太空了吧(笑).
