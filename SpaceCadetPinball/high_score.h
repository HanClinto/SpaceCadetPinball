#pragma once

struct high_score_struct
{
	char Name[32];
	int Score;
};

struct high_score_entry
{	
	high_score_struct Entry;
	int Position;
};


class high_score
{
public:
	static high_score_struct highscore_table[5];
	// Web build: the page keeps the table (Module.scoreboard, shared between players) instead of the settings file
	static bool External;

	static int read();
	static int write();
	static int get_score_position(int score);	

	static void show_high_score_dialog();
	static void show_and_set_high_score_dialog(high_score_entry score);
	static void RenderHighScoreDialog();
	static void set_external_entry(int position, const char* name, int score);
	static void submit_external(const int* scores, int playerCount, bool cheated);
private:
	static bool dlg_enter_name;
	static high_score_entry DlgData;
	static bool ShowDialog;
	static std::vector<high_score_entry> ScoreQueue;

	static void clear_table();
	static void place_new_score_into(high_score_entry data);
};
