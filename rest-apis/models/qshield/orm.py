#!/usr/bin/env python3
# -*- coding: utf-8 -*-

__author__ = 'CYX'

import asyncio, sys , logging
logging.basicConfig(level = logging.INFO)

from field import Field

from pyspark import SparkConf
from pyspark.sql import SparkSession
from pyspark.sql import SQLContext

def init_sql_ra_context(**kw):
    logging.info('initialize opaque context and launch remote attesation ... ')

    global __config
    global __spark
    global __sqlContext

    try:
        __config = SparkConf().setAll([('spark.jars', kw.get('jars','opaque_2.11-0.1.jar'))])
        __spark = SparkSession.builder.appName(kw.get('app_name', 'qshield')).master(kw.get('master', 'localhost')).config(conf=__config).getOrCreate()
        __sqlContext = SQLContext(__spark.sparkContext)

        __spark._jvm.edu.berkeley.cs.rise.opaque.Utils.initSQLContext(__sqlContext._jsqlContext)
    except Exception as e:
        logging.info('init_sql_ra_context() error: %s' % str(e))
        sys.exit()

class ModelMetaclass(type):
	def __new__(cls, name, bases, attrs):
		if name == 'Model':
			return type.__new__(cls, name, bases, attrs)

		tableName = attrs.get('__table__', None) or name
		logging.info('found model: %s (table: %s)' % (name, tableName))

		mappings = dict()
		fields = []
		for k, v in attrs.items():
			if isinstance(v, Field):
				logging.info(' found mapping: %s ==> %s' % (k, v))
				mappings[k] = v
				fields.append(k)
		for k in mappings.keys():
			attrs.pop(k)

		attrs['__mappings__'] = mappings
		attrs['__table__'] = tableName
		#attrs['__primary_key__'] = primaryKey
		attrs['__fields__'] = fields


		return type.__new__(cls, name, bases, attrs)

class Model(dict, metaclass=ModelMetaclass):
	def __init__(self, **kw):
		super(Model, self).__init__(**kw)

	def __getattr__(self, key):
		try:
			return self[key]
		except KeyError:
			raise AttributeError(r"'Model' object has no attribute '%s'" % key)

	def __setattr__(self, key, value):
		self[key] = value

	def getValue(self, key):
		return getattr(self, key, None)

	def getValueOrDefault(self, key):
		value = getattr(self, key, None)
		if value is None:
			field = self.__mappings__[key]
			if field.default is not None:
				value = field.default() if callable(field.default) else field.default
				logging.debug('using default value for %s: %s' % (key, str(value)))
				setattr(self, key, value)
		return value
