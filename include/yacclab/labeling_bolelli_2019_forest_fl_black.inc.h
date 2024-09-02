// Copyright (c) 2020, the YACCLAB contributors, as 
// shown by the AUTHORS file. All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

fl_tree_black_0: if ((c+=2) >= w - 2) { if (c > w - 2) { goto fl_break_black_0_0; } else { goto fl_break_black_1_0; } } 
		if (CONDITION_O) {
			NODE_BLACK_253:
			if (CONDITION_P) {
				ACTION_2
				goto fl_tree_black_1;
			}
			else {
				ACTION_2
				goto fl_tree_black_2;
			}
		}
		else {
			if (CONDITION_S){
				goto NODE_BLACK_253;
			}
			else {
				NODE_BLACK_255:
				if (CONDITION_P) {
					ACTION_2
					goto fl_tree_black_1;
				}
				else {
					if (CONDITION_T) {
						ACTION_2
						goto fl_tree_black_1;
					}
					else {
						ACTION_1
						goto fl_tree_black_0;
					}
				}
			}
		}
fl_tree_black_1: if ((c+=2) >= w - 2) { if (c > w - 2) { goto fl_break_black_0_1; } else { goto fl_break_black_1_1; } } 
		if (CONDITION_O) {
			NODE_BLACK_257:
			if (CONDITION_P) {
				ACTION_6
				goto fl_tree_black_1;
			}
			else {
				ACTION_6
				goto fl_tree_black_2;
			}
		}
		else {
			if (CONDITION_S){
				goto NODE_BLACK_257;
			}
			else{
				goto NODE_BLACK_255;
			}
		}
fl_tree_black_2: if ((c+=2) >= w - 2) { if (c > w - 2) { goto fl_break_black_0_2; } else { goto fl_break_black_1_2; } } 
		if (CONDITION_O) {
			if (CONDITION_R){
				goto NODE_BLACK_257;
			}
			else{
				goto NODE_BLACK_253;
			}
		}
		else {
			if (CONDITION_S) {
				if (CONDITION_P) {
					if (CONDITION_R) {
						ACTION_6
						goto fl_tree_black_1;
					}
					else {
						ACTION_2
						goto fl_tree_black_1;
					}
				}
				else {
					if (CONDITION_R) {
						ACTION_6
						goto fl_tree_black_2;
					}
					else {
						ACTION_2
						goto fl_tree_black_2;
					}
				}
			}
			else{
				goto NODE_BLACK_255;
			}
		}
fl_break_black_0_0:
		if (CONDITION_O) {
			ACTION_2
		}
		else {
			if (CONDITION_S) {
				ACTION_2
			}
			else {
				ACTION_1
			}
		}
	goto end_black_fl;
fl_break_black_0_1:
		if (CONDITION_O) {
			ACTION_6
		}
		else {
			if (CONDITION_S) {
				ACTION_6
			}
			else {
				ACTION_1
			}
		}
	goto end_black_fl;
fl_break_black_0_2:
		if (CONDITION_O) {
			NODE_BLACK_266:
			if (CONDITION_R) {
				ACTION_6
			}
			else {
				ACTION_2
			}
		}
		else {
			if (CONDITION_S){
				goto NODE_BLACK_266;
			}
			else {
				ACTION_1
			}
		}
	goto end_black_fl;
fl_break_black_1_0:
		if (CONDITION_O) {
			NODE_BLACK_268:
			if (CONDITION_P) {
				ACTION_2
			}
			else {
				ACTION_2
			}
		}
		else {
			if (CONDITION_S){
				goto NODE_BLACK_268;
			}
			else {
				NODE_BLACK_270:
				if (CONDITION_P) {
					ACTION_2
				}
				else {
					if (CONDITION_T) {
						ACTION_2
					}
					else {
						ACTION_1
					}
				}
			}
		}
	goto end_black_fl;
fl_break_black_1_1:
		if (CONDITION_O) {
			NODE_BLACK_272:
			if (CONDITION_P) {
				ACTION_6
			}
			else {
				ACTION_6
			}
		}
		else {
			if (CONDITION_S){
				goto NODE_BLACK_272;
			}
			else{
				goto NODE_BLACK_270;
			}
		}
	goto end_black_fl;
fl_break_black_1_2:
		if (CONDITION_O) {
			if (CONDITION_R){
				goto NODE_BLACK_272;
			}
			else{
				goto NODE_BLACK_268;
			}
		}
		else {
			if (CONDITION_S) {
				if (CONDITION_P){
					goto NODE_BLACK_266;
				}
				else{
					goto NODE_BLACK_266;
				}
			}
			else{
				goto NODE_BLACK_270;
			}
		}
	goto end_black_fl;
end_black_fl:;