#!/usr/bin/env python3
# -*- coding: utf-8 -*-

__author__ = 'CYX'


from abc import ABCMeta, abstractmethod
import asyncio

class BaseTimer:

	__metaclass__ = ABCMeta
	
	def __init__(self, howtime=1.0, enduring=True):
		
		"""
			howtime: time for each loop
			enduring: whether is closed
		"""
		
		self.enduring = enduring
		self.howtime = howtime

	@asyncio.coroutine
	def run(self):
		yield from asyncio.sleep(self.howtime)
		yield from self.exec()
		while self.enduring:
			yield from asyncio.sleep(self.howtime)
			yield from self.exec()

	@abstractmethod
	@asyncio.coroutine
	def exec(self):
		pass
	
	def destroy(self):
		self.enduring = False
		del self

	def stop(self):
		self.enduring = False
	
	def restart(self):
		self.enduring = True
	
	def get_status(self):
		return self.enduring
