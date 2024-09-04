// Copyright (c) 2020, the YACCLAB contributors, as 
// shown by the AUTHORS file. All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

ll_tree_black_0: if ((c+=2) >= w - 2) { if (c > w - 2) { goto ll_break_black_0_0; } else { goto ll_break_black_1_0; } } 
		if (CONDITION_O) {
			if (CONDITION_J) {
				ACTION_4
				goto ll_tree_black_6;
			}
			else {
				if (CONDITION_P) {
					NODE_BLACK_277:
					if (CONDITION_K) {
						if (CONDITION_I) {
							NODE_BLACK_279:
							if (CONDITION_D) {
								ACTION_5
								goto ll_tree_black_4;
							}
							else {
								ACTION_10
								goto ll_tree_black_4;
							}
						}
						else {
							ACTION_5
							goto ll_tree_black_4;
						}
					}
					else {
						if (CONDITION_I) {
							ACTION_4
							goto ll_tree_black_3;
						}
						else {
							ACTION_2
							goto ll_tree_black_2;
						}
					}
				}
				else {
					if (CONDITION_I) {
						ACTION_4
						goto ll_tree_black_0;
					}
					else {
						ACTION_2
						goto ll_tree_black_0;
					}
				}
			}
		}
		else {
			NODE_BLACK_282:
			if (CONDITION_P) {
				if (CONDITION_J) {
					ACTION_4
					goto ll_tree_black_5;
				}
				else{
					goto NODE_BLACK_277;
				}
			}
			else {
				ACTION_1
				goto ll_tree_black_1;
			}
		}
ll_tree_black_1: if ((c+=2) >= w - 2) { if (c > w - 2) { goto ll_break_black_0_1; } else { goto ll_break_black_1_1; } } 
		if (CONDITION_O) {
			if (CONDITION_J) {
				if (CONDITION_I) {
					ACTION_4
					goto ll_tree_black_6;
				}
				else {
					if (CONDITION_H) {
						NODE_BLACK_287:
						if (CONDITION_C) {
							ACTION_4
							goto ll_tree_black_6;
						}
						else {
							ACTION_7
							goto ll_tree_black_6;
						}
					}
					else {
						ACTION_4
						goto ll_tree_black_6;
					}
				}
			}
			else {
				if (CONDITION_P) {
					if (CONDITION_K) {
						if (CONDITION_I){
							goto NODE_BLACK_279;
						}
						else {
							if (CONDITION_H) {
								NODE_BLACK_292:
								if (CONDITION_D) {
									if (CONDITION_C) {
										ACTION_5
										goto ll_tree_black_4;
									}
									else {
										ACTION_8
										goto ll_tree_black_4;
									}
								}
								else {
									ACTION_8
									goto ll_tree_black_4;
								}
							}
							else {
								ACTION_5
								goto ll_tree_black_4;
							}
						}
					}
					else {
						if (CONDITION_I) {
							ACTION_4
							goto ll_tree_black_3;
						}
						else {
							if (CONDITION_H) {
								ACTION_3
								goto ll_tree_black_2;
							}
							else {
								ACTION_2
								goto ll_tree_black_2;
							}
						}
					}
				}
				else {
					if (CONDITION_I) {
						ACTION_4
						goto ll_tree_black_0;
					}
					else {
						if (CONDITION_H) {
							ACTION_3
							goto ll_tree_black_0;
						}
						else {
							ACTION_2
							goto ll_tree_black_0;
						}
					}
				}
			}
		}
		else{
			goto NODE_BLACK_282;
		}
ll_tree_black_2: if ((c+=2) >= w - 2) { if (c > w - 2) { goto ll_break_black_0_2; } else { goto ll_break_black_1_2; } } 
		if (CONDITION_O) {
			if (CONDITION_J) {
				ACTION_11
				goto ll_tree_black_6;
			}
			else {
				if (CONDITION_P) {
					if (CONDITION_K) {
						ACTION_12
						goto ll_tree_black_4;
					}
					else {
						ACTION_6
						goto ll_tree_black_7;
					}
				}
				else {
					ACTION_6
					goto ll_tree_black_0;
				}
			}
		}
		else {
			NODE_BLACK_301:
			if (CONDITION_P) {
				if (CONDITION_J) {
					ACTION_4
					goto ll_tree_black_5;
				}
				else {
					if (CONDITION_K) {
						ACTION_5
						goto ll_tree_black_4;
					}
					else {
						ACTION_2
						goto ll_tree_black_2;
					}
				}
			}
			else {
				ACTION_1
				goto ll_tree_black_1;
			}
		}
ll_tree_black_3: if ((c+=2) >= w - 2) { if (c > w - 2) { goto ll_break_black_0_2; } else { goto ll_break_black_1_3; } } 
		if (CONDITION_O) {
			if (CONDITION_J) {
				if (CONDITION_C) {
					NODE_BLACK_306:
					if (CONDITION_B) {
						ACTION_4
						goto ll_tree_black_6;
					}
					else {
						ACTION_7
						goto ll_tree_black_6;
					}
				}
				else {
					ACTION_11
					goto ll_tree_black_6;
				}
			}
			else {
				if (CONDITION_P) {
					if (CONDITION_K) {
						if (CONDITION_D) {
							if (CONDITION_C) {
								NODE_BLACK_311:
								if (CONDITION_B) {
									ACTION_5
									goto ll_tree_black_4;
								}
								else {
									ACTION_12
									goto ll_tree_black_4;
								}
							}
							else {
								ACTION_12
								goto ll_tree_black_4;
							}
						}
						else {
							ACTION_12
							goto ll_tree_black_4;
						}
					}
					else {
						ACTION_6
						goto ll_tree_black_7;
					}
				}
				else {
					ACTION_6
					goto ll_tree_black_0;
				}
			}
		}
		else{
			goto NODE_BLACK_301;
		}
ll_tree_black_4: if ((c+=2) >= w - 2) { if (c > w - 2) { goto ll_break_black_0_2; } else { goto ll_break_black_1_4; } } 
		if (CONDITION_O) {
			if (CONDITION_J) {
				ACTION_4
				goto ll_tree_black_6;
			}
			else {
				if (CONDITION_P) {
					if (CONDITION_K) {
						if (CONDITION_D) {
							ACTION_5
							goto ll_tree_black_4;
						}
						else {
							ACTION_12
							goto ll_tree_black_4;
						}
					}
					else {
						ACTION_6
						goto ll_tree_black_7;
					}
				}
				else {
					ACTION_6
					goto ll_tree_black_0;
				}
			}
		}
		else {
			if (CONDITION_P) {
				if (CONDITION_J) {
					ACTION_4
					goto ll_tree_black_5;
				}
				else {
					if (CONDITION_K){
						goto NODE_BLACK_279;
					}
					else {
						ACTION_4
						goto ll_tree_black_3;
					}
				}
			}
			else {
				ACTION_1
				goto ll_tree_black_1;
			}
		}
ll_tree_black_5: if ((c+=2) >= w - 2) { if (c > w - 2) { goto ll_break_black_0_2; } else { goto ll_break_black_1_5; } } 
		if (CONDITION_O) {
			NODE_BLACK_319:
			if (CONDITION_J) {
				if (CONDITION_I) {
					ACTION_4
					goto ll_tree_black_6;
				}
				else {
					if (CONDITION_C) {
						ACTION_4
						goto ll_tree_black_6;
					}
					else {
						ACTION_11
						goto ll_tree_black_6;
					}
				}
			}
			else {
				if (CONDITION_P) {
					if (CONDITION_K) {
						if (CONDITION_D) {
							if (CONDITION_I) {
								ACTION_5
								goto ll_tree_black_4;
							}
							else {
								if (CONDITION_C) {
									ACTION_5
									goto ll_tree_black_4;
								}
								else {
									ACTION_12
									goto ll_tree_black_4;
								}
							}
						}
						else {
							ACTION_12
							goto ll_tree_black_4;
						}
					}
					else {
						ACTION_6
						goto ll_tree_black_7;
					}
				}
				else {
					ACTION_6
					goto ll_tree_black_0;
				}
			}
		}
		else{
			goto NODE_BLACK_282;
		}
ll_tree_black_6: if ((c+=2) >= w - 2) { if (c > w - 2) { goto ll_break_black_0_3; } else { goto ll_break_black_1_6; } } 
		if (CONDITION_O) {
			if (CONDITION_N){
				goto NODE_BLACK_319;
			}
			else {
				if (CONDITION_J) {
					if (CONDITION_I) {
						ACTION_4
						goto ll_tree_black_6;
					}
					else{
						goto NODE_BLACK_287;
					}
				}
				else {
					if (CONDITION_P) {
						if (CONDITION_K) {
							if (CONDITION_I){
								goto NODE_BLACK_279;
							}
							else{
								goto NODE_BLACK_292;
							}
						}
						else {
							if (CONDITION_I) {
								ACTION_4
								goto ll_tree_black_3;
							}
							else {
								ACTION_3
								goto ll_tree_black_2;
							}
						}
					}
					else {
						if (CONDITION_I) {
							ACTION_4
							goto ll_tree_black_0;
						}
						else {
							ACTION_3
							goto ll_tree_black_0;
						}
					}
				}
			}
		}
		else{
			goto NODE_BLACK_282;
		}
ll_tree_black_7: if ((c+=2) >= w - 2) { if (c > w - 2) { goto ll_break_black_0_2; } else { goto ll_break_black_1_7; } } 
		if (CONDITION_O) {
			if (CONDITION_J) {
				if (CONDITION_C) {
					if (CONDITION_G){
						goto NODE_BLACK_306;
					}
					else {
						ACTION_11
						goto ll_tree_black_6;
					}
				}
				else {
					ACTION_11
					goto ll_tree_black_6;
				}
			}
			else {
				if (CONDITION_P) {
					if (CONDITION_K) {
						if (CONDITION_D) {
							if (CONDITION_C) {
								if (CONDITION_G){
									goto NODE_BLACK_311;
								}
								else {
									ACTION_12
									goto ll_tree_black_4;
								}
							}
							else {
								ACTION_12
								goto ll_tree_black_4;
							}
						}
						else {
							ACTION_12
							goto ll_tree_black_4;
						}
					}
					else {
						ACTION_6
						goto ll_tree_black_7;
					}
				}
				else {
					ACTION_6
					goto ll_tree_black_0;
				}
			}
		}
		else{
			goto NODE_BLACK_301;
		}
ll_break_black_0_0:
		if (CONDITION_O) {
			NODE_BLACK_343:
			if (CONDITION_I) {
				ACTION_4
			}
			else {
				ACTION_2
			}
		}
		else {
			ACTION_1
		}
	goto ll_black_end;
ll_break_black_0_1:
		if (CONDITION_O) {
			NODE_BLACK_344:
			if (CONDITION_I) {
				ACTION_4
			}
			else {
				if (CONDITION_H) {
					ACTION_3
				}
				else {
					ACTION_2
				}
			}
		}
		else {
			ACTION_1
		}
	goto ll_black_end;
ll_break_black_0_2:
		if (CONDITION_O) {
			ACTION_6
		}
		else {
			ACTION_1
		}
	goto ll_black_end;
ll_break_black_0_3:
		if (CONDITION_O) {
			if (CONDITION_N) {
				ACTION_6
			}
			else {
				NODE_BLACK_347:
				if (CONDITION_I) {
					ACTION_4
				}
				else {
					ACTION_3
				}
			}
		}
		else {
			ACTION_1
		}
	goto ll_black_end;
ll_break_black_1_0:
		if (CONDITION_O) {
			NODE_BLACK_348:
			if (CONDITION_J) {
				ACTION_4
			}
			else{
				goto NODE_BLACK_343;
			}
		}
		else {
			NODE_BLACK_349:
			if (CONDITION_P){
				goto NODE_BLACK_348;
			}
			else {
				ACTION_1
			}
		}
	goto ll_black_end;
ll_break_black_1_1:
		if (CONDITION_O) {
			if (CONDITION_J) {
				if (CONDITION_I) {
					ACTION_4
				}
				else {
					if (CONDITION_H) {
						NODE_BLACK_353:
						if (CONDITION_C) {
							ACTION_4
						}
						else {
							ACTION_7
						}
					}
					else {
						ACTION_4
					}
				}
			}
			else{
				goto NODE_BLACK_344;
			}
		}
		else{
			goto NODE_BLACK_349;
		}
	goto ll_black_end;
ll_break_black_1_2:
		if (CONDITION_O) {
			if (CONDITION_J) {
				ACTION_11
			}
			else {
				ACTION_6
			}
		}
		else {
			NODE_BLACK_355:
			if (CONDITION_P) {
				if (CONDITION_J) {
					ACTION_4
				}
				else {
					ACTION_2
				}
			}
			else {
				ACTION_1
			}
		}
	goto ll_black_end;
ll_break_black_1_3:
		if (CONDITION_O) {
			if (CONDITION_J) {
				if (CONDITION_C) {
					NODE_BLACK_359:
					if (CONDITION_B) {
						ACTION_4
					}
					else {
						ACTION_7
					}
				}
				else {
					ACTION_11
				}
			}
			else {
				ACTION_6
			}
		}
		else{
			goto NODE_BLACK_355;
		}
	goto ll_black_end;
ll_break_black_1_4:
		if (CONDITION_O) {
			if (CONDITION_J) {
				ACTION_4
			}
			else {
				ACTION_6
			}
		}
		else {
			if (CONDITION_P) {
				ACTION_4
			}
			else {
				ACTION_1
			}
		}
	goto ll_black_end;
ll_break_black_1_5:
		if (CONDITION_O) {
			NODE_BLACK_362:
			if (CONDITION_J) {
				if (CONDITION_I) {
					ACTION_4
				}
				else {
					if (CONDITION_C) {
						ACTION_4
					}
					else {
						ACTION_11
					}
				}
			}
			else {
				ACTION_6
			}
		}
		else{
			goto NODE_BLACK_349;
		}
	goto ll_black_end;
ll_break_black_1_6:
		if (CONDITION_O) {
			if (CONDITION_N){
				goto NODE_BLACK_362;
			}
			else {
				if (CONDITION_J) {
					if (CONDITION_I) {
						ACTION_4
					}
					else{
						goto NODE_BLACK_353;
					}
				}
				else{
					goto NODE_BLACK_347;
				}
			}
		}
		else{
			goto NODE_BLACK_349;
		}
	goto ll_black_end;
ll_break_black_1_7:
		if (CONDITION_O) {
			if (CONDITION_J) {
				if (CONDITION_C) {
					if (CONDITION_G){
						goto NODE_BLACK_359;
					}
					else {
						ACTION_11
					}
				}
				else {
					ACTION_11
				}
			}
			else {
				ACTION_6
			}
		}
		else{
			goto NODE_BLACK_355;
		}
	goto ll_black_end;
ll_black_end:;