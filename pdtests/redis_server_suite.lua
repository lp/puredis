local server_suite = pdtest.suite("redis server commands")
server_suite.setup(function()
  pdtest.raw.list({"command","flushdb"})
end)
server_suite.teardown(function()
  pdtest.raw.list({"command","flushdb"})
end)

server_suite.case("PING"
  ).test({"command","PING"}
    ).should:equal("PONG")

server_suite.case("ECHO"
  ).test({"command","ECHO","COLLE"}
    ).should:equal("COLLE")

server_suite.case("INFO"
  ).test({"command","INFO"}
    ).should:match("^redis_version")

server_suite.case("DBSIZE"
  ).test({"command","DBSIZE"}
    ).should:equal(0
  ).test(function()
    pdtest.raw.list({"command","SET","FOO","BAR"})
    pdtest.out.list({"command","DBSIZE"})
  end).should:equal(1)
  
server_suite.case("DEBUG OBJECT"
  ).test(function()
    pdtest.raw.list({"command","SET","FOO","BAR"})
    pdtest.out.list({"command","DEBUG","OBJECT","FOO"})
  end).should:match("^Value at")
  
server_suite.case("LASTSAVE"
  ).test({"command","LASTSAVE"}
    ).should:match("^%d+")
    
server_suite.case("CONFIG GET"
  ).test({"command","CONFIG","GET","timeout"}
    ).should:equal({"timeout","0"})
