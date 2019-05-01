 Make sure two nodes are on the same kernel 

Node A:
 4.15.0-45-generic
 cd hetnet-gzip && make

 cp popcorn-miniz popcorn-miniz_x86-64 popcorn-miniz_aarch64 /popcorn/miniz

 helios@helios-PowerEdge-R7425:/popcorn$ ls -lt
 drwxrwxrwx 3 helios helios    4096 May  1 09:53 miniz
 
 helios@helios-PowerEdge-R7425:/popcorn/miniz$ ls -lt
 rwxrwxr-x 1 helios helios  9065602 May  1 09:53 popcorn-miniz

 ./popcorn-miniz 1.txt 
[50163] hetpot is waiting for compression  0
[50163] hetpot is waiting for compression  1
[50163] hetpot is waiting for compression  2
[50163] hetpot is waiting for compression  3
[50163] hetpot is waiting for compression  4
[50163] hetpot is waiting for compression  5
[50163] hetpot is waiting for compression  6
[50163] hetpot is waiting for compression  7
[50163] hetpot is waiting for compression  8
 ........................

 Open another terminal and go to the same directory /popcorn/miniz
  ps -C popcorn-miniz
  PID TTY          TIME CMD
 50163 pts/9    00:00:00 popcorn-miniz
 sudo criu-het dump --arch aarch64 -j -t 50163

 

[50163] hetpot is waiting for compression  9
[50163] hetpot is waiting for compression  10
[50163] hetpot is waiting for compression  11
[50163] hetpot is waiting for compression  12
[50163] hetpot is waiting for compression  13
[50163] hetpot is waiting for compression  14
[50163] hetpot is waiting for compression  15
[50163] hetpot is waiting for compression  16
[50163] hetpot is waiting for compression  17
[50163] hetpot is waiting for compression  18
[50163] hetpot is waiting for compression  19
dest arch is 0
__migrate_shim_internal 121
__migrate_shim_internal 123
Killed

 sudo scp popcorn-miniz* 1.txt sunsky@192.168.10.22:/popcorn/miniz
 sudo scp -r aarch64/ sunsky@192.168.10.22:/popcorn/miniz


Node B:
 4.15.0-45-generic

sunsky@sunsky:/popcorn$ ls -lt
drwxrwxrwx 3 sunsky sunsky    4096 May  1 09:55 miniz
sunsky@sunsky:/popcorn/miniz$ ls -lt
-rw-rw-r-- 1 sunsky sunsky       14 May  1 09:53 1.txt
-rwxrwxr-x 1 sunsky sunsky  9065602 May  1 09:53 popcorn-miniz

sunsky@sunsky:/popcorn/miniz$ cd aarch64/
sunsky@sunsky:/popcorn/miniz/aarch64$ sudo criu-het restore -j

[50163] hetpot is waiting for compression  20
[50163] hetpot is waiting for compression  21
[50163] hetpot is waiting for compression  22
[50163] hetpot is waiting for compression  23
[50163] hetpot is waiting for compression  24
[50163] hetpot is waiting for compression  25
[50163] hetpot is waiting for compression  26
[50163] hetpot is waiting for compression  27
[50163] hetpot is waiting for compression  28
[50163] hetpot is waiting for compression  29
[50163] hetpot is waiting for compression  30
[50163] hetpot is waiting for compression  31
[50163] hetpot is waiting for compression  32
[50163] hetpot is waiting for compression  33
[50163] hetpot is waiting for compression  34
[50163] hetpot is waiting for compression  35
[50163] hetpot is waiting for compression  36
[50163] hetpot is waiting for compression  37
[50163] hetpot is waiting for compression  38
[50163] hetpot is waiting for compression  39
[50163] hetpot is waiting for compression  40
[50163] hetpot is waiting for compression  41
[50163] hetpot is waiting for compression  42
[50163] hetpot is waiting for compression  43
[50163] hetpot is waiting for compression  44
[50163] hetpot is waiting for compression  45
[50163] hetpot is waiting for compression  46
[50163] hetpot is waiting for compression  47
[50163] hetpot is waiting for compression  48
[50163] hetpot is waiting for compression  49
[50163] hetpot is waiting for compression  50
[50163] hetpot is waiting for compression  51
[50163] hetpot is waiting for compression  52
[50163] hetpot is waiting for compression  53
[50163] hetpot is waiting for compression  54
[50163] hetpot is waiting for compression  55
[50163] hetpot is waiting for compression  56
[50163] hetpot is waiting for compression  57
[50163] hetpot is waiting for compression  58
[50163] hetpot is waiting for compression  59
Hetpot starts compression
sunsky@sunsky:/popcorn/miniz/aarch64$ cd ..
sunsky@sunsky:/popcorn/miniz$ ls -lt
-rwxr-x--T 1   1004 sunsky       24 May  1 09:55 1.txt.gz



