#! /usr/bin/env python

from pdunit import PdUnit

pu = PdUnit(testfile="puredis-test.pd")

pu.setup(['puredis', 'command', 'SET', 'MYKEY', 'MYVALUE'])
pu.teardown(['puredis', 'command', 'flushdb'])

pu.test(
  case='SET STRING',
  test=['puredis', 'command', 'SET', 'FOO', 'BAR'],
  should=['equal', ['OK']])
  
pu.test(
  case='GET STRING',
  test=['puredis', 'command', 'GET', 'MYKEY'],
  should=['equal', ['MYVALUE']])

pu.test(
  case='STRLEN',
  test=['puredis', 'command', 'STRLEN', 'MYKEY'],
  should=['equal', ['7']])
  
pu.test(
  case='GETRANGE',
  test=['puredis', 'command', 'GETRANGE', 'MYKEY', 2, 6],
  should=['equal', ['VALUE']])

pu.test(
  case='MSET REPLY',
  test=['puredis', 'command', 'MSET', 'MYKEY2', 'MYVALUE2', 'MYKEY3', 'MYVALUE3'],
  should=['equal', ['OK']])
  
pu.test(
  case='MGET',
  setup=['puredis', 'command', 'MSET', 'MYKEY2', 'MYVALUE2', 'MYKEY3', 'MYVALUE3'],
  test=['puredis', 'command', 'MGET', 'MYKEY', 'MYKEY2', 'MYKEY3'],
  should=['equal', ['MYVALUE', 'MYVALUE2', 'MYVALUE3']])
  
pu.test(
  case='SETNX FAIL',
  test=['puredis', 'command', 'SETNX', 'MYKEY', 'MYVALUE'],
  should=['equal', ['0']])
  
pu.test(
  case='SETNX',
  test=['puredis', 'command', 'SETNX', 'MYKEY2', 'MYVALUE2'],
  should=['equal', ['1']])

pu.run()

