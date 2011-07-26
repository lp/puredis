from time import time
from Pd import Pd
import sys

green = '\033[01;32m'
red = '\033[01;31m'
white = '\033[0m'
blue = '\033[94m'
orange = '\033[93m'

class PdUnit:
  
  def __init__(self,testfile):
    self.testfile = testfile
    self.suite = []
    self.pd = Pd(nogui=False, open=testfile)
    self.start = time()
    self.results = {}
    self.stats = {'success': 0, 'fail': 0}
    self.before = False
    self.after = False
    
  def print_stats(self):
    if self.stats['fail'] == 0:
      self.print_color(blue, "All %i tests passed!" % self.stats['success'])
    else:
      self.print_color(orange, "Oups!  %i tests failed" % self.stats['fail'])
      self.print_color(blue, "%i tests still passed successfully" % self.stats['success']) 
    
  def print_color(self,color,data):
    print color+(data)+white
    
  def report(self,name):
    if self.results[name]['status']:
      self.print_color(green, "\t%s -> OK" % name)
      self.stats['success'] += 1
    else:
      self.print_color(red, "\t%s >> FAIL!!" % name)
      self.print_color(red, ', '.join( self.results[name]['test']))
      self.print_color(red, "not '%s' to:" % self.results[name]['should'][0])
      self.print_color(red, ', '.join( self.results[name]['should'][1]))
      self.stats['fail'] += 1
  
  def test(self,case,should,test,setup=False,teardown=False):
    self.suite.append({'case': case, 'should': should, 'test': test, 'setup': setup, 'teardown': teardown})
    self.results[case] = {'should': should}
  
  def setup(self,setup):
    self.before = setup
    
  def teardown(self,teardown):
    self.after = teardown
  
  def assertion(self,message):
    if message[0] == 'idx' and message[1] != '-1':
      idx = int(message[1])
      test = message[2:]
      case = self.suite[int(idx)]['case']
      self.results[case]['test'] = test[1:]
      if self.results[case]['should'][0] == 'equal':
        self.results[case]['status'] = self.results[case]['should'][1] == self.results[case]['test']
      
      self.report(case)
  
  def run(self):
    caller_self = self
    def Pd_unit(self,message):
      caller_self.assertion(message)
    
    self.pd.Pd_unit = Pd_unit
    for unit in self.suite:
      # sending the setup if there is
      if self.before:
        self.pd.Send(['idx', '-1'])
        self.pd.Send(self.before)
      if unit['setup']:
        self.pd.Send(['idx', '-1'])
        self.pd.Send(unit['setup'])
      # sending the actual tests
      self.pd.Send(['idx', str(self.suite.index(unit))])
      self.pd.Send(unit['test'])
      # sending the teardowns
      if unit['teardown']:
        self.pd.Send(['idx', '-1'])
        self.pd.Send(unit['teardown'])
      if self.after:
        self.pd.Send(['idx', '-1'])
        self.pd.Send(self.after)
      
    while self.pd.Alive():
      if len(self.suite) <= (self.stats['success']+self.stats['fail']):
        self.pd.Send(["exit"])
      self.pd.Update()
    
    if self.pd.Alive():
      self.pd.Exit()
    self.print_stats()



      