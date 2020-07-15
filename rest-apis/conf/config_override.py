#!/usr/bin/env python3
# -*- coding: utf-8 -*-

__author__ = 'CYX'

configs = {
	'server': {
		'host': '0.0.0.0',
		'port': 9090
	},
	'qshield': {
		'jars': '/home/hadoop/qshield/opaque-ext/target/scala-2.11/opaque-ext_2.11-0.1.jar,/home/hadoop/qshield/data-owner/target/scala-2.11/data-owner_2.11-0.1.jar',
		'master': 'spark://SGX:7077'
	}
}
