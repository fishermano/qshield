#!/usr/bin/env python3
# -*- coding: utf-8 -*-

__author__ = 'CYX'

import logging
logging.basicConfig(level = logging.INFO)
from coroweb import get, post

from apis import APIError, APIValueError, APIResourceNotFoundError, APIPermissionError
from models import *

import sqlparse
from sqlparse.sql import IdentifierList, Identifier, Where
from sqlparse.tokens import Keyword, DML

from format import data_res_format

def table_model(table_identifier):
	model = None
	if table_identifier == 'RANKINGS':
		model = Rankings()
	elif table_identifier == 'USERVISITS':
		model = Uservisits()
	else:
		raise APIValueError('st', message = r"'%s' Model is not defined" % i)
	return model

@get('/')
def all_apis():
	test_apis = []
	test_apis.append('Hello: GET /api/hello/:name')
	test_apis.append('Hello: GET /api/hello?name=CX&age=25')
	test_apis.append('Hello: GET /api/hello/multi-keys?name=CX&gender=male&age=25')
	test_apis.append('QueryRequest: POST /qshield/query')

	all_apis = {'Test':test_apis}
	return {
		'__template__': 'apis.html',
		'all_apis': all_apis
	}

@get('/api/hello/{name}')
def index(*, name):
	return {
		'__template__': 'default.html',
		'name': name
	}

@get('/api/hello')
def hello_named_key(*, name, age):
	name = name + ', and you are ' + age + ' years old'
	return {
		'__template__': 'default.html',
		'name': name
	}

@get('/api/hello/multi-keys')
def hello_multi_key(**kw):
	name = ''
	i = 0
	for k, v in kw.items():
		if i == 0:
			name = name + k + ' = ' +v
			i = 1
			continue
		name = name + ' , ' + k + ' = ' + v

	return {
		'__template__': 'default.html',
		'name': name
	}

# apis for multiple tables
def is_subselect(parsed):
    if not parsed.is_group:
        return False
    for item in parsed.tokens:
        if item.ttype is DML and item.value.upper() == 'SELECT':
            return True
    return False

def extract_from_part(parsed):
    from_seen = False
    for item in parsed.tokens:
        if from_seen:
            if is_subselect(item):
                for x in extract_from_part(item):
                    yield x
            elif item.ttype is Keyword:
                raise StopIteration
            else:
                yield item
        elif item.ttype is Keyword and item.value.upper() == 'FROM':
            from_seen = True

def extract_table_identifiers(token_stream):
    for item in token_stream:
        if isinstance(item, IdentifierList):
            for identifier in item.get_identifiers():
                yield identifier.get_name()
        elif isinstance(item, Identifier):
            yield item.get_name()
        # It's a bug to check for Keyword here, but in the example
        # above some tables names are identified as keywords...
        elif item.ttype is Keyword:
            yield item.value

def extract_tables(sql):
    stream = extract_from_part(sqlparse.parse(sql)[0])
    return list(extract_table_identifiers(stream))

@post('/qshield/query')
async def qshield_query(*, st, p, tk, **kw):
	if st is None:
		raise APIValueError('st', message = 'st is None')

	if p is None:
		raise APIValueError('p', message = 'p is None')

	if tk is None:
		raise APIValueError('tk', message = 'tk is None')

	table_identifiers = extract_tables(st)
	if table_identifiers is None:
	    raise APIValueError('st', message = r"cannot extract tables from '%s'" % st)
	else:
	    logging.info('found target tables: %s' % ', '.join(table_identifiers))
	tables = []
	for i in table_identifiers:
		tables.append(table_model(i))

	res = await Tables.sql_exe(tables=tables, st=st, p=p, tk=tk, **kw)
	return data_res_format(data = res)

@post('/qshield/filter')
async def qshield_filter(*, t, st, tk, **kw):
	table = None
	if t is None:
		raise APIValueError('t', message = 't is None')
	else:
		table = table_model(t)

	if st is None:
		raise APIValueError('st', message = 'st is None')

	if tk is None:
		raise APIValueError('tk', message = 'tk is None')

	res = await table.filter(st=st, tk=tk, **kw)
	return data_res_format(data = res)

@post('/qshield/selector')
async def qshield_selector(*, t, st, tk, **kw):
	table = None
	if t is None:
		raise APIValueError('t', message = 't is None')
	else:
		table = table_model(t)

	if st is None:
		raise APIValueError('st', message = 'st is None')

	if tk is None:
		raise APIValueError('tk', message = 'tk is None')

	res = await table.selector(st=st, tk=tk, **kw)
	return data_res_format(data = res)

@post('/qshield/sorter')
async def qshield_sorter(*, t, st, asc, tk, **kw):
	table = None
	if t is None:
		raise APIValueError('t', message = 't is None')
	else:
		table = table_model(t)

	if st is None:
		raise APIValueError('st', message = 'st is None')

	if asc is None:
		raise APIValueError('asc', message = 'asc is None')

	if tk is None:
		raise APIValueError('tk', message = 'tk is None')

	res = await table.sorter(st=st, asc=asc, tk=tk, **kw)
	return data_res_format(data = res)

@post('/qshield/joiner')
async def qshield_joiner(*, t1, t2, st, mode, tk, **kw):
	table1 = None
	if t1 is None:
		raise APIValueError('t1', message = 't1 is None')
	else:
		table1 = table_model(t1)

	table2 = None
	if t2 is None:
		raise APIValueError('t2', message = 't2 is None')
	else:
		table2 = table_model(t2)

	if st is None:
		raise APIValueError('st', message = 'st is None')

	if mode is None:
		raise APIValueError('mode', message = 'mode is None')

	if tk is None:
		raise APIValueError('tk', message = 'tk is None')

	res = await table1.joiner(table2=table2, st=st, mode=mode, tk=tk, **kw)
	return data_res_format(data = res)
