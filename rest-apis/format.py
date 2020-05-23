#!/usr/bin/env python3
# -*- coding: utf-8 -*-

__author__ = 'CYX'

############################################################################################
#							format for data results
#
#	code:	200(ok);	204(not found);	401(unauthorized); 	500(error);	
#	ok:		true(has results);		false(no results);
#	msg:	information about the status
#	needLogin:	true(need authorization);		false(not need authorization);
#	p:	current page
#	pcount:	data count per page
#	data:	data entity
############################################################################################

def data_res_format(*, code = 200, ok = True, msg = '', need_login = False, p = 0, pcount = 0, data = None):
	return {
		'code': code,
		'ok': ok,
		'msg': msg,
		'needLogin': need_login,
		'p': p,
		'pcount': pcount,
		'data': data
	}

############################################################################################
#							format for data items number
#
#	code:	200(ok);	204(not found);	401(unauthorized); 	500(error);	
#	ok:		true(has results);		false(no results);
#	msg:	information about the status
#	needLogin:	true(need authorization);		false(not need authorization);
#	totalDataCount:	the total data count of the results
############################################################################################

def items_num_format(*, code = 200, ok = True, msg = '', need_login = False, total_data_count = 0):
	return {
		'code': code,
		'ok': ok,
		'msg': msg,
		'needLogin': need_login,
		'totalDataCount': total_data_count
	}

############################################################################################
#							repo format for data items number
#
#	code:	200(ok);	204(not found);	401(unauthorized); 	500(error);	
#	ok:		true(has results);		false(no results);
#	msg:	information about the status
#	needLogin:	true(need authorization);		false(not need authorization);
#	totalDataCount:	the total data count of the results
############################################################################################

def repo_items_num_format(*, code = 200, ok = True, msg = '', need_login = False, total_data_count = {}):
	return {
		'code': code,
		'ok': ok,
		'msg': msg,
		'needLogin': need_login,
		'totalDataCount': total_data_count
	}

############################################################################################
#						repo format for data results
#
#	code:	200(ok);	204(not found);	401(unauthorized); 	500(error);	
#	ok:		true(has results);		false(no results);
#	msg:	information about the status
#	needLogin:	true(need authorization);		false(not need authorization);
#	p:	current page
#	pcount:	data count per page
#	data:	data entity
############################################################################################

def repo_data_res_format(*, code = 200, ok = True, msg = '', need_login = False, p = 0, pcount = 0, data = None):
	return {
		'code': code,
		'ok': ok,
		'msg': msg,
		'needLogin': need_login,
		'p': p,
		'pcount': pcount,
		'data': data
	}
