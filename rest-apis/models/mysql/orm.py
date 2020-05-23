#!/usr/bin/env python3
# -*- coding: utf-8 -*-

__author__ = 'CYX'

import asyncio, aiomysql, sys , logging
logging.basicConfig(level = logging.INFO)

from field import Field

@asyncio.coroutine
def create_pool(loop, **kw):
	logging.info('create MySQL connection pool...')
	global __pool
	try:
		__pool = yield from aiomysql.create_pool(
			host = kw.get('host', 'localhost'),
			port = kw.get('port', 3306),
			user = kw['user'],
			password = kw['password'],
			db = kw['db'],
			autocommit = kw.get('autocommit', True),
			maxsize = kw.get('maxsize', 100),
			minsize = kw.get('minsize', 10),
			loop = loop
		)
	except Exception as e:
		logging.info('create MySQL connection pool error: %s' % str(e))
		sys.exit()

@asyncio.coroutine
def select(sql, args, size = None):
	logging.info('SQL: %s' % sql)
	global __pool
	with (yield from __pool) as conn:
		cur = yield from conn.cursor(aiomysql.DictCursor)
		
		yield from cur.execute(sql.replace('?', '%s'), args or ())
		if size:
			rs = yield from cur.fetchmany(size)
		else:
			rs = yield from cur.fetchall()
		yield from cur.close()
		conn.close()
		logging.info('rows returned: %s' % len(rs))
		return rs

@asyncio.coroutine
def execute(sql, args, autocommit=True):
	logging.info('SQL: %s' % sql)
	args_encoded = []
	for x in args:
		logging.info('ARGS: %s' % x)
		if(isinstance(x,str)):
			args_encoded.append(x.encode('utf-8'))
		else:
			args_encoded.append(x)
	with (yield from __pool) as conn:
		if not autocommit:
			yield from conn.begin()
		try:
			cur = yield from conn.cursor()
			yield from cur.execute(sql.replace('?', '%s'), args_encoded)
			affected = cur.rowcount
			yield from cur.close()
			if not autocommit:
				yield from conn.commit()
		except BaseException as e:
			if not autocommit:
				yield from conn.rollback()
			conn.close()
			raise
		conn.close()
		return affected

def create_args_string(num):
	L = []
	for n in range(num):
		L.append('?')
	return ', '.join(L)

class ModelMetaclass(type):
	def __new__(cls, name, bases, attrs):
		if name == 'Model':
			return type.__new__(cls, name, bases, attrs)

		tableName = attrs.get('__table__', None) or name
		logging.info('found model: %s (table: %s)' % (name, tableName))

		mappings = dict()
		fields = []
		primaryKey = []
		for k, v in attrs.items():
			if isinstance(v, Field):
				logging.info(' found mapping: %s ==> %s' % (k, v))
				mappings[k] = v
				if v.primary_key:
					if k in primaryKey:
						raise RuntimeError('Duplicate primary key for field: %s' % k)
					primaryKey.append(k)
				else:
					fields.append(k)
		if not primaryKey:
			raise RuntimeError('Primary key not found.')
		for k in mappings.keys():
			attrs.pop(k)

		selected_escaped_fields = attrs.get('__selected_escaped_fields__', None)
		if selected_escaped_fields == None:
			selected_escaped_fields = list(map(lambda f: '`%s`' % f, fields))

		escaped_fields = list(map(lambda f: '`%s`' % f, fields))

		attrs['__mappings__'] = mappings
		attrs['__table__'] = tableName
		attrs['__primary_key__'] = primaryKey
		attrs['__fields__'] = fields

		if not selected_escaped_fields:
			attrs['__select__'] = 'select %s from `%s`' % (', '.join(primaryKey), tableName)
		else:
			attrs['__select__'] = 'select %s, %s from `%s`' % (', '.join(primaryKey), ', '.join(selected_escaped_fields), tableName)			
		if not escaped_fields:
			attrs['__insert__'] = 'insert into `%s` (%s) values(%s)' % (tableName, ', '.join(primaryKey), create_args_string(len(escaped_fields) + len(primaryKey)))
		else:
			attrs['__insert__'] = 'insert into `%s` (%s, %s) values(%s)' % (tableName, ', '.join(escaped_fields), ', '.join(primaryKey), create_args_string(len(escaped_fields) + len(primaryKey)))			
		attrs['__update__'] = 'update `%s` set %s where `%s`=?' % (tableName, ', '.join(map(lambda f: '`%s`=?' % (mappings.get(f).name or f), fields)), primaryKey[0])
		attrs['__delete__'] = 'delete from `%s` where `%s`=?' % (tableName, primaryKey[0])
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

	@classmethod
	@asyncio.coroutine
	def findAll(cls, where = None, args = None, **kw):
		sql = [cls.__select__]
		if where:
			if where.startswith('inner join'):
				sql.append(where)
			else:
				sql.append('where')
				sql.append(where)
		if args is None:
			args = []
		orderby = kw.get('orderby', None)
		if orderby:
			sql.append('order by')
			sql.append(orderby)
		limit = kw.get('limit', None)
		if limit is not None:
			sql.append('limit')
			if isinstance(limit, int):
				sql.append('?')
				args.append(limit)
			elif isinstance(limit, tuple) and len(limit) == 2:
				sql.append('?, ?')
				args.extend(limit)
			else:
				raise ValueError('Invalid limit value: %s' % str(limit))
		rs = yield from select(' '.join(sql), args)
		return [cls(**r) for r in rs]

	@classmethod
	@asyncio.coroutine
	def getNumber(cls, where = None, args = None):
		sql = ['select count(*) from `%s`' % (cls.__table__)]
		if where:
			if where.startswith('inner join'):
				sql.append(where)
			else:
				sql.append('where')
				sql.append(where)
		rs = yield from select(' '.join(sql), args)

		return rs[0]['count(*)']

	@classmethod
	@asyncio.coroutine
	def find(cls, pk):
		rs = yield from select('%s where `%s`=?' % (cls.__select__, cls.__primary_key__[0]), [pk], 1)
		if len(rs) == 0:
			return None
		return cls(**rs[0])

	@asyncio.coroutine
	def save(self):
		args = list(map(self.getValueOrDefault, self.__fields__))
		for i in range(len(self.__primary_key__)):
			args.append(self.getValueOrDefault(self.__primary_key__[i]))
		rows = yield from execute(self.__insert__, args)
		if rows != 1:
			logging.warn('failed to insert record: affected rows: %s' % rows)

	@asyncio.coroutine
	def update(self):
		args = list(map(self.getValue, self.__fields__))
		args.append(self.getValue(self.__primary_key__[0]))
		rows = yield from execute(self.__update__, args)
		if rows != 1:
			logging.warn('failed to update by primary key: affected rows: %s' % rows)

	@asyncio.coroutine
	def remove(self):
		args = [self.getValue(self.__primary_key__[0])]
		rows = yield from execute(self.__delete__, args)
		if rows != 1:
			logging.warn('failed to remove by primary key: affected rows: %s' % rows)
