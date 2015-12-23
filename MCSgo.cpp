/*
 * This code is provied as a sample code of Hw 2 of "Theory of Computer Game".
 * The "genmove" function will randomly output one of the legal move.
 * This code can only be used within the class.
 *
 * 2015 Nov. Hung-Jui Chang
 * */
#include <iostream>
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <ctime>
#include <cmath>
#include <vector>

#include <stdio.h>

#define BOARDSIZE        9
#define BOUNDARYSIZE    11
#define COMMANDLENGTH 1000
#define DEFAULTTIME      2
#define SIMULATETIME     1
#define ONCESIMULATE   100
#define DEFAULTKOMI      7

#define SAFEMOVE        20
#define UCBCONSTANT      2

#define MAXGAMELENGTH 1000
#define MAXSTRING       50
#define MAXDIRECTION     4

#define NUMINTERSECTION 81
#define HISTORYLENGTH   200

#define EMPTY            0
#define BLACK            1
#define WHITE            2
#define BOUNDARY         3

#define SELF             1
#define OPPONENT         2

#define NUMGTPCOMMANDS      15

#define LOCALVERSION      1
#define GTPVERSION        2
 
using namespace std;
int _board_size = BOARDSIZE;
int _board_boundary = BOUNDARYSIZE;
double _komi =  DEFAULTKOMI;
const int DirectionX[MAXDIRECTION] = {-1, 0, 1, 0};
const int DirectionY[MAXDIRECTION] = { 0, 1, 0,-1};
const char LabelX[]="0ABCDEFGHJ";

typedef struct MCS_node{
    struct MCS_node* parent;
    double win;
    int visit;
	int move;
    int turn;
    int (*Board)[BOUNDARYSIZE];
    vector <struct MCS_node*> child;
}MCSNODE;

void reset(int Board[BOUNDARYSIZE][BOUNDARYSIZE]);
int find_liberty(int X, int Y, int label, int Board[BOUNDARYSIZE][BOUNDARYSIZE], int ConnectBoard[BOUNDARYSIZE][BOUNDARYSIZE]);
void count_liberty(int X, int Y, int Board[BOUNDARYSIZE][BOUNDARYSIZE], int Liberties[MAXDIRECTION]);
void count_neighboorhood_state(int Board[BOUNDARYSIZE][BOUNDARYSIZE], int X, int Y, int turn, int* empt, int* self, int* oppo ,int* boun, int NeighboorhoodState[MAXDIRECTION]);
int remove_piece(int Board[BOUNDARYSIZE][BOUNDARYSIZE], int X, int Y, int turn);
void update_board(int Board[BOUNDARYSIZE][BOUNDARYSIZE], int X, int Y, int turn);
int update_board_check(int Board[BOUNDARYSIZE][BOUNDARYSIZE], int X, int Y, int turn);
int gen_legal_move(int Board[BOUNDARYSIZE][BOUNDARYSIZE], int turn, int game_length, int GameRecord[MAXGAMELENGTH][BOUNDARYSIZE][BOUNDARYSIZE], int MoveList[HISTORYLENGTH]);
int rand_pick_move(int num_legal_moves, int MoveList[HISTORYLENGTH]);
int rand_simulate(int Board[BOUNDARYSIZE][BOUNDARYSIZE], int turn, int game_length, int GameRecord[MAXGAMELENGTH][BOUNDARYSIZE][BOUNDARYSIZE]);
int MCS_pure_move(int Board[BOUNDARYSIZE][BOUNDARYSIZE], int num_legal_moves, int MoveList[HISTORYLENGTH], clock_t end_t, int turn, int game_length, int GameRecord[MAXGAMELENGTH][BOUNDARYSIZE][BOUNDARYSIZE]);
void do_move(int Board[BOUNDARYSIZE][BOUNDARYSIZE], int turn, int move);
void record(int Board[BOUNDARYSIZE][BOUNDARYSIZE], int GameRecord[MAXGAMELENGTH][BOUNDARYSIZE][BOUNDARYSIZE], int game_length);
double final_score(int Board[BOUNDARYSIZE][BOUNDARYSIZE]);
void gtp_showboard(int Board[BOUNDARYSIZE][BOUNDARYSIZE]);
/*
 * This function reset the board, the board intersections are labeled with 0,
 * the boundary intersections are labeled with 3.
 * */
void reset(int Board[BOUNDARYSIZE][BOUNDARYSIZE]) {
    for (int i = 1 ; i <= BOARDSIZE; ++i) {
    for (int j = 1 ; j <= BOARDSIZE; ++j) {
        Board[i][j] = EMPTY;
    }
    }
    for (int i = 0 ; i < BOUNDARYSIZE; ++i) {
    Board[0][i] = Board[BOUNDARYSIZE-1][i] = Board[i][0] = Board[i][BOUNDARYSIZE-1] = BOUNDARY;
    }
}

/*
 * This function return the total number of liberity of the string of (X, Y) and
 * the string will be label with 'label'.
 * */
int find_liberty(int X, int Y, int label, int Board[BOUNDARYSIZE][BOUNDARYSIZE], int ConnectBoard[BOUNDARYSIZE][BOUNDARYSIZE]) {
    // Label the current intersection
    ConnectBoard[X][Y] |= label;
    int total_liberty = 0;
    for (int d = 0 ; d < MAXDIRECTION; ++d) {
        // Check this intersection has been visited or not
        if ((ConnectBoard[X+DirectionX[d]][Y+DirectionY[d]] & (1<<label) )!= 0) continue;

        // Check this intersection is not visited yet
        ConnectBoard[X+DirectionX[d]][Y+DirectionY[d]] |=(1<<label);
        // This neighboorhood is empty
        if (Board[X+DirectionX[d]][Y+DirectionY[d]] == EMPTY)
            total_liberty++;
        // This neighboorhood is self stone
        else if (Board[X+DirectionX[d]][Y+DirectionY[d]] == Board[X][Y])
            total_liberty += find_liberty(X+DirectionX[d], Y+DirectionY[d], label, Board, ConnectBoard);
    }
    return total_liberty;
}

/*
 * This function count the liberties of the given intersection's neighboorhod
 * */
void count_liberty(int X, int Y, int Board[BOUNDARYSIZE][BOUNDARYSIZE], int Liberties[MAXDIRECTION]) {
    int ConnectBoard[BOUNDARYSIZE][BOUNDARYSIZE];
    // Initial the ConnectBoard
    for (int i = 0 ; i < BOUNDARYSIZE; ++i) {for (int j = 0 ; j < BOUNDARYSIZE; ++j) {ConnectBoard[i][j] = 0;}}
    // Find the same connect component and its liberity
    for (int d = 0 ; d < MAXDIRECTION; ++d) {
        Liberties[d] = 0;
        if (Board[X+DirectionX[d]][Y+DirectionY[d]] == BLACK || Board[X+DirectionX[d]][Y+DirectionY[d]] == WHITE ) {
            Liberties[d] = find_liberty(X+DirectionX[d], Y+DirectionY[d], d, Board, ConnectBoard);
        }
    }
}

/*
 * This function count the number of empty, self, opponent, and boundary intersections of the neighboorhod
 * and saves the type in NeighboorhoodState.
 * */
void count_neighboorhood_state(int Board[BOUNDARYSIZE][BOUNDARYSIZE], int X, int Y, int turn, int* empt, int* self, int* oppo ,int* boun, int NeighboorhoodState[MAXDIRECTION]) {
    for (int d = 0 ; d < MAXDIRECTION; ++d) {
    // check the number of nonempty neighbor
    switch(Board[X+DirectionX[d]][Y+DirectionY[d]]) {
        case EMPTY:    (*empt)++; 
               NeighboorhoodState[d] = EMPTY;
               break;
        case BLACK:    if (turn == BLACK) {
                   (*self)++;
                   NeighboorhoodState[d] = SELF;
               }
               else {
                   (*oppo)++;
                   NeighboorhoodState[d] = OPPONENT;
               }
               break;
        case WHITE:    if (turn == WHITE) {
                   (*self)++;
                   NeighboorhoodState[d] = SELF;
               }
               else {
                   (*oppo)++;
                   NeighboorhoodState[d] = OPPONENT;
               }
               break;
        case BOUNDARY: (*boun)++;
               NeighboorhoodState[d] = BOUNDARY;
               break;
    }
    }
}

/*
 * This function remove the connect component contains (X, Y) with color "turn" 
 * And return the number of remove stones.
 * */
int remove_piece(int Board[BOUNDARYSIZE][BOUNDARYSIZE], int X, int Y, int turn) {
    int remove_stones = (Board[X][Y]==EMPTY)?0:1;
    Board[X][Y] = EMPTY;
    for (int d = 0; d < MAXDIRECTION; ++d) {
    if (Board[X+DirectionX[d]][Y+DirectionY[d]] == turn) {
        remove_stones += remove_piece(Board, X+DirectionX[d], Y+DirectionY[d], turn);
    }
    }
    return remove_stones;
}
/*
 * This function update Board with place turn's piece at (X,Y).
 * Note that this function will not check if (X, Y) is a legal move or not.
 * */
void update_board(int Board[BOUNDARYSIZE][BOUNDARYSIZE], int X, int Y, int turn) {
    int num_neighborhood_self = 0;
    int num_neighborhood_oppo = 0;
    int num_neighborhood_empt = 0;
    int num_neighborhood_boun = 0;
    int Liberties[4];
    int NeighboorhoodState[4];
    count_neighboorhood_state(Board, X, Y, turn,
        &num_neighborhood_empt,
        &num_neighborhood_self,
        &num_neighborhood_oppo,
        &num_neighborhood_boun, NeighboorhoodState);
    // check if there is opponent piece in the neighboorhood
    if (num_neighborhood_oppo != 0) {
    count_liberty(X, Y, Board, Liberties);
    for (int d = 0 ; d < MAXDIRECTION; ++d) {
        // check if there is opponent component only one liberty
        if (NeighboorhoodState[d] == OPPONENT && Liberties[d] == 1 && Board[X+DirectionX[d]][Y+DirectionY[d]]!=EMPTY) {
        remove_piece(Board, X+DirectionX[d], Y+DirectionY[d], Board[X+DirectionX[d]][Y+DirectionY[d]]);
        }
    }
    }
    Board[X][Y] = turn;
}
/*
 * This function update Board with place turn's piece at (X,Y).
 * Note that this function will check if (X, Y) is a legal move or not.
 * */
int update_board_check(int Board[BOUNDARYSIZE][BOUNDARYSIZE], int X, int Y, int turn) {
    // Check the given coordination is legal or not
    if ( X < 1 || X > BOARDSIZE || Y < 1 || Y > BOARDSIZE || Board[X][Y]!=EMPTY)
    return 0;
    int num_neighborhood_self = 0;
    int num_neighborhood_oppo = 0;
    int num_neighborhood_empt = 0;
    int num_neighborhood_boun = 0;
    int Liberties[4];
    int NeighboorhoodState[4];
    count_neighboorhood_state(Board, X, Y, turn,
        &num_neighborhood_empt,
        &num_neighborhood_self,
        &num_neighborhood_oppo,
        &num_neighborhood_boun, NeighboorhoodState);
    // Check if the move is a legal move
    // Condition 1: there is a empty intersection in the neighboorhood
    int legal_flag = 0;
    count_liberty(X, Y, Board, Liberties);
    if (num_neighborhood_empt != 0) {
    legal_flag = 1;
    }
    else {
    // Condition 2: there is a self string has more than one liberty
    for (int d = 0; d < MAXDIRECTION; ++d) {
        if (NeighboorhoodState[d] == SELF && Liberties[d] > 1) {
        legal_flag = 1;
        }
    }
    if (legal_flag == 0) {
    // Condition 3: there is a opponent string has exactly one liberty
        for (int d = 0; d < MAXDIRECTION; ++d) {
        if (NeighboorhoodState[d] == OPPONENT && Liberties[d] == 1) {
            legal_flag = 1;
        }
        }
    }
    }

    if (legal_flag == 1) {
        // check if there is opponent piece in the neighboorhood
        if (num_neighborhood_oppo != 0) {
            for (int d = 0 ; d < MAXDIRECTION; ++d) {
                // check if there is opponent component only one liberty
                if (NeighboorhoodState[d] == OPPONENT && Liberties[d] == 1 && Board[X+DirectionX[d]][Y+DirectionY[d]]!=EMPTY)
                    remove_piece(Board, X+DirectionX[d], Y+DirectionY[d], Board[X+DirectionX[d]][Y+DirectionY[d]]);
            }
        }
        Board[X][Y] = turn;
    }

    return (legal_flag==1)?1:0;
}

/*
 * This function return the number of legal moves with clor "turn" and
 * saves all legal moves in MoveList
 * */
int gen_legal_move(int Board[BOUNDARYSIZE][BOUNDARYSIZE], int turn, int game_length, int GameRecord[MAXGAMELENGTH][BOUNDARYSIZE][BOUNDARYSIZE], int MoveList[HISTORYLENGTH]) {
    int NextBoard[BOUNDARYSIZE][BOUNDARYSIZE];
    int num_neighborhood_self = 0;
    int num_neighborhood_oppo = 0;
    int num_neighborhood_empt = 0;
    int num_neighborhood_boun = 0;
    int legal_moves = 0;
    int next_x, next_y;
    int Liberties[4];
    int NeighboorhoodState[4];
    bool eat_move = 0;
    for (int x = 1 ; x <= BOARDSIZE; ++x) {
    for (int y = 1 ; y <= BOARDSIZE; ++y) {
		if(game_length < SAFEMOVE &&( x == 1 || y == 1 || x == BOARDSIZE || y == BOARDSIZE))continue;
        // check if current 
        if (Board[x][y] == 0) {
            // check the liberty of the neighborhood intersections
            num_neighborhood_self = 0; num_neighborhood_oppo = 0;
            num_neighborhood_empt = 0; num_neighborhood_boun = 0;
            // count the number of empy, self, opponent, and boundary neighboorhood
            count_neighboorhood_state(Board, x, y, turn, &num_neighborhood_empt, &num_neighborhood_self, &num_neighborhood_oppo, &num_neighborhood_boun, NeighboorhoodState);
            // check if the emtpy intersection is a legal move
            next_x = next_y = 0;
            eat_move = 0;
            count_liberty(x, y, Board, Liberties);
            // Case 1: exist empty intersection in the neighborhood
            if (num_neighborhood_empt > 0) {
             next_x = x;
             next_y = y;
             // check if it is a capture move
            for (int d = 0 ; d < MAXDIRECTION; ++d) {
                if (NeighboorhoodState[d] == OPPONENT && Liberties[d] == 1) {
                    eat_move = 1;
                }   
            }

            }
            // Case 2: no empty intersection in the neighborhood
            else {
                // Case 2.1: Surround by the self piece
                if (num_neighborhood_self + num_neighborhood_boun == MAXDIRECTION) {
                    int check_flag = 0, check_eye_flag = num_neighborhood_boun;
                    for (int d = 0 ; d < MAXDIRECTION; ++d) {
                        // Avoid fill self eye
                        if (NeighboorhoodState[d]==SELF && Liberties[d] > 1)
                            check_eye_flag++;
                        // Check if there is one self component which has more than one liberty
                        if (NeighboorhoodState[d]==SELF && Liberties[d] > 1)
                            check_flag = 1;
                    }
                    if (check_flag == 1 && check_eye_flag!=4) {
                        next_x = x;
                        next_y = y;
                    }
                }   
                // Case 2.2: Surround by opponent or both side's pieces.
                else if (num_neighborhood_oppo > 0) {
                    int check_flag = 0;
                    int eat_flag = 0;
                    for (int d = 0 ; d < MAXDIRECTION; ++d) {
                        // Check if there is one self component which has more than one liberty
                        if (NeighboorhoodState[d]==SELF && Liberties[d] > 1)
                            check_flag = 1;
                        // Check if there is one opponent's component which has exact one liberty
                        if (NeighboorhoodState[d]==OPPONENT && Liberties[d] == 1) 
                            eat_flag = 1;
                    }
                    if (check_flag == 1) {
                        next_x = x;
                        next_y = y;
                        if (eat_flag == 1)
                            eat_move = 1;
                    }
                    else { // check_flag == 0
                        if (eat_flag == 1) {
                            next_x = x;
                            next_y = y;
                            eat_move = 1;
                        }
                    }
                }   
            }
            if (next_x !=0 && next_y !=0) {
                // copy the current board to next board
                for (int i = 0 ; i < BOUNDARYSIZE; ++i)for (int j = 0 ; j < BOUNDARYSIZE; ++j)NextBoard[i][j] = Board[i][j];
                // do the move
                // The move is a capture move and the board needs to be updated.
                if (eat_move == 1) 
                    update_board(NextBoard, next_x, next_y, turn);
                else 
                    NextBoard[x][y] = turn;
                // Check the history to avoid the repeat board
                bool repeat_move = 0;
                for (int t = 0 ; t < game_length; ++t) {
                    bool repeat_flag = 1;
                    for (int i = 1; i <=BOARDSIZE; ++i) {
                    for (int j = 1; j <=BOARDSIZE; ++j) {
                        if (NextBoard[i][j] != GameRecord[t][i][j])
                            repeat_flag = 0;
                    }
                    }
                    if (repeat_flag == 1) {
                        repeat_move = 1;
                        break;
                    }
                }
                if (repeat_move == 0) {
                    // 3 digit zxy, z means eat or not, and put at (x, y)
                    MoveList[legal_moves] = eat_move * 100 + next_x * 10 + y ;
                    legal_moves++;
                }
            }
        }
    }
    }
    return legal_moves;
}
/*
 * This function randomly selects one move from the MoveList.
 * */
int rand_pick_move(int num_legal_moves, int MoveList[HISTORYLENGTH]) {
    if (num_legal_moves == 0)
    return 0;
    else {
    int move_id = rand()%num_legal_moves;
    return MoveList[move_id];
    }
}
/*
 * This function simlates the rand_move a single game.
 * */
int rand_simulate(int Board[BOUNDARYSIZE][BOUNDARYSIZE], int turn, int game_length, int GameRecord[MAXGAMELENGTH][BOUNDARYSIZE][BOUNDARYSIZE]){

    int ori_turn = turn;
    int MoveList[HISTORYLENGTH];
    int return_move,num_legal_moves;
    bool passornot[2]={false,false};
    //printf("start game_length = %d\n",game_length);
    while(!(passornot[0]&&passornot[1])){
        turn = (turn%2)+1;
        num_legal_moves = gen_legal_move(Board, turn, game_length, GameRecord, MoveList);
        //printf("num_legal_moves = %d",num_legal_moves);
        //getchar();getchar();
        if(num_legal_moves == 0) passornot[turn - 1] = true;
        else passornot[turn - 1] = false;
        if(passornot[0] && passornot[1])break;
        return_move = rand_pick_move(num_legal_moves,MoveList);
        do_move(Board,turn,return_move);
        record(Board, GameRecord, game_length+1);
        //printf("game_length = %d",game_length);
        game_length ++;
    }
    int result = final_score(Board) - _komi;
    if(result == 0) return 0;
    else if(result > 0 ^ ori_turn == BLACK) return -1;
    else return 1;
}
/*
 * This function selects one move with Monte-Carlo pure search.
 * */
int MCS_pure_move(int Board[BOUNDARYSIZE][BOUNDARYSIZE], int num_legal_moves, int MoveList[HISTORYLENGTH], clock_t end_t, int turn, int game_length, int GameRecord[MAXGAMELENGTH][BOUNDARYSIZE][BOUNDARYSIZE]){
    if(num_legal_moves == 0)return 0;
    int record[num_legal_moves];
    int num_simulate = 0;
    for(int i=0;i<num_legal_moves;i++) record[i]=0;
    while(clock() < end_t){
        for(int i=0;i<num_legal_moves;i++){
            for(int j=0;j<BOUNDARYSIZE;j++){
                for(int k=0;k<BOUNDARYSIZE;k++){
                    GameRecord[game_length+1][j][k] = Board[j][k];
                }
            }
            do_move(GameRecord[game_length+1],turn,MoveList[i]);
            num_simulate ++ ;
            //printf("outer game_length = %d\n",game_length);
            record[i] = record[i] + rand_simulate(GameRecord[game_length+1], turn, game_length+1, GameRecord);
        }
    }
    //printf("num_legal_moves = %d\n",num_legal_moves);
    int max = record[0];
    int max_index = 0;
    for(int i=1;i<num_legal_moves;i++){
        //printf("%d ",record[i]);
        if(record[i]>max){
            max = record[i];
            max_index = i;
        }
    }
    //printf("num_simulate = %d\n",num_simulate);
    cerr << "max_index = " << max_index << endl;
    cerr << "move = " << MoveList[max_index] << endl;
    return MoveList[max_index];
}
int MCTS_move(int Board[BOUNDARYSIZE][BOUNDARYSIZE], clock_t end_t, int turn, int game_length,int GameRecord[MAXGAMELENGTH][BOUNDARYSIZE][BOUNDARYSIZE]){
    
	int depth;
    
    MCSNODE* cur_node;
    MCSNODE* root_node = (MCSNODE*)malloc(sizeof(MCSNODE));
    root_node -> parent = NULL;
    root_node -> win = 0;
    root_node -> visit = 0;
	root_node -> move = 0;
    root_node -> turn = turn;
    root_node -> Board = Board;
    gtp_showboard(root_node -> Board);
    cur_node = root_node;
    
    int best_child;
    double best_UCB,tmp_UCB;
    
    int num_legal_moves;
    int MoveList[HISTORYLENGTH];
    
    bool move_check;
    int move[2];
	clock_t s_t, e_t;
	int CurBoard[BOUNDARYSIZE][BOUNDARYSIZE];
	vector<int> rand_array;
	for(int i=0;i<BOARDSIZE*BOARDSIZE;i++)rand_array.push_back(10*(i/9)+i%9+11);
	double score;
	int BorW,visit;
	
    while(clock() < end_t){
		depth = game_length;
		
        //selection
        while(cur_node -> child.size()!=0){
            best_UCB = (cur_node -> child[0] -> win)/(cur_node -> child[0] -> visit) + sqrt(UCBCONSTANT*log(cur_node -> visit)/cur_node -> child[0] -> visit);
            best_child = 0;
            for(int i=1;i<cur_node -> child.size();i++){
                tmp_UCB = (cur_node -> child[i] -> win)/(cur_node -> child[i] -> visit) + sqrt(UCBCONSTANT*log(cur_node -> visit)/cur_node -> child[i] -> visit);
                if(tmp_UCB > best_UCB){
                    best_UCB = tmp_UCB;
                    best_child = i;
                }
            }
            cur_node = cur_node -> child[best_child];
			depth ++;
        }
		
        //expansion
        num_legal_moves = gen_legal_move(Board, turn, game_length, GameRecord, MoveList);
        for(int i=0;i<num_legal_moves;i++){
            MCSNODE* new_node = (MCSNODE*)malloc(sizeof(MCSNODE));
            new_node -> parent = cur_node;
            new_node -> win = 0;
            new_node -> visit = 0;
			new_node -> move = MoveList[i];
            new_node -> turn = (cur_node -> turn%2)+1;
            do_move(new_node -> Board, new_node -> turn, MoveList[i]);
            cur_node -> child.push_back(new_node);
        }
    
        //simulation
		e_t = clock() + CLOCKS_PER_SEC* SIMULATETIME;
			for(int i=0;i<cur_node -> child.size();i++){
				turn = cur_node -> child[i] -> turn;
				for(int x=0;x<BOUNDARYSIZE;x++)for(int y=0;y<BOUNDARYSIZE;x++)CurBoard[x][y] = cur_node -> child[i] ->Board[x][y];
				score = 0.0;
				for(int j=0;j<ONCESIMULATE;j++){
					do{
						random_shuffle(rand_array.begin(), rand_array.end());
						move_check = false;
						for(int i=0;i<rand_array.size();i++){
							if((update_board_check(CurBoard, rand_array[i]/10, rand_array[i]%10, turn))==1){
								move_check = true;
								if(move[turn] == rand_array[i]) continue;
								move[turn] = rand_array[i];
								break;
							}
						}
						if(!move_check)move[turn] = 0;
						turn = (turn%2)+1;
						depth ++;
					}while(!(move[0]==0&&move[1]==0) && depth < HISTORYLENGTH);
					score = score + final_score(CurBoard);
					visit = visit + 1;
				}
				BorW = 2*cur_node -> child[i] -> turn - 3;
				cur_node -> child[i] -> win = cur_node -> child[i] -> win + BorW*score;
				cur_node -> child[i] -> visit = visit;
			}
			
        //back propagation 
		while(cur_node != NULL){
			if(cur_node -> turn == BLACK) cur_node -> win = cur_node -> win + score;
			else cur_node -> win = cur_node -> win - score;
			cur_node -> visit = cur_node -> visit + visit;
			cur_node = cur_node -> parent;
		}
    }
	double best_score = root_node -> child[0] -> win;
	int return_move = root_node -> child[0] -> move;
	for(int i=0;root_node -> child.size();i++){
		if(root_node -> child[i] -> win > best_score)
			return_move = root_node -> child[i] -> move;
	}
    return return_move;
}
/*
 * This function update the Board with put 'turn' at (x,y)
 * where x = (move % 100) / 10 and y = move % 10.
 * Note this function will not check 'move' is legal or not.
 * */
void do_move(int Board[BOUNDARYSIZE][BOUNDARYSIZE], int turn, int move) {
    int move_x = (move % 100) / 10;
    int move_y = move % 10;
    if (move<100)
        Board[move_x][move_y] = turn;
    else 
        update_board(Board, move_x, move_y, turn);
}
/* 
 * This function records the current game baord with current
 * game length "game_length"
 * */
void record(int Board[BOUNDARYSIZE][BOUNDARYSIZE], int GameRecord[MAXGAMELENGTH][BOUNDARYSIZE][BOUNDARYSIZE], int game_length) {
        for (int i = 0 ; i < BOUNDARYSIZE; ++i) {
            for (int j = 0 ; j < BOUNDARYSIZE; ++j) {
            GameRecord[game_length][i][j] = Board[i][j];
            }
        }
}
/* 
 * This function randomly generate one legal move (x, y) with return value x*10+y,
 * if there is no legal move the function will return 0.
 * */
int genmove(int Board[BOUNDARYSIZE][BOUNDARYSIZE], int turn, int time_limit, int game_length, int GameRecord[MAXGAMELENGTH][BOUNDARYSIZE][BOUNDARYSIZE]) {
    clock_t start_t, end_t;
    // record start time
    start_t = clock();
    // calculate the time bound
    end_t = start_t + CLOCKS_PER_SEC * time_limit;


    int return_move = 0;

    //num_legal_moves = gen_legal_move(Board, turn, game_length, GameRecord, MoveList);
    
    return_move = MCTS_move(Board, end_t, turn, game_length, GameRecord);
    //return_move = rand_pick_move(num_legal_moves, MoveList);
    //return_move = MCS_pure_move(Board,num_legal_moves, MoveList, end_t, turn, game_length, GameRecord);

    do_move(Board, turn, return_move);
    return return_move % 100;
}
/*
 * This function counts the number of points remains in the board by Black's view
 * */
double final_score(int Board[BOUNDARYSIZE][BOUNDARYSIZE]) {
    int black, white;
    black = white = 0;
    int is_black, is_white;
    for (int i = 1 ; i <= BOARDSIZE; ++i) {
    for (int j = 1; j <= BOARDSIZE; ++j) {
        switch(Board[i][j]) {
        case EMPTY:
            is_black = is_white = 0;
            for(int d = 0 ; d < MAXDIRECTION; ++d) {
            if (Board[i+DirectionX[d]][j+DirectionY[d]] == BLACK) is_black = 1;
            if (Board[i+DirectionX[d]][j+DirectionY[d]] == WHITE) is_white = 1;
            }
            if (is_black + is_white == 1) {
            black += is_black;
            white += is_white;
            }
            break;
        case WHITE:
            white++;
            break;
        case BLACK:
            black++;
            break;
        }
    }
    }
    return black - white;
}
/* 
 * Following are commands for Go Text Protocol (GTP)
 *
 * */
const char *KnownCommands[]={
    "protocol_version",
    "name",
    "version",
    "known_command",
    "list_commands",
    "quit",
    "boardsize",
    "clear_board",
    "komi",
    "play",
    "genmove",
    "undo",
    "quit",
    "showboard",
    "final_score"
};

void gtp_final_score(int Board[BOUNDARYSIZE][BOUNDARYSIZE]) {
    double result;
    result = final_score(Board);
    result -= _komi;
    cout << "= ";
    if (result > 0.0) { // Black win
    cout << "B+" << result << endl << endl<< endl;;
    }
    if (result < 0.0) { // White win
    cout << "W+" << -result << endl << endl<< endl;;
    }
    else { // draw
    cout << "0" << endl << endl<< endl;;
    }
}
void gtp_undo(int Board[BOUNDARYSIZE][BOUNDARYSIZE], int game_length, int GameRecord[MAXGAMELENGTH][BOUNDARYSIZE][BOUNDARYSIZE]) {
    if (game_length!=0) {
    for (int i = 1; i <= BOARDSIZE; ++i) {
        for (int j = 1; j <= BOARDSIZE; ++j) {
        Board[i][j] = GameRecord[game_length][i][j];
        }
    }
    }
    cout << "= " << endl << endl;
}
void gtp_showboard(int Board[BOUNDARYSIZE][BOUNDARYSIZE]) {
    for (int i = 1; i <=BOARDSIZE; ++i) {
    cout << "#";
    cout <<10-i;
    for (int j = 1; j <=BOARDSIZE; ++j) {
        switch(Board[i][j]) {
        case EMPTY: cout << " .";break;
        case BLACK: cout << " X";break;
        case WHITE: cout << " O";break;
        }
    }
    cout << endl;
    }
    cout << "#  ";
    for (int i = 1; i <=BOARDSIZE; ++i) 
    cout << LabelX[i] <<" ";
    cout << endl;
    cout << endl;

}
void gtp_protocol_version() {
    cout <<"= 2"<<endl<< endl;
}
void gtp_name() {
    cout <<"= TCG-randomGo99" << endl<< endl;
}
void gtp_version() {
    cout << "= 1.02" << endl << endl;
}
void gtp_list_commands(){
    cout <<"= ";
    for (int i = 0 ; i < NUMGTPCOMMANDS; ++i) {
    cout <<KnownCommands[i] << endl;
    }
    cout << endl;
}
void gtp_known_command(const char Input[]) {
    for (int i = 0 ; i < NUMGTPCOMMANDS; ++i) {
    if (strcmp(Input, KnownCommands[i])==0) {
        cout << "= true" << endl<< endl;
        return;
    }
    }
    cout << "= false" << endl<< endl;
}
void gtp_boardsize(int size) {
    if (size!=9) {
    cout << "? unacceptable size" << endl<< endl;
    }
    else {
    _board_size = size;
    cout << "= "<<endl<<endl;
    }
}
void gtp_clear_board(int Board[BOUNDARYSIZE][BOUNDARYSIZE], int NumCapture[]) {
    reset(Board);
    NumCapture[BLACK] = NumCapture[WHITE] = 0;
    cout << "= "<<endl<<endl;
}
void gtp_komi(double komi) {
    _komi = komi;
    cout << "= "<<endl<<endl;
}
void gtp_play(char Color[], char Move[], int Board[BOUNDARYSIZE][BOUNDARYSIZE], int game_length, int GameRecord[MAXGAMELENGTH][BOUNDARYSIZE][BOUNDARYSIZE]) {
    int turn, move_i, move_j;
    if (Color[0] =='b' || Color[0] == 'B')
    turn = BLACK;
    else
    turn = WHITE;
    if (strcmp(Move, "PASS") == 0 || strcmp(Move, "pass")==0) {
    record(Board, GameRecord, game_length+1);
    }
    else {
    // [ABCDEFGHJ][1-9], there is no I in the index.
    Move[0] = toupper(Move[0]);
    move_j = Move[0]-'A'+1;
    if (move_j == 10) move_j = 9;
    move_i = 10-(Move[1]-'0');
    update_board(Board, move_i, move_j, turn);
    record(Board, GameRecord, game_length+1);
    }
    cout << "= "<<endl<<endl;
}
void gtp_genmove(int Board[BOUNDARYSIZE][BOUNDARYSIZE], char Color[], int time_limit, int game_length, int GameRecord[MAXGAMELENGTH][BOUNDARYSIZE][BOUNDARYSIZE]){
    int turn = (Color[0]=='b'||Color[0]=='B')?BLACK:WHITE;
    int move = genmove(Board, turn, time_limit, game_length, GameRecord);
    int move_i, move_j;
    record(Board, GameRecord, game_length+1);
    if (move==0)
        cout << "= PASS" << endl<< endl<< endl;
    else {
    move_i = (move%100)/10;
    move_j = (move%10);
//  cerr << "#turn("<<game_length<<"): (move, move_i,move_j)" << turn << ": " << move<< " " << move_i << " " << move_j << endl;
    cout << "= " << LabelX[move_j]<<10-move_i<<endl<< endl;
    }
}
/*
 * This main function is used of the gtp protocol
 * */
void gtp_main(int display) {
    char Input[COMMANDLENGTH]="";
    char Command[COMMANDLENGTH]="";
    char Parameter[COMMANDLENGTH]="";
    char Move[4]="";
    char Color[6]="";
    int ivalue;
    double dvalue;
    int Board[BOUNDARYSIZE][BOUNDARYSIZE]={{0}};
    int NumCapture[3]={0};// 1:Black, 2: White
    int time_limit = DEFAULTTIME;
    int GameRecord[MAXGAMELENGTH][BOUNDARYSIZE][BOUNDARYSIZE]={{{0}}};
    int game_length = 0;
    if (display==1) {
    gtp_list_commands();
    gtp_showboard(Board);
    }
    while (gets(Input) != 0) {
    sscanf(Input, "%s", Command);
    if (Command[0]== '#')
        continue;

    if (strcmp(Command, "protocol_version")==0) {
        gtp_protocol_version();
    }
    else if (strcmp(Command, "name")==0) {
        gtp_name();
    }
    else if (strcmp(Command, "version")==0) {
        gtp_version();
    }
    else if (strcmp(Command, "list_commands")==0) {
        gtp_list_commands();
    }
    else if (strcmp(Command, "known_command")==0) {
        sscanf(Input, "known_command %s", Parameter);
        gtp_known_command(Parameter);
    }
    else if (strcmp(Command, "boardsize")==0) {
        sscanf(Input, "boardsize %d", &ivalue);
        gtp_boardsize(ivalue);
    }
    else if (strcmp(Command, "clear_board")==0) {
        gtp_clear_board(Board, NumCapture);
        game_length = 0;
    }
    else if (strcmp(Command, "komi")==0) {
        sscanf(Input, "komi %lf", &dvalue);
        gtp_komi(dvalue);
    }
    else if (strcmp(Command, "play")==0) {
        sscanf(Input, "play %s %s", Color, Move);
        gtp_play(Color, Move, Board, game_length, GameRecord);
        game_length++;
        if (display==1) {
        gtp_showboard(Board);
        }
    }
    else if (strcmp(Command, "genmove")==0) {
        sscanf(Input, "genmove %s", Color);
        gtp_genmove(Board, Color, time_limit, game_length, GameRecord);
        game_length++;
        if (display==1) {
        gtp_showboard(Board);
        }
    }
    else if (strcmp(Command, "quit")==0) {
        break;
    }
    else if (strcmp(Command, "showboard")==0) {
        gtp_showboard(Board);
    }
    else if (strcmp(Command, "undo")==0) {
        game_length--;
        gtp_undo(Board, game_length, GameRecord);
        if (display==1) {
        gtp_showboard(Board);
        }
    }
    else if (strcmp(Command, "final_score")==0) {
        if (display==1) {
        gtp_showboard(Board);
        }
        gtp_final_score(Board);
    }
    cerr << "game_length = " << game_length << endl;
    }
}
int main(int argc, char* argv[]) {
//    int type = GTPVERSION;// 1: local version, 2: gtp version
    int type = GTPVERSION;// 1: local version, 2: gtp version
    int display = 0; // 1: display, 2 nodisplay
    if (argc > 1) {
    if (strcmp(argv[1], "-display")==0) {
        display = 1;
    }
    if (strcmp(argv[1], "-nodisplay")==0) {
        display = 0;
    }
    }
    gtp_main(display);
    return 0;
}
