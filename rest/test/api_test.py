#!/usr/bin/env python3
# -*- coding: utf-8 -*-

__author__ = 'CYX'

import aiohttp, asyncio

HOST = 'localhost'

async def url_access(session, url):
    async with session.get(url) as response:
        return await response.read()

async def main(url):
    async with aiohttp.ClientSession() as sess:
        res = await url_access(sess, url)
        print(res)

loop = asyncio.get_event_loop()
tasks = []
for i in range (1, 100):
    tasks.append(main('http://localhost:9090/api/test/a'))
loop.run_until_complete(asyncio.wait(tasks))
