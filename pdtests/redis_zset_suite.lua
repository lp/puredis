local suite = Suite("redis zset suite")
suite.setup(function()
  _.outlet({"command","flushdb"})
  _.outlet({"command","ZADD","MYZSET1",1,"M1"})
  _.outlet({"command","ZADD","MYZSET1",2,"M2"})
  _.outlet({"command","ZADD","MYZSET1",3,"M3"})
  _.outlet({"command","ZADD","MYZSET1",4,"M4"})
  _.outlet({"command","ZADD","MYZSET1",5,"M5"})
  _.outlet({"command","ZADD","MYZSET2",3,"M3"})
  _.outlet({"command","ZADD","MYZSET2",4,"M4"})
  _.outlet({"command","ZADD","MYZSET2",5,"M5"})
  _.outlet({"command","ZADD","MYZSET2",6,"M6"})
  _.outlet({"command","ZADD","MYZSET2",7,"M7"})
end)
suite.teardown(function()
  _.outlet({"command","flushdb"})
end)

suite.case("ZADD"
  ).test({"command","ZADD","MYZSET1",6,"M6"}
    ).should:equal(1)

suite.case("ZREM"
  ).test({"command","ZREM","MYZSET1","M2"}
    ).should:equal(1)

suite.case("ZREMRANGEBYRANK"
  ).test({"command","ZREMRANGEBYRANK","MYZSET1",0,3}
    ).should:equal(4)

suite.case("ZREMRANGEBYSCORE"
  ).test({"command","ZREMRANGEBYSCORE","MYZSET2",0,4}
    ).should:equal(2)

suite.case("ZCARD"
  ).test({"command","ZCARD","MYZSET2"}
    ).should:equal(5)

suite.case("ZCOUNT"
  ).test({"command","ZCOUNT","MYZSET2",5,"+inf"}
    ).should:equal(3)

suite.case("ZSCORE"
  ).test({"command","ZSCORE","MYZSET2","M5"}
    ).should:equal("5")

suite.case("ZRANK"
  ).test({"command","ZRANK","MYZSET1","M3"}
    ).should:equal(2)

suite.case("ZREVRANK"
  ).test({"command","ZREVRANK","MYZSET1","M2"}
    ).should:equal(3)

suite.case("ZINCRBY"
  ).test({"command","ZINCRBY","MYZSET2",2.5,"M7"}
    ).should:equal("9.5")

suite.case("ZRANGE"
  ).test({"command","ZRANGE","MYZSET1",3,-1,"WITHSCORES"}
    ).should:resemble({"M4","4","M5","5"})

suite.case("ZRANGEBYSCORE"
  ).test({"command","ZRANGEBYSCORE","MYZSET1",2,4,"WITHSCORES"}
    ).should:equal({"M2","2","M3","3","M4","4"})

suite.case("ZREVRANGE"
  ).test({"command","ZREVRANGE","MYZSET1",3,-1,"WITHSCORES"}
    ).should:equal({"M2","2","M1","1"})

suite.case("ZREVRANGEBYSCORE"
  ).test({"command","ZREVRANGEBYSCORE","MYZSET1",4,2,"WITHSCORES"}
    ).should:equal({"M4","4","M3","3","M2","2"})

suite.case("ZINTERSTORE"
  ).test({"command","ZINTERSTORE","MYZSET3",2,"MYZSET1","MYZSET2"}
    ).should:equal(3)

suite.case("ZUNIONSTORE"
  ).test({"command","ZUNIONSTORE","MYZSET3",2,"MYZSET1","MYZSET2"}
    ).should:equal(7)