// Define Conditions and Actions

#define CONDITION_B img_row_prev_prev[c-1]==WHITE_COLOR_VALUE
#define CONDITION_C img_row_prev_prev[c]==WHITE_COLOR_VALUE
#define CONDITION_D img_row_prev_prev[c+1]==WHITE_COLOR_VALUE
#define CONDITION_E img_row_prev_prev[c+2]==WHITE_COLOR_VALUE

#define CONDITION_G img_row_prev[c-2]==WHITE_COLOR_VALUE
#define CONDITION_H img_row_prev[c-1]==WHITE_COLOR_VALUE
#define CONDITION_I img_row_prev[c]==WHITE_COLOR_VALUE
#define CONDITION_J img_row_prev[c+1]==WHITE_COLOR_VALUE
#define CONDITION_K img_row_prev[c+2]==WHITE_COLOR_VALUE

#define CONDITION_M img_row[c-2]==WHITE_COLOR_VALUE
#define CONDITION_N img_row[c-1]==WHITE_COLOR_VALUE
#define CONDITION_O img_row[c]==WHITE_COLOR_VALUE
#define CONDITION_P img_row[c+1]==WHITE_COLOR_VALUE

#define CONDITION_R img_row_fol[c-1]==WHITE_COLOR_VALUE
#define CONDITION_S img_row_fol[c]==WHITE_COLOR_VALUE
#define CONDITION_T img_row_fol[c+1]==WHITE_COLOR_VALUE

			// Action 1: No action
#define ACTION_1 img_labels_row[c] = 0; 
							   // Action 2: New label (the block has foreground pixels and is not connected to anything else)
#define ACTION_2 img_labels_row[c] = LabelsSolver::NewLabel(); 
							   //Action 3: Assign label of block P
#define ACTION_3 img_labels_row[c] = img_labels_row_prev_prev[c - 2];
							   // Action 4: Assign label of block Q 
#define ACTION_4 img_labels_row[c] = img_labels_row_prev_prev[c];
							   // Action 5: Assign label of block R
#define ACTION_5 img_labels_row[c] = img_labels_row_prev_prev[c + 2];
							   // Action 6: Assign label of block S
#define ACTION_6 img_labels_row[c] = img_labels_row[c - 2]; 
							   // Action 7: Merge labels of block P and Q
#define ACTION_7 img_labels_row[c] = LabelsSolver::Merge(img_labels_row_prev_prev[c - 2], img_labels_row_prev_prev[c]);
							   //Action 8: Merge labels of block P and R
#define ACTION_8 img_labels_row[c] = LabelsSolver::Merge(img_labels_row_prev_prev[c - 2], img_labels_row_prev_prev[c + 2]);
							   // Action 9 Merge labels of block P and S
#define ACTION_9 img_labels_row[c] = LabelsSolver::Merge(img_labels_row_prev_prev[c - 2], img_labels_row[c - 2]);
							   // Action 10 Merge labels of block Q and R
#define ACTION_10 img_labels_row[c] = LabelsSolver::Merge(img_labels_row_prev_prev[c], img_labels_row_prev_prev[c + 2]);
							   // Action 11: Merge labels of block Q and S
#define ACTION_11 img_labels_row[c] = LabelsSolver::Merge(img_labels_row_prev_prev[c], img_labels_row[c - 2]);
							   // Action 12: Merge labels of block R and S
#define ACTION_12 img_labels_row[c] = LabelsSolver::Merge(img_labels_row_prev_prev[c + 2], img_labels_row[c - 2]);
							   // Action 13: Merge labels of block P, Q and R
#define ACTION_13 img_labels_row[c] = LabelsSolver::Merge(LabelsSolver::Merge(img_labels_row_prev_prev[c - 2], img_labels_row_prev_prev[c]), img_labels_row_prev_prev[c + 2]);
							   // Action 14: Merge labels of block P, Q and S
#define ACTION_14 img_labels_row[c] = LabelsSolver::Merge(LabelsSolver::Merge(img_labels_row_prev_prev[c - 2], img_labels_row_prev_prev[c]), img_labels_row[c - 2]);
							   //Action 15: Merge labels of block P, R and S
#define ACTION_15 img_labels_row[c] = LabelsSolver::Merge(LabelsSolver::Merge(img_labels_row_prev_prev[c - 2], img_labels_row_prev_prev[c + 2]), img_labels_row[c - 2]);
							   //Action 16: labels of block Q, R and S
#define ACTION_16 img_labels_row[c] = LabelsSolver::Merge(LabelsSolver::Merge(img_labels_row_prev_prev[c], img_labels_row_prev_prev[c + 2]), img_labels_row[c - 2]);
