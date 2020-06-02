#!/usr/bin/env python3
# -*- coding: utf-8 -*-

__author__ = 'CYX'

import asyncio, sys , logging
logging.basicConfig(level = logging.INFO)

from field import Field

from pyspark import SparkConf
from pyspark.sql import SparkSession
from pyspark.sql import SQLContext
from pyspark.sql import DataFrame
from pyspark.sql.types import *

import asyncactions

def init_sql_ra_context(**kw):
    logging.info('initialize qshield context and launch remote attesation ... ')

    global __config
    global __spark
    global __sqlContext

    try:
        __config = SparkConf().setAll([('spark.jars', kw.get('jars','opaque-ext_2.11-0.1.jar,data-owner_2.11-0.1.jar'))])
        __spark = SparkSession.builder.appName(kw.get('app_name', 'qshield')).master(kw.get('master', 'localhost')).config(conf=__config).getOrCreate()
        __sqlContext = SQLContext(__spark.sparkContext)

        __spark._jvm.edu.xjtu.cs.cyx.qshield.QShieldUtils.initQShieldSQLContext(__sqlContext._jsqlContext)
    except Exception as e:
        logging.info('init_sql_ra_context() error: %s' % str(e))
        sys.exit()

def fur_call_back(fur):
    for row in fur.result():
        logging.info('Has row: word = %s, count = %d' % (row['word'], row['count']))

async def spark_sql_exe():

    global __spark
    global __sqlContext

    # data = [("foo", 4), ("bar", 1), ("baz",5)]
    # df = __spark.createDataFrame(data).toDF("word", "count")
    # opaqueDF = __spark._jvm.org.apache.spark.sql.OpaqueDatasetFunctions(df._jdf)
    # opaqueDFEnc = opaqueDF.encrypted()
    # dfEnc = DataFrame(opaqueDFEnc, __sqlContext)
    # coll_fur = await asyncio.wrap_future(dfEnc.collectAsync())
    # return coll_fur

    df = __spark.read.format("edu.berkeley.cs.rise.opaque.EncryptedSource").schema(StructType([StructField("word", StringType(), True), StructField("count", IntegerType(), True)])).load("dfEncrypted")
    qdf = __spark._jvm.org.apache.spark.sql.QShieldDatasetFunctions(df._jdf)
    qdfAC = qdf.acPolicyApplied(bytearray())
    dfAC = DataFrame(qdfAC, __sqlContext)
    coll_fur = await asyncio.wrap_future(dfAC.collectAsync())
    return coll_fur

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
        return value

    @classmethod
    async def exe(cls, sql = None, where = None, args = None, **kw):

        global __spark
        global __sqlContext

        sql_exe = sql

        if sql_exe is None:
            raise ValueError('Invalid sql statement: %s' % sql_exe)

        if where:
            if where.startswith('inner join'):
                sql_exe.append(where)
            else:
                sql_exe.append('where')
                sql_exe.append(where)

        if args is None:
            args_exe = []

        orderby = kw.get('orderby', None)
        if orderby:
            sql_exe.append('order by')
            sql_exe.append(orderby)
        limit = kw.get('limit', None)
        if limit:
            sql_exe.append('limit')
            if isinstance(limit, int):
                sql_exe.append('?')
                args_exe.append(limit)
            elif isinstance(limit, tuple) and len(limit) == 2:
                sql_exe.append('?, ?')
                args_exe.extend(limit)
            else:
                raise ValueError('Invalid limit value: %s' % str(limit))

        logging.info('SQL statement: %s' % sql_exe)

        res = await spark_sql_exe()
        for row in res:
            logging.info('Has row: word = %s, count = %d' % (row['word'], row['count']))
