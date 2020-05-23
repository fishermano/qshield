#!/usr/bin/env python3
# -*- coding: utf-8 -*-

__author__ = 'CYX'

import copy
from datetime import *
from aiohttp import web

import gl
from coroweb import get, post
from format import data_res_format, items_num_format, repo_items_num_format, repo_data_res_format
from apis import APIError, APIValueError, APIResourceNotFoundError, APIPermissionError
from models.mysql.models import SR_News_Result
from tools import next_id, cur_timestamp, today


@get('/')
def all_apis():
	test_apis = []
	test_apis.append('Hello: GET /api/hello/:name')
	test_apis.append('Hello: GET /api/hello?name=CX&age=25')
	test_apis.append('Hello: GET /api/hello/multi-keys?name=CX&gender=male&age=25')

	logs_apis = []
	logs_apis.append('Insert Item: POST /api/logs/items')
	logs_apis.append('Retrive Items: GET /api/logs/items?key1=value1&key2=value2')

	all_apis = {'Test':test_apis, 'Log': logs_apis}
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


@post('/api/hot_topics/items')
def hot_topics_insert_items(*, table, item):
	if not isinstance(item, dict):
		raise APIValueError('data', message = 'data\'s type should be a dict')

	if table is None:
		raise APIValueError('data', message = 'table\'s value should be defined')

	iid = next_id()
	item_obj = None
	if table == 'sr_news_result':
		item_obj = SR_News_Result(id = iid, **item)
		yield from item_obj.save()
		return item_obj.id

@get('/api/hot_topics/items/{id}')
def hot_topics_retrive_items(*, id, **kw):
	if kw:
		items = []

		orderby = kw.get('orderby', None)
		if orderby:
			kw.pop('orderby')

		p = kw.get('p', None)
		if p:
			kw.pop('p')

		pcount = kw.get('pcount', None)
		if pcount:
			kw.pop('pcount')

		limit = None
		if p and pcount:
			try:
				p_ = int(p)
				pcount_ = int(pcount)
			except ValueError as e:
				return data_res_format(code = 500, ok = False, msg = str(e))
			start_item_num = (p_ - 1) * pcount_
			limit = (start_item_num, pcount_)

		kw_other = {
			'orderby': orderby,
			'limit': limit
		}

		try:
			topic_item = yield from SR_Topics_Result.find(id)

			if topic_item:
				item = {}

				item['id'] = topic_item['id']
				item['keywords'] = topic_item['keywords']
				item['summary'] = topic_item['summary']
				item['doc_num'] = topic_item['doc_num']
				item['topic_time'] = topic_item['topic_time']

				where_str = 'related_topic = ?'
				args_list = []
				args_list.append(topic_item['id'])
				item['related_news'] = yield from SR_News_Result.findAll(where = where_str, args = args_list, **kw_other)

				items.append(item)

			return data_res_format(p = p, pcount = pcount, data = items)

		except Exception as e:
			return data_res_format(code = 500, ok = False, msg = str(e))

	else:
		items = []
		try:
			topic_item = yield from SR_Topics_Result.find(id)

			if topic_item:
				item = {}

				item['id'] = topic_item['id']
				item['keywords'] = topic_item['keywords']
				item['summary'] = topic_item['summary']
				item['doc_num'] = topic_item['doc_num']
				item['topic_time'] = topic_item['topic_time']

				where_str = 'related_topic = ?'
				args_list = []
				args_list.append(topic_item['id'])
				item['related_news'] = yield from SR_News_Result.findAll(where = where_str, args = args_list)

				items.append(item)

			return data_res_format(data = items)
		except Exception as e:
			return data_res_format(code = 500, ok = False, msg = str(e))
