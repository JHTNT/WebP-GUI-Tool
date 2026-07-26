#pragma once
#include <FL/Fl_Double_Window.H>

#include <string>
#include <unordered_set>
#include <vector>

#include "cwebp_runner.h"
#include "job_model.h"

class FileTree;
class Fl_Tree_Item;
class Fl_Box;
class Fl_Button;
class Fl_Check_Button;
class Fl_Choice;
class Fl_Group;
class Fl_Input;
class Fl_Int_Input;
class Fl_Output;
class Fl_Progress;
class Fl_Round_Button;
class Fl_Spinner;
class Fl_Toggle_Button;
class Fl_Value_Slider;

class MainWindow : public Fl_Double_Window {
public:
    MainWindow();
    int handle(int event) override;

private:
    std::string label_for(const FileItem& it) const;
    CwebpOptions read_options() const;
    void add_dropped(const std::string& text);
    void add_paths(const std::vector<std::string>& paths_utf8);
    void rebuild_tree();
    void refresh_leaf(int idx);
    void set_all_checked(bool checked);
    void remove_items(const std::vector<int>& indices);
    void show_context_menu(Fl_Tree_Item* item);
    void update_idle_status();
    void toggle_advanced();
    void apply_z_state();
    void set_running_ui(bool running);
    void start_conversion();
    void browse_output_dir();
    void on_tree_event();
    void on_msg(const Converter::Msg& m);
    void on_close();
    static void awake_cb(void* p);

    std::vector<FileItem> items_;
    std::vector<Fl_Tree_Item*> leaf_;         // item index -> tree leaf
    std::unordered_set<std::wstring> keys_;   // lowercased paths for dedup
    Converter converter_;
    std::string cwebp_path_;
    std::string out_dir_utf8_;

    Fl_Group* toolbar_;
    Fl_Check_Button* recursive_;
    FileTree* tree_;
    Fl_Value_Slider* quality_;
    Fl_Toggle_Button* adv_toggle_;
    Fl_Group* adv_group_;
    Fl_Check_Button* lossless_;
    Fl_Check_Button* nl_chk_;
    Fl_Spinner* nl_val_;
    Fl_Choice* z_choice_;
    Fl_Choice* m_choice_;
    Fl_Choice* preset_choice_;
    Fl_Choice* meta_choice_;
    Fl_Check_Button* resize_chk_;
    Fl_Int_Input* rw_;
    Fl_Int_Input* rh_;
    Fl_Check_Button* mt_;
    Fl_Check_Button* sharp_;
    Fl_Check_Button* aq_chk_;
    Fl_Spinner* aq_val_;
    Fl_Input* extra_;
    Fl_Group* bottom_group_;
    Fl_Round_Button* out_same_;
    Fl_Round_Button* out_custom_;
    Fl_Output* out_dir_;
    Fl_Button* out_browse_;
    Fl_Button* start_btn_;
    Fl_Button* cancel_btn_;
    Fl_Progress* progress_;
    Fl_Box* status_box_;
};
