#!/usr/bin/env python3
# -*- coding: utf-8 -*-

__author__ = 'CYX'

from orm import Model
from field import MediumBlobField

class Test(Model):
    __table__ = 'test'

    word = MediumBlobField()
    count = MediumBlobField()
