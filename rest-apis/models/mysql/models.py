#!/usr/bin/env python3
# -*- coding: utf-8 -*-

__author__ = 'CYX'

from field import StringField, BooleanField, IntegerField, FloatField, TextField, DateTimeField, MediumBlobField

from tools import next_id, cur_timestamp
from config import configs
from .orm import Model

class SR_News_Result(Model):
	__table__ = 'sr_news_result'

	id = StringField(primary_key = True, default = next_id, column_type = 'varchar(255)')
	title = StringField(column_type = 'varchar(255)')
	site_name = StringField(column_type = 'varchar(255)')
	url = StringField(column_type = 'varchar(255)')
	post_time = StringField(column_type = 'varchar(64)')
	related_topic = StringField(column_type = 'varchar(255)')
