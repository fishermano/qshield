#!/usr/bin/env python3
# -*- coding: utf-8 -*-

__author__ = 'CYX'

import asyncio, sys , logging
logging.basicConfig(level = logging.INFO)

from field import *

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
        __config = SparkConf().setAll([('spark.jars', kw.get('jars','opaque-ext_2.11-0.1.jar,data-owner_2.11-0.1.jar')), ('spark.debug.maxToStringFields', '1000'), ('spark.driver.memory', '2g'), ('spark.executor.memory', '8g')])
        __spark = SparkSession.builder.appName(kw.get('app_name', 'qshield')).master(kw.get('master', 'localhost')).config(conf=__config).getOrCreate()
        __sqlContext = SQLContext(__spark.sparkContext)

        __spark._jvm.edu.xjtu.cs.cyx.qshield.QShieldUtils.initQShieldSQLContext(__sqlContext._jsqlContext)
    except Exception as e:
        logging.info('init_sql_ra_context() error: %s' % str(e))
        sys.exit()

def table_schema(t_model):
    sfs = []
    for k, v in t_model.__mappings__.items():
        if isinstance(v, StringField):
            sfs.append(StructField(k, StringType(), True))
        elif isinstance(v, IntegerField):
            sfs.append(StructField(k, IntegerType(), True))
        elif isinstance(v, FloatField):
            sfs.append(StructField(k, FloatType(), True))
        elif isinstance(v, DateField):
            sfs.append(StructField(k, DateType(), True))
    return sfs

async def spark_sql_exe(objs, st, p, tk):

    global __spark
    global __sqlContext

    for obj in objs:
        df = __spark.read.format("edu.berkeley.cs.rise.opaque.EncryptedSource") \
                                .schema(obj.schema) \
                                .load(obj.path)
        qdf = __spark._jvm.org.apache.spark.sql.QShieldDatasetFunctions(df._jdf)
        qdfAC = qdf.acPolicyApplied(tk)
        dfAC = DataFrame(qdfAC, __sqlContext)
        dfAC.createOrReplaceTempView(obj.name)

    dfsql = __spark.sql(st)

    qres = __spark._jvm.org.apache.spark.sql.QShieldDatasetFunctions(dfsql._jdf)
    qresPrep = qres.resPrepared()
    resPrep = DataFrame(qresPrep, __sqlContext)
    coll_fur = await asyncio.wrap_future(resPrep.collectAsync())
    return coll_fur

async def spark_filter(obj, st, tk):

    global __spark
    global __sqlContext

    df = __spark.read.format("edu.berkeley.cs.rise.opaque.EncryptedSource") \
                            .schema(obj.schema) \
                            .load(obj.path)
    qdf = __spark._jvm.org.apache.spark.sql.QShieldDatasetFunctions(df._jdf)
    qdfAC = qdf.acPolicyApplied(tk)
    dfAC = DataFrame(qdfAC, __sqlContext)

    dffilter = dfAC.filter(st)

    qres = __spark._jvm.org.apache.spark.sql.QShieldDatasetFunctions(dffilter._jdf)
    qresPrep = qres.resPrepared()
    resPrep = DataFrame(qresPrep, __sqlContext)
    coll_fur = await asyncio.wrap_future(resPrep.collectAsync())
    return coll_fur

async def spark_selector(obj, st, tk):

    global __spark
    global __sqlContext

    df = __spark.read.format("edu.berkeley.cs.rise.opaque.EncryptedSource") \
                            .schema(obj.schema) \
                            .load(obj.path)
    qdf = __spark._jvm.org.apache.spark.sql.QShieldDatasetFunctions(df._jdf)
    qdfAC = qdf.acPolicyApplied(tk)
    dfAC = DataFrame(qdfAC, __sqlContext)

    cols = st.split(',')
    cols_stripped = []
    for col in cols:
        cols_stripped.append(col.strip())
    dfproj = dfAC.select(cols_stripped)

    qres = __spark._jvm.org.apache.spark.sql.QShieldDatasetFunctions(dfproj._jdf)
    qresPrep = qres.resPrepared()
    resPrep = DataFrame(qresPrep, __sqlContext)
    coll_fur = await asyncio.wrap_future(resPrep.collectAsync())
    return coll_fur

async def spark_sorter(obj, st, asc, tk):

    global __spark
    global __sqlContext

    df = __spark.read.format("edu.berkeley.cs.rise.opaque.EncryptedSource") \
                            .schema(obj.schema) \
                            .load(obj.path)
    qdf = __spark._jvm.org.apache.spark.sql.QShieldDatasetFunctions(df._jdf)
    qdfAC = qdf.acPolicyApplied(tk)
    dfAC = DataFrame(qdfAC, __sqlContext)

    dfsort = None
    if asc == 'False':
        dfsort = dfAC.sort(st, ascending=False)
    else:
        dfsort = dfAC.sort(st, ascending=True)

    qres = __spark._jvm.org.apache.spark.sql.QShieldDatasetFunctions(dfsort._jdf)
    qresPrep = qres.resPrepared()
    resPrep = DataFrame(qresPrep, __sqlContext)
    coll_fur = await asyncio.wrap_future(resPrep.collectAsync())
    return coll_fur

async def spark_joiner(obj1, obj2, st, mode, tk):

    global __spark
    global __sqlContext

    df1 = __spark.read.format("edu.berkeley.cs.rise.opaque.EncryptedSource") \
                            .schema(obj1.schema) \
                            .load(obj1.path)
    qdf1 = __spark._jvm.org.apache.spark.sql.QShieldDatasetFunctions(df1._jdf)
    qdfAC1 = qdf1.acPolicyApplied(tk)
    dfAC1 = DataFrame(qdfAC1, __sqlContext)

    df2 = __spark.read.format("edu.berkeley.cs.rise.opaque.EncryptedSource") \
                            .schema(obj2.schema) \
                            .load(obj2.path)
    qdf2 = __spark._jvm.org.apache.spark.sql.QShieldDatasetFunctions(df2._jdf)
    qdfAC2 = qdf2.acPolicyApplied(tk)
    dfAC2 = DataFrame(qdfAC2, __sqlContext)

    dfjoin = dfAC1.join(dfAC2, st, mode)

    qres = __spark._jvm.org.apache.spark.sql.QShieldDatasetFunctions(dfjoin._jdf)
    qresPrep = qres.resPrepared()
    resPrep = DataFrame(qresPrep, __sqlContext)
    coll_fur = await asyncio.wrap_future(resPrep.collectAsync())
    return coll_fur

async def spark_aggregator(obj, g_c, a_c, func, tk):

    global __spark
    global __sqlContext

    df = __spark.read.format("edu.berkeley.cs.rise.opaque.EncryptedSource") \
                            .schema(obj.schema) \
                            .load(obj.path)
    qdf = __spark._jvm.org.apache.spark.sql.QShieldDatasetFunctions(df._jdf)
    qdfAC = qdf.acPolicyApplied(tk)
    dfAC = DataFrame(qdfAC, __sqlContext)

    dfagg = dfAC.groupBy(g_c).agg({a_c: func})

    qres = __spark._jvm.org.apache.spark.sql.QShieldDatasetFunctions(dfagg._jdf)
    qresPrep = qres.resPrepared()
    resPrep = DataFrame(qresPrep, __sqlContext)
    coll_fur = await asyncio.wrap_future(resPrep.collectAsync())
    return coll_fur

class DataObj(object):
    def __init__(self, name, path, schema):
        self.name = name
        self.path = path
        self.schema = schema

class ModelMetaclass(type):
    def __new__(cls, name, bases, attrs):
        if name == 'Model':
            return type.__new__(cls, name, bases, attrs)

        tableName = attrs.get('__table__', None) or name
        path = attrs.get('__path__', None) or 'NULL'
        logging.info('found model: %s (table: %s; path: %s)' % (name, tableName, path))

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
        attrs['__fields__'] = fields
        attrs['__path__'] = path

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
    async def sql_exe(cls, tables = None, st = None, p = None, tk = None, **kw):
        objs = []
        for t in tables:
            sfs = table_schema(t)
            objs.append(DataObj(t.__table__, t.__path__, StructType(sfs)))
        return await spark_sql_exe(objs, st, p, tk)

    @classmethod
    async def filter(cls, st = None, tk = None, **kw):
        sfs = table_schema(cls)
        obj = DataObj(cls.__table__, cls.__path__, StructType(sfs))
        return await spark_filter(obj, st, tk)

    @classmethod
    async def selector(cls, st = None, tk = None, **kw):
        sfs = table_schema(cls)
        obj = DataObj(cls.__table__, cls.__path__, StructType(sfs))
        return await spark_selector(obj, st, tk)

    @classmethod
    async def sorter(cls, st = None, asc = None, tk = None, **kw):
        sfs = table_schema(cls)
        obj = DataObj(cls.__table__, cls.__path__, StructType(sfs))
        return await spark_sorter(obj, st, asc, tk)

    @classmethod
    async def joiner(cls, table2 = None, st = None, mode = None, tk = None, **kw):
        sfs1 = table_schema(cls)
        obj1 = DataObj(cls.__table__, cls.__path__, StructType(sfs1))
        sfs2 = table_schema(table2)
        obj2 = DataObj(table2.__table__, table2.__path__, StructType(sfs2))
        return await spark_joiner(obj1, obj2, st, mode, tk)

    @classmethod
    async def aggregator(cls, g_c = None, a_c = None, func = None, tk = None, **kw):
        sfs = table_schema(cls)
        obj = DataObj(cls.__table__, cls.__path__, StructType(sfs))
        return await spark_aggregator(obj, g_c, a_c, func, tk)
