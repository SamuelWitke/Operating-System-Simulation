# Operating System Simulation
![alt tag](https://github.com/SamuelWitke/Operating-System-Simulation/blob/master/Life-Cycle-Of-A-Job.png)

## Handle a simulation of jobs arriving , being process and removed


                          FINAL STATISTICS  



 * * * SYSTEM STATUS AT  441818  
 =====================================

 CPU : idle 
 Disk running for job  192   since   441721   
 Drum : idle 
Memory :  85  K words in use
Average dilation :  44.08    
Average Response time :   15997.13  


                                CORE MAP

 Partition   Job   Partition   Job   Partition   JobPartition   Job

     0       36        25      36        50     192        75    222
     1       36        26      36        51     192        76    222
     2       36        27      36        52     192        77    222
     3       36        28      36        53     192        78    222
     4       36        29      36        54     192        79    222
     5       36        30      36        55     192        80    222
     6       36        31      36        56     192        81    222
     7       36        32      36        57     192        82    222
     8       36        33      36        58     192        83    222
     9       36        34      36        59     192        84    222
    10       36        35      36        60     192        85      0
    11       36        36      36        61     192        86      0
    12       36        37      36        62     192        87      0
    13       36        38      36        63     192        88      0
    14       36        39      36        64     192        89      0
    15       36        40      36        65     192        90      0
    16       36        41      36        66     222        91      0
    17       36        42      36        67     222        92      0
    18       36        43      36        68     222        93      0
    19       36        44      36        69     222        94      0
    20       36        45      36        70     222        95      0
    21       36        46      36        71     222        96      0
    22       36        47     192        72     222        97      0
    23       36        48     192        73     222        98      0
    24       36        49     192        74     222        99      0



                            JOBTABLE 

Job#  Size  Time CPUTime MaxCPU  I/O's Priority Blocked  Latched InCore Term
          Arrived  Used  Time   Pending 


 94   10 185041       0    3500    0      1        no      no      no     no
192   19 377831     156     550    1      1       yes     yes     yes     no
182   25 358082       0    2500    0      1        no      no      no     no
 36   47  66587    3684   65000    1      5       yes      no     yes     no
 88    5 168201       0    7100    0      1        no      no      no     no
 90   15 174941       0   14000    0      1        no      no      no     no
 45   15  86326       0   40000    0      2        no      no      no     no
 85   17 165322       0    5300    0      2        no      no      no     no
124   10 244288       0    3500    0      1        no      no      no     no
122   25 239588       0    2500    0      1        no      no      no     no
 92   25 180341       0    2500    0      1        no      no      no     no
 96   47 185081       0   65000    0      5        no      no      no     no
193   23 381131       0    1400    0      2        no      no      no     no
118    5 227448       0    7100    0      1        no      no      no     no
120   15 234188       0   14000    0      1        no      no      no     no
105   15 204820       0   40000    0      2        no      no      no     no
 55   17 106075       0    5300    0      2        no      no      no     no
148    5 286695       0    7100    0      1        no      no      no     no
150   15 293435       0   14000    0      1        no      no      no     no
 58    5 108954       0    7100    0      1        no      no      no     no
126   47 244328       0   65000    0      5        no      no      no     no
 60   15 115694       0   14000    0      1        no      no      no     no
115   17 224569       0    5300    0      2        no      no      no     no
222   19 437078     114     550    1      1       yes      no     yes     no
202   40 397580       0    1000    0      1        no      no      no     no
 64   10 125794       0    3500    0      1        no      no      no     no
145   17 283816       0    5300    0      2        no      no      no     no
 66   47 125834       0   65000    0      5        no      no      no     no
184   10 362782       0    3500    0      1        no      no      no     no
154   10 303535       0    3500    0      1        no      no      no     no
165   15 323314       0   40000    0      2        no      no      no     no
195   15 382561       0   40000    0      2        no      no      no     no
208    5 405189       0    7100    0      1        no      no      no     no
197   10 382590       0    1300    0      2        no      no      no     no
156   47 303575       0   65000    0      5        no      no      no     no
186   47 362822       0   65000    0      5        no      no      no     no
 75   15 145573       0   40000    0      2        no      no      no     no
135   15 264067       0   40000    0      2        no      no      no     no
205   17 402310       0    5300    0      2        no      no      no     no
175   17 343063       0    5300    0      2        no      no      no     no
223   23 440378       0    1400    0      2        no      no      no     no
210   15 411929       0   14000    0      1        no      no      no     no
212   25 417329       0    2500    0      1        no      no      no     no
178    5 345942       0    7100    0      1        no      no      no     no
214   10 422029       0    3500    0      1        no      no      no     no
180   15 352682       0   14000    0      1        no      no      no     no
219   14 427678       0    1500    0      2        no      no      no     no
216   47 422069       0   65000    0      5        no      no      no     no
224    5 441778       0      17    0      1        no      no      no     no
225   15 441808       0   40000    0      2        no      no      no     no






 Total jobs :  225       terminated : 175
 % utilization CPU : 24.96   disk :  82.24  drum : 6.35 memory  : 60.93 

