#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import logging; logging.basicConfig(level = logging.INFO)
import asyncio, os, json, time
from datetime import datetime
from aiohttp import web
from jinja2 import Environment, FileSystemLoader

from config import configs
HOST = configs.server.host
PORT = configs.server.port
from coroweb import add_routes, add_static
from tools import MyJsonEncoder

from models.qshield import orm

def init_jinja2(app, **kw):
	logging.info('init jinja2...')
	options = dict(
		autoescape = kw.get('autoescape', True),
		block_start_string = kw.get('block_start_string', '{%'),
		block_end_string = kw.get('block_end_string', '%}'),
		variable_start_string = kw.get('variable_start_string', '{{'),
		variable_end_string = kw.get('variable_end_string', '}}'),
		auto_reload = kw.get('auto_reload', True)
	)
	path = kw.get('path', None)
	if path is None:
		path = os.path.join(os.path.dirname(os.path.abspath(__file__)), 'templates')
	logging.info('set jinja2 template path: %s' % path)
	env = Environment(loader = FileSystemLoader(path), **options)
	filters = kw.get('filters', None)
	if filters is not None:
		for name, f in filters.items():
			env.filters[name] = f
	app['__templating__'] = env

@asyncio.coroutine
def logger_factory(app, handler):
	@asyncio.coroutine
	def logger(request):
		logging.info('Request: %s %s' % (request.method, request.path))
		return (yield from handler(request))
	return logger

@asyncio.coroutine
def response_factory(app, handler):
	@asyncio.coroutine
	def response(request):
		logging.info('Response handler...')
		r = yield from handler(request)
		if isinstance(r, web.StreamResponse):
			return r
		if isinstance(r, bytes):
			resp = web.Response(body = r)
			resp.content_type = 'application/octet-stream'
			return resp
		if isinstance(r, str):
			if r.startswith('redirect:'):
				return web.HTTPFound(r[9:])
			resp = web.Response(body = r.encode('utf-8'))
			resp.content_type = 'text/html;charset = utf-8'
			return resp
		if isinstance(r, dict):
			template = r.get('__template__')
			if template is None:
				resp = web.Response(body = json.dumps(r, ensure_ascii = False, cls = MyJsonEncoder, sort_keys = True).encode('utf-8'))
				return resp
			else:
				resp = web.Response(body = app['__templating__'].get_template(template).render(**r).encode('utf-8'))
				resp.content_type = 'text/html;charset = utf-8'
				return resp

		resp = web.Response(body = str(r).encode('utf-8'))
		resp.content_type = 'text/plain;charset = utf-8'
		return resp
	return response

@asyncio.coroutine
def init(loop):
	orm.init_sql_ra_context(**configs.qshield)
	app = web.Application(loop = loop, middlewares = [logger_factory, response_factory])
	init_jinja2(app)
	#app.router.add_route('GET', '/', index)
	#add_route(app, index)
	#add_static(app)
	add_routes(app, 'handlers')
	srv = yield from loop.create_server(app.make_handler(), HOST, PORT)
	logging.info('server started at http://%s:%s' % (HOST, PORT))
	return srv

loop = asyncio.get_event_loop()
loop.run_until_complete(init(loop))
loop.run_forever()
