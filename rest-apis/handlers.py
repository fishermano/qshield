#!/usr/bin/env python3
# -*- coding: utf-8 -*-

__author__ = 'CYX'

from coroweb import get, post

from apis import APIError, APIValueError, APIResourceNotFoundError, APIPermissionError
from models import Rankings


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


@get('/api/sql')
def sql(*, st, p, tk):
	print("st = %s; p = %s; tk = %s" % (st, p, tk))

@post('/qshield/query')
async def qshield_query(*, st, p, tk, **kw):
	if st is None:
		raise APIValueError('st', message = 'st is None')

	if st is None:
		raise APIValueError('p', message = 'p is None')

	if tk is None:
		raise APIValueError('tk', message = 'tk is None')

	await Rankings.exe(st=st, p=p, tk=tk, **kw)
