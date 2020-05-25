#!/usr/bin/env python3
# -*- coding: utf-8 -*-

__author__ = 'CYX'

from coroweb import get, post

from apis import APIError, APIValueError, APIResourceNotFoundError, APIPermissionError
from models.qshield.models import Test


@get('/')
def all_apis():
	test_apis = []
	test_apis.append('Hello: GET /api/hello/:name')
	test_apis.append('Hello: GET /api/hello?name=CX&age=25')
	test_apis.append('Hello: GET /api/hello/multi-keys?name=CX&gender=male&age=25')
	test_apis.append('Test: GET /api/test/:sql')

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

@get('/api/test/{sql}')
def test(*, sql, **kw):
	where_str = ''
	if kw:
		for k, v in kw.items():
			if k == 'where':
				where_str = v
				kw.pop('where')

	Test.exe(sql=sql, where=where_str, args=None, **kw)
