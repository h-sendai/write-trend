# write-trend / read-trend

write(2)を使ってファイルに書き込むプログラム。
(当初はfwrite(3)を使っていましたが、やめました）。

writeもあるならreadも、ということでread-trendというプログラムも
作りました。

## write-trend 使い方

```
./write-trend filename 1回fwriteするサイズ 最終ファイルサイズ
```

サイズの指定にはk (kilo), m (mega), g (giga)が使えます。

```
./write-trend test.file 32k 4g
```
と起動すると、
32kB fwrite()するのを繰り返し、4GBまで書きます。
1秒に1度、書き込み速度を表示します。

経過時間、その1秒間での書き込み速度、トータルファイルサイズの順に
表示します。

```
% ./write-trend test.file 12k 4g
1.000035 583.559 MB/s 583.559 MB
2.000045 149.004 MB/s 732.562 MB
3.008992 121.359 MB/s 853.922 MB
4.000054 101.566 MB/s 955.488 MB
5.001983 124.312 MB/s 1079.801 MB
6.001995 111.375 MB/s 1191.176 MB
7.007022 115.453 MB/s 1306.629 MB
8.000992 123.645 MB/s 1430.273 MB
9.004989 112.875 MB/s 1543.148 MB
10.003990 112.875 MB/s 1656.023 MB
11.005986 117.914 MB/s 1773.938 MB
12.005990 112.875 MB/s 1886.812 MB
13.002981 113.883 MB/s 2000.695 MB
14.051807 101.801 MB/s 2102.496 MB
15.083791 115.910 MB/s 2218.406 MB
16.192840 124.441 MB/s 2342.848 MB
17.001013 117.363 MB/s 2460.211 MB
18.007000 121.406 MB/s 2581.617 MB
19.000046 114.340 MB/s 2695.957 MB
20.001001 118.348 MB/s 2814.305 MB
21.001005 123.434 MB/s 2937.738 MB
22.000994 114.645 MB/s 3052.383 MB
23.001986 114.762 MB/s 3167.145 MB
24.000047 115.980 MB/s 3283.125 MB
25.004045 116.602 MB/s 3399.727 MB
26.004985 115.781 MB/s 3515.508 MB
27.001980 114.762 MB/s 3630.270 MB
28.002009 120.867 MB/s 3751.137 MB
29.001008 115.781 MB/s 3866.918 MB
30.003146 116.789 MB/s 3983.707 MB
```

## オプション

```
write-trend [-d] [-i interval] [-s usec] [-C] [-D] filename buffer_size total_size
suffix m for mega, g for giga
Options:
-d debug
-i interval (default: 1 seconds): print interval (may decimal such as 0.1)
-s usec (default: none): sleep usec micro seconds between each write
-C : drop page cache after all write() done
-D : Use direct IO (O_DIRECT)
-S : Use synchronized IO (O_SYNC)
```

### -i interval

途中経過の出力間隔を指定します。指定しなかった場合は1秒おきに表示がでます。
0.1のように小数も指定できます。

### -s usec

write()終了後、usecマイクロ秒スリープして、次のwrite()を行います。

### -C

全てのwrite()が終了後、filenameで指定したファイルのページキャッシュを削除します。

### -D

O_DIRECTを付けてopen()します（ダイレクトIO。OSのページキャッシュ機能をバイパスして
write()するようになる）。

### -S

O_SYNCを付けてopen()します（synchronized IO。とてつもなく遅くなります）。

## read-trend 使い方

read-trend -h でヘルプがでます。

```
Usage: read-trend [-i interval] [-b bufsize] filename
-i interval (allow decimal) sec (default 1 second)
-b bufsize  buffer size (default 64kB)
-D          use O_DIRECT
```

intervalは小数でも指定できます。例:

```
read-trend -i 0.1 test.file
```
