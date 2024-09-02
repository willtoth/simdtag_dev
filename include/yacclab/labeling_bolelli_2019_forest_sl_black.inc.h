// Copyright (c) 2020, the YACCLAB contributors, as 
// shown by the AUTHORS file. All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

sl_tree_black_0: if ((c+=2) >= w - 2) { if (c > w - 2) { goto sl_break_black_0_0; } else { goto sl_break_black_1_0; } } 
		if (CONDITION_O) {
			if (CONDITION_P) {
				ACTION_2
				goto sl_tree_black_1;
			}
			else {
				ACTION_2
				goto sl_tree_black_0;
			}
		}
		else {
			NODE_BLACK_372:
			if (CONDITION_P) {
				ACTION_2
				goto sl_tree_black_1;
			}
			else {
				ACTION_1
				goto sl_tree_black_0;
			}
		}
sl_tree_black_1: if ((c+=2) >= w - 2) { if (c > w - 2) { goto sl_break_black_0_1; } else { goto sl_break_black_1_1; } } 
		if (CONDITION_O) {
			if (CONDITION_P) {
				ACTION_6
				goto sl_tree_black_1;
			}
			else {
				ACTION_6
				goto sl_tree_black_0;
			}
		}
		else{
			goto NODE_BLACK_372;
		}
sl_break_black_0_0:
		if (CONDITION_O) {
			ACTION_2
		}
		else {
			ACTION_1
		}
	goto end_black_sl;
sl_break_black_0_1:
		if (CONDITION_O) {
			ACTION_6
		}
		else {
			ACTION_1
		}
	goto end_black_sl;
sl_break_black_1_0:
		if (CONDITION_O) {
			if (CONDITION_P) {
				ACTION_2
			}
			else {
				ACTION_2
			}
		}
		else {
			NODE_BLACK_375:
			if (CONDITION_P) {
				ACTION_2
			}
			else {
				ACTION_1
			}
		}
	goto end_black_sl;
sl_break_black_1_1:
		if (CONDITION_O) {
			if (CONDITION_P) {
				ACTION_6
			}
			else {
				ACTION_6
			}
		}
		else{
			goto NODE_BLACK_375;
		}
	goto end_black_sl;
end_black_sl:;
