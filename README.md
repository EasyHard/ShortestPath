TODO List
--------------------

 * Color
     * 模块化
     * 支持dump Color Files
     * 调研其他Road Network Partition方法

 * 弱同步的最短路实现


----------

##同步协议

```
globalVersion

localVersion[ThreadNum]

oldVersion=globalVersion

if localVersion[currentThreadID]<oldVersion
    try to relax, if succeed ++globalVersion
    localVersion[currentThreadID]=oldVersion
MemoryFence
if oldVersion==globalVersion
    more=ExistOne(localVersion[1..ThreadNum] != oldVersion) || (oldVersion != globalVersion)
else
    more=true
```

--------------
##volatile如何使用

```
volatile int* ptr; // content ptr points is volatile
int * volatile ptr; // ptr itself is volatile
```
 