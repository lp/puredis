local suite = Suite("redis list suite")
suite.setup(function()
  _.outlet({"command","flushdb"})
  _.outlet({"command","RPUSH","MYLIST","MYVALUE1"})
  _.outlet({"command","RPUSH","MYLIST","MYVALUE2"})
  _.outlet({"command","RPUSH","MYLIST","MYVALUE3"})
end)
suite.teardown(function()
  _.outlet({"command","flushdb"})
end)

suite.case("LLEN"
  ).test({"command","LLEN","MYLIST"}
    ).should:equal(3)

suite.case("LREM"
  ).test({"command","LREM","MYLIST",0,"MYVALUE3"}
    ).should:equal(1)

suite.case("RPUSH"
  ).test({"command","RPUSH","MYLIST","MYVALUE4"}
    ).should:equal(4)

suite.case("LPUSH"
  ).test({"command","LPUSH","MYLIST","MYVALUE0"}
    ).should:equal(4)

suite.case("LPOP"
  ).test({"command","LPOP","MYLIST"}
    ).should:equal("MYVALUE1")

suite.case("RPOP"
  ).test({"command","RPOP","MYLIST"}
    ).should:equal("MYVALUE3")

suite.case("LSET"
  ).test({"command","LSET","MYLIST",0,"MYVALUE0"}
    ).should:equal("OK")

suite.case("LSET result"
  ).test(function(test)
    _.outlet({"command","LSET","MYLIST",0,"MYVALUE0"})
    test({"command","LPOP","MYLIST"})
  end).should:equal("MYVALUE0")

suite.case("RPUSHX succeed"
  ).test({"command","RPUSHX","MYLIST","MYVALUE4"}
    ).should:equal(4)

suite.case("RPUSHX fail"
  ).test({"command","RPUSHX","OTHERLIST","MYVALUE0"}
    ).should:equal(0)

suite.case("LPUSHX succeed"
  ).test({"command","LPUSHX","MYLIST","MYVALUE0"}
    ).should:equal(4)

suite.case("LPUSHX fail"
  ).test({"command","LPUSHX","OTHERLIST","MYVALUE0"}
    ).should:equal(0)

suite.case("LTRIM status"
  ).test({"command","LTRIM","MYLIST",0,-2}
    ).should:equal("OK")

suite.case("LTRIM result"
  ).test(function(test)
    _.outlet({"command","LTRIM","MYLIST",0,1})
    test({"command","LLEN","MYLIST"})
  end).should:equal(2)

suite.case("LINDEX"
  ).test({"command","LINDEX","MYLIST",1}
    ).should:equal("MYVALUE2")

suite.case("LINSERT"
  ).test({"command","LINSERT","MYLIST","AFTER","MYVALUE2","MYVALUE2.5"}
    ).should:equal(4)

suite.case("LRANGE"
  ).test({"command","LRANGE","MYLIST","0","-1"}
    ).should:equal({"MYVALUE1","MYVALUE2","MYVALUE3"})

suite.case("RPOPLPUSH"
  ).test({"command","RPOPLPUSH","MYLIST","MYLIST"}
    ).should:equal("MYVALUE3")