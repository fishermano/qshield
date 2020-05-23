#!/usr/bin/env python3
# -*- coding: utf-8 -*-

__author__ = 'CYX'

import json, time , uuid
from datetime import datetime
from datetime import date

class MyJsonEncoder(json.JSONEncoder):
	def default(self, obj):
		if isinstance(obj, datetime):
			return obj.strftime('%Y-%m-%d %H:%M:%S')
		elif isinstance(obj, date):
			return obj.strftime('%Y-%m-%d')
		else:
			return json.JSONEncoder.default(self, obj)

def next_id():
	return '%015d%s000' % (int(time.time() * 1000), uuid.uuid4().hex)

def cur_timestamp():
	return int(time.time() * 1000)

def today():
	return date.today().strftime('%Y-%m-%d')
