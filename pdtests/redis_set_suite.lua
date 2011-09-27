local suite = Suite("redis set suite")
suite.setup(function()
  _.outlet({"command","flushdb"})
  _.outlet({"command","SADD","MYSET1","M1"})
  _.outlet({"command","SADD","MYSET1","M2"})
  _.outlet({"command","SADD","MYSET1","M3"})
  _.outlet({"command","SADD","MYSET1","M4"})
  _.outlet({"command","SADD","MYSET1","M5"})
  _.outlet({"command","SADD","MYSET2","M3"})
  _.outlet({"command","SADD","MYSET2","M4"})
  _.outlet({"command","SADD","MYSET2","M5"})
  _.outlet({"command","SADD","MYSET2","M6"})
  _.outlet({"command","SADD","MYSET2","M7"})
end)
suite.teardown(function()
  _.outlet({"command","flushdb"})
end)

suite.case("SADD"
  ).test({"command","SADD","MYSET","M6"}
    ).should:equal(1)

suite.case("SPOP"
  ).test({"command","SPOP","MYSET1"}
    ).should:be_any({"M1","M2","M3","M4","M5"})

suite.case("SINTER"
  ).test({"command","SINTER","MYSET1","MYSET2"}
    ).should:resemble({"M3","M4","M5"})

suite.case("SMOVE"
  ).test({"command","SMOVE","MYSET2","MYSET1","M6"}
    ).should:equal(1)

suite.case("SUNION"
  ).test({"command","SUNION","MYSET1","MYSET2"}
    ).should:resemble({"M1","M2","M3","M4","M5","M6","M7"})

suite.case("SDIFF"
  ).test({"command","SDIFF","MYSET2","MYSET1"}
    ).should:resemble({"M6","M7"})

suite.case("SCARD"
  ).test({"command","SCARD","MYSET1"}
    ).should:equal(5)

suite.case("SISMEMBER"
  ).test({"command","SISMEMBER","MYSET1","M3"}
    ).should:equal(1)

suite.case("SRANDMEMBER"
  ).test({"command","SRANDMEMBER","MYSET1"}
    ).should:be_any({"M1","M2","M3","M4","M5"})

suite.case("SMEMBERS"
  ).test({"command","SMEMBERS","MYSET2"}
    ).should:resemble({"M3","M4","M5","M6","M7"})

suite.case("SREM"
  ).test({"command","SREM","MYSET2","M7"}
    ).should:equal(1)

suite.case("SINTERSTORE"
  ).test({"command","SINTERSTORE","MYSET3","MYSET2","MYSET1"}
    ).should:equal(3)

suite.case("SUNIONSTORE"
  ).test({"command","SUNIONSTORE","MYSET3","MYSET1","MYSET2"}
    ).should:equal(7)

suite.case("SDIFFSTORE"
  ).test({"command","SDIFFSTORE","MYSET3","MYSET2","MYSET1"}
    ).should:equal(2)