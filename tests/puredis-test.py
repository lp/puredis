#! /usr/bin/env python

from pdunit import PdUnit

pu = PdUnit(testfile="puredis-test.pd")
  
pu.test(
  case='SET STRING',
  test=['puredis', 'command', 'SET', 'FOO', 'BAR'],
  should=['equal', ['OK']])
  
pu.test(
  case='GET STRING',
  test=['puredis', 'command', 'GET', 'FOO'],
  should=['equal', ['BAR']])

pu.test(
  case='STRLEN',
  test=['puredis', 'command', 'STRLEN', 'FOO'],
  should=['equal', ['3']])

pu.run()

