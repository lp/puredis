local suite = Suite("spuredis tests")

suite.case("subscribe"
  ).test({"spuredis","subscribe","TESTCHAN"}
    ).should:resemble({"subscribe","TESTCHAN","1"})
    
suite.case("publish"
  ).test({"puredis","command","publish","TESTCHAN","TESTMSG"}
    ).should:resemble({"message","TESTCHAN","TESTMSG"})
