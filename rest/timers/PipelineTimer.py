#!/usr/bin/env python3
# -*- coding: utf-8 -*-

__author__ = 'CYX'

import time, asyncio, sys

from models.mongodb.mongodb_models import Logs, Items
from timers.BaseTimer import BaseTimer

from kafka import KafkaProducer
from kafka.errors import KafkaError

class PipelineTimer(BaseTimer):

	def __init__(self,howtime=1.0,enduring=True):
		BaseTimer.__init__(self,howtime,enduring)
		self.ws = 0

	@asyncio.coroutine
	def exec(self):

		results = []
		self.ws = self.ws + 1
		try:
			items = yield from Items.find()
			for _item in items:

				item = {}

				item['date'] = _item['date']
				item['logs'] = yield from Logs.find(doc = {'item_id': _item['_id']})

				results.append(item)
			print('*****************This is the %d Updated******************' % self.ws)
			
			producer = KafkaProducer(bootstrap_servers=['localhost:9092'])
			producer.send('my-topic', b'raw_bytes')
		except Exception as e:
			print(str(e))


