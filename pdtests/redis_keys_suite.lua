local suite = Suite("redis keys")
suite.setup(function()
  _.outlet({"command","flushdb"})
  _.outlet({"command","SET","KEY1","VALUE1"})
  _.outlet({"command","SET","KEY2","VALUE2"})
  _.outlet({"command","SET","KEY3","VALUE3"})
end)
suite.teardown(function()
  _.outlet({"command","flushdb"})
end)

suite.case("DEL"
  ).test({"command","DEL","KEY1","KEY2","KEY3"}
    ).should:equal(3)
    
suite.case("KEYS"
  ).test({"command","KEYS","KEY*"}
    ).should:resemble({"KEY1","KEY2","KEY3"})
    
suite.case("RANDOMKEY"
  ).test({"command","RANDOMKEY"}
    ).should:be_any({"KEY1","KEY2","KEY3"})

suite.case("TTL unexpired"
  ).test({"command","TTL","KEY1"}
    ).should:equal(-1)

suite.case("EXISTS"
  ).test({"command","EXISTS","KEY1"}
    ).should:equal(1)

suite.case("RENAME"
  ).test(function(test)
    _.outlet({"command","RENAME","KEY1","KEY10"})
    test({"command","GET","KEY10"})
  end).should:equal("VALUE1")
  
suite.case("TYPE"
  ).test({"command","TYPE","KEY1"}
    ).should:equal("string")
    
suite.case("EXPIRE"
  ).test({"command","EXPIRE","KEY1","10"}
    ).should:equal(1)

suite.case("EXPIREAT"
  ).test({"command","EXPIREAT","KEY1","4320000"}
    ).should:equal(1)

suite.case("PERSIST already"
  ).test({"command","PERSIST","KEY1"}
    ).should:equal(0)

suite.case("PERSIST"
  ).test(function(test)
    _.outlet({"command","EXPIRE","KEY1","10"})
    test({"command","PERSIST","KEY1"})
  end).should:equal(1)

suite.case("TTL expired"
  ).test(function(test)
    _.outlet({"command","EXPIRE","KEY1","10"})
    test({"command","TTL","KEY1"})
  end).should:equal(10)

suite.case("OBJECT REFCOUNT"
  ).test({"command","OBJECT","REFCOUNT","KEY1"}
    ).should:be_more(0)

suite.case("OBJECT ENCODING"
  ).test({"command","OBJECT","ENCODING","KEY1"}
    ).should:be_any({"raw","ziplist"})

suite.case("OBJECT IDLETIME"
  ).test({"command","OBJECT","IDLETIME","KEY1"}
    ).should:be_more_or_equal(0)

suite.case("RENAMENX fail!"
  ).test({"command","RENAMENX","KEY1","KEY2"}
    ).should:equal(0)

suite.case("RENAMENX succeed!"
  ).test(function(test)
    _.outlet({"command","RENAMENX","KEY1","KEY10"})
    test({"command","GET","KEY10"})
  end).should:equal("VALUE1")

suite.case("SORT DESC"
  ).test(function(test)
    _.outlet({"command","RPUSH","MYLIST","0"})
    _.outlet({"command","RPUSH","MYLIST","1"})
    _.outlet({"command","RPUSH","MYLIST","1"})
    _.outlet({"command","RPUSH","MYLIST","2"})
    _.outlet({"command","RPUSH","MYLIST","3"})
    test({"command","SORT","MYLIST","DESC"})
  end).should:equal({"3","2","1","1","0"})

suite.case("SORT LIMIT"
  ).test(function(test)
    _.outlet({"command","RPUSH","MYLIST","0"})
    _.outlet({"command","RPUSH","MYLIST","1"})
    _.outlet({"command","RPUSH","MYLIST","1"})
    _.outlet({"command","RPUSH","MYLIST","2"})
    _.outlet({"command","RPUSH","MYLIST","3"})
    test({"command","SORT","MYLIST","LIMIT","2","2"})
  end).should:equal({"1","2"})
