local suite = Suite("apuredis async list")
suite.setup(function()
  _.outlet({"command","flushdb"})
end)
suite.teardown(function()
  _.outlet({"command","flushdb"})
end)

suite.case("BLPOP"
  ).test({"command","BLPOP","mylist",0}
    ).should:equal({"mylist","value1"})