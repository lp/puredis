local suite = Suite("redis hash suite")
suite.setup(function()
  _.outlet({"command","flushdb"})
  _.outlet({"command","HSET","MYHASH","MYKEY1","MYVALUE1"})
  _.outlet({"command","HSET","MYHASH","MYKEY2","MYVALUE2"})
  _.outlet({"command","HSET","MYHASH","MYKEY3","MYVALUE3"})
  _.outlet({"command","HSET","MYHASH2","MYCOUNT",11})
end)
suite.teardown(function()
  _.outlet({"command","flushdb"})
end)

suite.case("HSET new"
  ).test({"command","HSET","MYHASH2","MYKEY","MYVALUE"}
    ).should:equal(1)

suite.case("HSET exist"
  ).test({"command","HSET","MYHASH","MYKEY1","MYVALUE"}
    ).should:equal(0)

suite.case("HGET"
  ).test({"command","HGET","MYHASH","MYKEY1"}
    ).should:equal("MYVALUE1")

suite.case("HMSET"
  ).test(function(test)
    _.outlet({"command","HMSET","MYHASH","MYKEY4","MYVALUE4","MYKEY5","MYVALUE5"})
    test({"command","HLEN","MYHASH"})
  end).should:equal(5)

suite.case("HMGET"
  ).test({"command","HMGET","MYHASH","MYKEY2","MYKEY3"}
    ).should:resemble({"MYVALUE2","MYVALUE3"})

suite.case("HGETALL"
  ).test({"command","HGETALL","MYHASH"}
    ).should:resemble({"MYKEY1","MYVALUE1","MYKEY2","MYVALUE2","MYKEY3","MYVALUE3"})

suite.case("HKEYS"
  ).test({"command","HKEYS","MYHASH"}
    ).should:resemble({"MYKEY1","MYKEY2","MYKEY3"})

suite.case("HVALS"
  ).test({"command","HVALS","MYHASH"}
    ).should:resemble({"MYVALUE1","MYVALUE2","MYVALUE3"})

suite.case("HSETNX"
  ).test({"command","HSETNX","MYHASH","MYKEY1","MYVALUE0"}
    ).should:equal(0)

suite.case("HDEL"
  ).test({"command","HDEL","MYHASH","MYKEY1"}
    ).should:equal(1)

suite.case("HLEN"
  ).test({"command","HLEN","MYHASH"}
    ).should:equal(3)

suite.case("HEXISTS"
  ).test({"command","HEXISTS","MYHASH","MYKEY1"}
    ).should:equal(1)

suite.case("HINCRBY"
  ).test({"command","HINCRBY","MYHASH2","MYCOUNT",100}
    ).should:equal(111)
