local suite = Suite("apuredis async list")
suite.setup(function()
  _.outlet({"command","flushdb"})
end)
suite.teardown(function()
  _.outlet({"command","flushdb"})
end)

suite.case("BLPOP"
  ).test({"command","BLPOP","mylpoplist",10}
    ).should:equal({"mylpoplist","value1"})

suite.case("BRPOP"
  ).test({"command","BRPOP","myrpoplist",10}
    ).should:equal({"myrpoplist","value1"})

suite.case("BRPOPLPUSH"
  ).test({"command","BRPOPLPUSH","myrpoppushlist","myrpoppushlist",10}
    ).should:equal("value1")