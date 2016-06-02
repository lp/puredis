h1. Puredis

h3. "Redis":http://redis.io/ external for "Pure Data":http://puredata.info/.

h2. Build

bc. wget https://github.com/lp/puredis/tarball/master
PUREDIS_SRC=`find . -name lp-puredis-* -print`
tar xzf $PUREDIS_SRC
cd ${PUREDIS_SRC:0:20}
# *** edit the Makefile top portion with proper system settings ***
make

h2. Dependencies

p. The build process will take care of fetching, compiling and linking to "Hiredis":https://github.com/antirez/hiredis and "libcsv":http://sourceforge.net/projects/libcsv/.  The core and purpose of this external, "Redis":http://redis.io/, is not bundled herein and must be installed separately.  The only mandatory Redis configuration (redis.conf) option for Puredis is "timeout 0" (meaning no timeout), unless your use of Puredis is limited to short time spans.

h2. Usage:

!https://github.com/lp/puredis/raw/master/img/puredis-help.png!

bc. SET: symbol OK
GET: symbol BAR
LPUSH: 1
LPUSH: 2
LPUSH: 3
LPOP: symbol VALUE3
LRANGE: list VALUE2 VALUE1
HMSET: symbol OK
HGET: symbol value2
HMGET: list value1 value3
HVALS: list value1 value2 value3
SADD: 1
SADD: 1
SADD: 1
SADD: 1
SADD: 1
SINTER: symbol C
SUNION: list C A B
ZADD: 1
ZADD: 1
ZADD: 1
ZADD: 1
ZADD: 1
ZINCRBY: symbol 4
ZRANGEBYSCORE: list D 10 E 100
ZRANGE: list A 1 B 2 C 4

h2. Async Puredis: apuredis

!https://github.com/lp/puredis/raw/master/img/apuredis-help.png!

bc. right: 1
bang: bang
right: 1
bang: bang
...
left: symbol BAR
right: 0

h2. Pub/Sub Puredis: spuredis

!https://github.com/lp/puredis/raw/master/img/spuredis-help.png!

bc. subscriber: list subscribe X 1
subscriber: list subscribe NORTH 2
subscriber: list subscribe CHANNEL_Z 3
subscriber: list unsubscribe X 2
publisher: 1
subscriber: list message CHANNEL_Z BEEEZZZZ
publisher: 1
subscriber: list message NORTH HOHOHO
publisher: 0

h2. Loading datasets from .csv files

p. It is also possible to load datasets from csv files in a puredis object.  The puredis object won't output anything while loading except for Redis error messages to the pd console.  A status message is sent when loading completes.

!https://github.com/lp/puredis/raw/master/img/puredis-csv-help.png!

p. For csv loading, your files will need to be formatted in a way puredis can understand.  (Lines preceded by # are ignored).

h4. CSV Strings:

bc. # KEY, VALUE
KEY1, VALUE1
KEY2, VALUE2
KEY3, VALUE3

h4. CSV Lists:

bc. # LIST,ITEM1,ITEM2,...
MYLIST,A,B,C,D,E
MYLIST2,F,G,H,I,J

h4. CSV Hashes:

bc. PERSON,AGE,CITY
ANNA,26,Vancouver
PIETER,32,Dublin
SERGEI,41,Moscow

h4. CSV Sets:

bc. # SET,ITEM1,ITEM2,...
MYSET1,A,B,C,D,E
MYSET2,F,G,H,I,J

h4. CSV Sorted Sets:

bc. # ZSET, SCORE1, VALUE1, SCORE2, VALUE2, ...
MYZSET1,1,A,2,B,3,C,4,D,5,E
MYZSET2,1,F,2,G,3,H,4,I,5,J

