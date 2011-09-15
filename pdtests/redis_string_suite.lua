local suite = Suite("redis string suite")
suite.setup(function()
  _.outlet({"command","SET","KEY1","VALUE1"})
  _.outlet({"command","SET","KEY2","VALUE2"})
  _.outlet({"command","SET","COUNT","23"})
  _.outlet({"command","SET","BITS","0101"})
end)
suite.teardown(function()
  _.outlet({"command","flushdb"})
end)

suite.case("APPEND"
  ).test({"command","APPEND","KEY1","_EXTRA"}
    ).should:equal(12)

suite.case("GETRANGE"
  ).test({"command","GETRANGE","KEY1",2,-1}
    ).should:equal("LUE1")

suite.case("SETRANGE"
  ).test({"command","SETRANGE","KEY1",5,"23"}
    ).should:equal(7)

suite.case("MSET OK"
  ).test({"command","MSET","KEY3","VALUE3","KEY4","VALUE4"}
    ).should:equal("OK")

suite.case("MSET result"
  ).test(function(test)
    _.outlet({"command","MSET","KEY3","VALUE3","KEY4","VALUE4"})
    test({"command","KEYS", "KEY*"})
  end).should:resemble({"KEY1","KEY2","KEY3","KEY4"})

suite.case("MGET"
  ).test({"command","MGET","KEY1","KEY2","dummy"}
    ).should:equal({"VALUE1","VALUE2","nil"})

suite.case("SETNX fail"
  ).test({"command","SETNX","KEY1","ANYVALUE"}
    ).should:equal(0)

suite.case("SETNX succeed"
  ).test({"command","SETNX","KEY3","VALUE3"}
    ).should:equal(1)

suite.case("DECR"
  ).test({"command","DECR","COUNT"}
    ).should:equal(22)

suite.case("INCR"
  ).test({"command","INCR","COUNT"}
    ).should:equal(24)

suite.case("DECRBY"
  ).test({"command","DECRBY","COUNT", 3}
    ).should:equal(20)

suite.case("INCRBY"
  ).test({"command","INCRBY","COUNT",4}
    ).should:equal(27)

suite.case("GETSET"
  ).test({"command","GETSET","KEY1","NEWVALUE"}
    ).should:equal("VALUE1")

suite.case("MSETNX fail"
  ).test({"command","MSETNX","KEY3","VALUE3","KEY1","NEWVALUE"}
    ).should:equal(0)

suite.case("MSETNX ok"
  ).test({"command","MSETNX","KEY3","VALUE3","KEY4","VALUE4"}
    ).should:equal(1)

suite.case("STRLEN"
  ).test({"command","STRLEN","KEY1"}
    ).should:equal(6)

suite.case("SETBIT"
  ).test({"command","SETBIT","BITS",1,0}
    ).should:equal(0)

suite.case("GETBIT"
  ).test({"command","GETBIT","BITS",2}
    ).should:equal(1)

suite.case("SETEX"
  ).test(function(test)
    _.outlet({"command","SETEX","KEY3",44,"VALUE3"})
    test({"command","TTL","KEY3"})
  end).should:equal(44)