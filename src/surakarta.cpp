#include <iostream>
#include <iterator>
#include <fstream>
#include <thread>
#include <mutex>
#include "board.h"
#include "action.h"
#include "agent.h"
#include "episode.h"
#include "statistic.h"
#include "utilities.h"
#include "mcts.h"

const std::string PLAYER[] = {"MCTS_with_tuple", "MCTS", "tuple", "eat_first"};
const std::string SIMULATION[] = {"(random)", "(eat-first)", "(tuple)"};
std::mutex mtx;
int fight_black_win, fight_white_win;

void fight_thread(int player1, int player2, int sim1, int sim2, Tuple *tuple, int game_count, uint32_t seeds) {
    MCTS mcts_tuple(tuple, true, seeds), mcts(tuple, false, seeds);
    TuplePlayer tuple_player(tuple);
    RandomPlayer random_player(seeds);
    
    int black_win = 0, white_win = 0;
    for (int i = 0; i < game_count; i++) {
        // std::cout << i << std::endl;
        Board board;
        int color = 0, count = 0, current, sim;

        while (!board.game_over() && count++ < 200) {
            current = color ? player2 : player1;
            sim = color ? sim2 : sim1;
            switch (current) {
                case 0:
                    mcts_tuple.playing(board, color, sim);
                    break;
                case 1:
                    mcts.playing(board, color, sim);
                    break;
                case 2:
                    tuple_player.playing(board, color);
                    break;
                case 3:
                    random_player.playing(board, color);
                    break;
                default:
                    break;
            }
            color ^= 1; // toggle color
        }

        int black_bitcount = Bitcount(board.get_board(0));
        int white_bitcount = Bitcount(board.get_board(1));
        if      (black_bitcount == 0) white_win++;
        else if (white_bitcount == 0) black_win++;
        else {
            if (black_bitcount > white_bitcount)    black_win++;
            if (black_bitcount < white_bitcount)    white_win++;
        }
    }
    mtx.lock();
    fight_black_win += black_win;
    fight_white_win += white_win;
    mtx.unlock();
    // std::cout << black_win << " " << white_win << std::endl;
}

void fight(int player1, int player2, int sim1, int sim2, Tuple *tuple, int game_count) {
    /** 
     * player 
     * 0 : MCTS with tuple
     * 1 : MCTS
     * 2 : tuple
     * 3 : eat first
     *
     * simulation
     * 0 : random
     * 1 : eat first
     * 2 : tuple
     */
    std::cout << PLAYER[player1];
    if (player1 <= 1)   std::cout << SIMULATION[sim1];
    std::cout << " VS " << PLAYER[player2];
    if (player2 <= 1)   std::cout << SIMULATION[sim2];
    std::cout << std::endl;

    fight_black_win = 0, fight_white_win = 0;
    std::vector<std::thread> threads;
    std::random_device rd;
    for(int i = 0; i < 5; i++) {
        threads.push_back(std::thread(fight_thread, player1, player2, sim1, sim2, tuple, game_count / 5, rd()));
    }
    for (auto& th : threads) {
        th.join();
    }

    std::cout << "Playing " << game_count << " episodes: \n";
    std::cout << std::fixed << std::setprecision(1);
    std::cout << "Black: " << fight_black_win * 100.0 / (fight_black_win + fight_white_win) << " %" << std::endl;
    std::cout << "White: " << fight_white_win * 100.0 / (fight_black_win + fight_white_win) << " %\n" << std::endl;
}

int main(int argc, const char* argv[]) {
    std::cout << "Surakarta: ";
    std::copy(argv, argv + argc, std::ostream_iterator<const char*>(std::cout, " "));
    std::cout << std::endl << std::endl;

    size_t total = 1000, block = 0, limit = 0, game_count = 2000;
    int sim1 = 1, sim2 = 1;
    std::string play1_args, play2_args, tuple_args;
    bool summary = false;

    for (int i = 1; i < argc; i++) {
        std::string para(argv[i]);
        if (para.find("--total=") == 0) {
            total = std::stoull(para.substr(para.find("=") + 1));
        } else if (para.find("--block=") == 0) {
            block = std::stoull(para.substr(para.find("=") + 1));
        } else if (para.find("--limit=") == 0) {
            limit = std::stoull(para.substr(para.find("=") + 1));
        } else if (para.find("--game=") == 0) {
            game_count = std::stoull(para.substr(para.find("=") + 1));
        } else if (para.find("--play1=") == 0) {
            play1_args = para.substr(para.find("=") + 1);
        } else if (para.find("--play2=") == 0) {
            play2_args = para.substr(para.find("=") + 1);
        } else if (para.find("--sim1=") == 0) {
            sim1 = std::stoull(para.substr(para.find("=") + 1));
        } else if (para.find("--sim2=") == 0) {
            sim2 = std::stoull(para.substr(para.find("=") + 1));
        } else if (para.find("--tuple=") == 0) {
            tuple_args = para.substr(para.find("=") + 1);
        } else if (para.find("--summary") == 0) {
            summary = true;
        }
    }

    Tuple tuple(tuple_args);
    Statistic stat(total, block, limit);
    TrainingPlayer play1(0, &tuple), play2(1, &tuple);

    while (!stat.is_finished()) {     
        play1.open_episode();
        play2.open_episode();
        stat.open_episode(play1.role() + ":" + play2.role());
        Episode& game = stat.back();
        // int count = 100;
        while (true) {
            TrainingPlayer& who = game.take_turns(play1, play2);
            Action action = who.take_action(game.state());

            if (game.apply_action(action) != true) break;
            if (who.check_for_win(game.state())) break;
            // if (--count == 0) break;
        }
        
        int black_bitcount = Bitcount(game.state().get_board(0));
        int white_bitcount = Bitcount(game.state().get_board(1));
        std::string win_bitcount = std::to_string(black_bitcount - white_bitcount);
        std::string win;
        if (black_bitcount > white_bitcount) win = "Black";
        else if (black_bitcount < white_bitcount) win = "White";
        else win = "Draw";
        play1.close_episode(win_bitcount);
        play2.close_episode(win_bitcount);
        stat.close_episode(win);

        // after training some episodes, test playing result
        if (stat.episode_count() % 100 == 0) {
            fight(2, 3, sim1, sim2, &tuple, game_count);
            fight(3, 2, sim2, sim1, &tuple, game_count);
        }
    }

    if (summary) {
        stat.summary();
    }

    return 0;
}
