local suite = Suite("redis string suite")
suite.setup(function()
  _.outlet.list({"command","SET","KEY1","VALUE1"})
  _.outlet.list({"command","SET","KEY2","VALUE2"})
end)
suite.teardown(function()
  _.outlet.list({"command","flushdb"})
end)

suite.case("APPEND"
  ).test({"command","APPEND","KEY1","_EXTRA"}
    ).should:equal(12)
