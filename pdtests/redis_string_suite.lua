local suite = Suite("redis string suite")
suite.setup(function()
  _.outlet({"command","SET","KEY1","VALUE1"})
  _.outlet({"command","SET","KEY2","VALUE2"})
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

suite.case("MSET OK"
  ).test({"command","MSET","KEY3","VALUE3","KEY4","VALUE4"}
    ).should:equal("OK")

suite.case("MSET result"
  ).test(function(test)
    _.outlet({"command","MSET","KEY3","VALUE3","KEY4","VALUE4"})
    test({"command","KEYS", "*"})
  end).should:resemble({"KEY1","KEY2","KEY3","KEY4"})

suite.case("SETNX fail"
  ).test({"command","SETNX","KEY1","ANYVALUE"}
    ).should:equal(0)

suite.case("SETNX succeed"
  ).test({"command","SETNX","KEY3","VALUE3"}
    ).should:equal(1)

suite.case("DECR"
  ).test(function(test)
    _.outlet({"command","SET","COUNT","23"})
    test({"command","DECR","COUNT"})
  end).should:equal(22)

suite.case("INCR"
  ).test(function(test)
    _.outlet({"command","SET","COUNT","23"})
    test({"command","INCR","COUNT"})
  end).should:equal(24)

suite.case("GETSET"
  ).test({"command","GETSET","KEY1","NEWVALUE"}
    ).should:equal("VALUE1")

suite.case("MSETNX fail"
  ).test({"command","MSETNX","KEY3","VALUE3","KEY1","NEWVALUE"}
    ).should:equal(0)

suite.case("MSETNX ok"
  ).test({"command","MSETNX","KEY3","VALUE3","KEY4","VALUE4"}
    ).should:equal(1)
