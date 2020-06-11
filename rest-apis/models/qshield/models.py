#!/usr/bin/env python3
# -*- coding: utf-8 -*-

__author__ = 'CYX'

from orm import Model
from field import StringField, IntegerField

class Rankings(Model):
    __table__ = 'RANKINGS'
    __path__ = 'outsourced/RANKINGS'

    pageURL = StringField()
    pageRank = IntegerField()
    avgDuration = IntegerField()
