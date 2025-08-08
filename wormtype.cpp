#include <ncurses.h>   // Terminal UI library
#include <string>      // String class
#include <vector>      // Dynamic arrays
#include <ctime>       // Time functions
#include <cstdlib>     // Random number generation and atexit()
#include <fstream>     // File I/O
#include <algorithm>   // For sorting
#include <iomanip>     // For formatting
#include <unistd.h>    // For usleep() delay function
#include <signal.h>    // For signal handling
#include <cctype>      // For toupper()

// Utility function to convert string to uppercase
std::string toUpperCase(const std::string& str) {
    std::string result = str;
    std::transform(result.begin(), result.end(), result.begin(), ::toupper);
    return result;
}

// Forward declarations
void drawBouncyWorm(int y, int start_x, int width, double position, int frame);
void drawDecorativeWorm(int y, int start_x, int width, int color_pair, bool reverse_direction, int frame);
void initializeAchievements();
void saveAchievements();
void loadAchievements();
void checkAchievements(double wpm, double accuracy, double time);


// Achievement system
struct Achievement {
    std::string id;
    std::string name;
    std::string description;
    bool unlocked;
    
    Achievement(const std::string& i, const std::string& n, const std::string& d) 
        : id(i), name(n), description(d), unlocked(false) {}
};

// Global achievements list
std::vector<Achievement> achievements;
std::string equippedWormColor = "default";  // default, pink, etc.

// Structure to hold player score data
struct PlayerScore {
    std::string name;
    double wpm;
    double accuracy;
    double time;
    std::string date;
    int wordCount;
    bool hasPunctuation;
    bool hasNumbers;
    
    // Constructor with all options
    PlayerScore(const std::string& n, double w, double a, double t, const std::string& d, int wc, bool punct, bool nums) 
        : name(n), wpm(w), accuracy(a), time(t), date(d), wordCount(wc), hasPunctuation(punct), hasNumbers(nums) {}
    
    // Constructor with word count and options but without date (auto-generates date)
    PlayerScore(const std::string& n, double w, double a, double t, int wc, bool punct, bool nums) 
        : name(n), wpm(w), accuracy(a), time(t), wordCount(wc), hasPunctuation(punct), hasNumbers(nums) {
        // Get current date/time
        time_t rawtime;
        struct tm* timeinfo;
        char buffer[80];
        
        ::time(&rawtime);
        timeinfo = localtime(&rawtime);
        strftime(buffer, sizeof(buffer), "%m/%d/%Y %H:%M", timeinfo);
        date = std::string(buffer);
    }
    
    // Constructor with date and word count (backward compatibility - no punctuation/numbers)
    PlayerScore(const std::string& n, double w, double a, double t, const std::string& d, int wc) 
        : name(n), wpm(w), accuracy(a), time(t), date(d), wordCount(wc), hasPunctuation(false), hasNumbers(false) {}
    
    // Constructor with word count but without date (backward compatibility - no punctuation/numbers)
    PlayerScore(const std::string& n, double w, double a, double t, int wc) 
        : name(n), wpm(w), accuracy(a), time(t), wordCount(wc), hasPunctuation(false), hasNumbers(false) {
        // Get current date/time
        time_t rawtime;
        struct tm* timeinfo;
        char buffer[80];
        
        ::time(&rawtime);
        timeinfo = localtime(&rawtime);
        strftime(buffer, sizeof(buffer), "%m/%d/%Y %H:%M", timeinfo);
        date = std::string(buffer);
    }
    
    // Constructor with date (for backward compatibility - assumes 15 words, no punctuation/numbers)
    PlayerScore(const std::string& n, double w, double a, double t, const std::string& d) 
        : name(n), wpm(w), accuracy(a), time(t), date(d), wordCount(15), hasPunctuation(false), hasNumbers(false) {}
    
    // Constructor without date (for backward compatibility - assumes 15 words, no punctuation/numbers)
    PlayerScore(const std::string& n, double w, double a, double t) 
        : name(n), wpm(w), accuracy(a), time(t), wordCount(15), hasPunctuation(false), hasNumbers(false) {
        // Get current date/time
        time_t rawtime;
        struct tm* timeinfo;
        char buffer[80];
        
        ::time(&rawtime);
        timeinfo = localtime(&rawtime);
        strftime(buffer, sizeof(buffer), "%m/%d/%Y %H:%M", timeinfo);
        date = std::string(buffer);
    }
    
    // Default constructor for file loading
    PlayerScore() : name(""), wpm(0), accuracy(0), time(0), date(""), wordCount(15), hasPunctuation(false), hasNumbers(false) {}
};

// Function to get unique player names from leaderboard
std::vector<std::string> getUniquePlayerNames(const std::vector<PlayerScore>& leaderboard) {
    std::vector<std::string> uniqueNames;
    
    for (size_t i = 0; i < leaderboard.size(); i++) {
        const std::string& name = leaderboard[i].name;
        bool found = false;
        
        // Check if name already exists in uniqueNames
        for (size_t j = 0; j < uniqueNames.size(); j++) {
            if (uniqueNames[j] == name) {
                found = true;
                break;
            }
        }
        
        if (!found) {
            uniqueNames.push_back(name);
        }
    }
    
    return uniqueNames;
}

// Function to show name selection menu
std::string showNameSelectionMenu(const std::vector<std::string>& names) {
    int max_x, max_y;
    int choice = 0;
    int ch;
    
    while (true) {
        getmaxyx(stdscr, max_y, max_x);
        clear();
        
        // Calculate box dimensions
        int box_width = 40;
        int box_height = names.size() + 7;  // Title + names + new name option + padding
        if (box_height > max_y - 4) box_height = max_y - 4;
        int box_start_x = (max_x - box_width) / 2;
        int box_start_y = (max_y - box_height) / 2;
        
        // Draw retro box border
        // Top border
        mvaddch(box_start_y, box_start_x, ACS_ULCORNER);
        for (int x = box_start_x + 1; x < box_start_x + box_width - 1; x++) {
            mvaddch(box_start_y, x, ACS_HLINE);
        }
        mvaddch(box_start_y, box_start_x + box_width - 1, ACS_URCORNER);
        
        // Side borders
        for (int y = box_start_y + 1; y < box_start_y + box_height - 1; y++) {
            mvaddch(y, box_start_x, ACS_VLINE);
            mvaddch(y, box_start_x + box_width - 1, ACS_VLINE);
        }
        
        // Bottom border
        mvaddch(box_start_y + box_height - 1, box_start_x, ACS_LLCORNER);
        for (int x = box_start_x + 1; x < box_start_x + box_width - 1; x++) {
            mvaddch(box_start_y + box_height - 1, x, ACS_HLINE);
        }
        mvaddch(box_start_y + box_height - 1, box_start_x + box_width - 1, ACS_LRCORNER);
        
        // Title inside box
        std::string title = "SELECT YOUR NAME";
        mvprintw(box_start_y + 2, box_start_x + (box_width - title.length()) / 2, "%s", title.c_str());
        
        // Draw separator line
        for (int x = box_start_x + 2; x < box_start_x + box_width - 2; x++) {
            mvaddch(box_start_y + 3, x, ACS_HLINE);
        }
        
        // Show previous names inside box
        int startY = box_start_y + 5;
        int center_x = box_start_x + box_width / 2;
        
        for (size_t i = 0; i < names.size() && i < 6; i++) {  // Show max 6 names to fit in box
            std::string displayName = toUpperCase(names[i]);
            if (displayName.length() > 32) {
                displayName = displayName.substr(0, 29) + "...";
            }
            
            // Highlight selected option - center the text
            if (choice == (int)i) {
                std::string fullOption = "[" + displayName + "]";
                int option_x = center_x - fullOption.length() / 2;
                mvprintw(startY + i, option_x, "%s", fullOption.c_str());
            } else {
                int option_x = center_x - displayName.length() / 2;
                mvprintw(startY + i, option_x, "%s", displayName.c_str());
            }
        }
        
        // Add "Enter new name" option inside box
        std::string newNameOption = "Create New Player";
        if (choice == (int)names.size()) {
            std::string fullOption = "[" + newNameOption + "]";
            int option_x = center_x - fullOption.length() / 2;
            mvprintw(startY + names.size(), option_x, "%s", fullOption.c_str());
        } else {
            int option_x = center_x - newNameOption.length() / 2;
            mvprintw(startY + names.size(), option_x, "%s", newNameOption.c_str());
        }
        
        
        refresh();
        
        // Instructions at bottom of terminal (outside box)
        std::string instructions = "WASD/Arrows + Enter | Q: Back";
        mvprintw(max_y - 2, (max_x - instructions.length()) / 2, "%s", instructions.c_str());
        
        // Add program-wide worm closet instruction
        std::string wormInstruction = "Press W for Worm Closet";
        mvprintw(max_y - 1, (max_x - wormInstruction.length()) / 2, "%s", wormInstruction.c_str());
        
        ch = getch();
        if (ch == 'w' || ch == 'W') {
            return "WORM_CLOSET";  // Signal to open worm closet
        } else if (ch == KEY_UP && choice > 0) {
            choice--;
        } else if ((ch == KEY_DOWN || ch == 's' || ch == 'S') && choice < (int)names.size()) {  // names.size() + "new name"
            choice++;
        } else if (ch == 10 || ch == 13) { // Enter
            if (choice < (int)names.size()) {
                return names[choice];  // Return selected existing name
            } else if (choice == (int)names.size()) {
                return "";  // Signal to enter new name
            }
        } else if (ch == 'q' || ch == 'Q') { // Q - go back to main menu
            return "CANCEL";
        }
    }
}

// Function to get new player name input
std::string getNewPlayerName() {
    std::string name;
    int ch;
    int max_x, max_y;
    
    while (true) {
        getmaxyx(stdscr, max_y, max_x);
        clear();
        
        // Calculate box dimensions
        int box_width = 40;
        int box_height = 8;
        int box_start_x = (max_x - box_width) / 2;
        int box_start_y = (max_y - box_height) / 2;
        
        // Draw retro box border
        // Top border
        mvaddch(box_start_y, box_start_x, ACS_ULCORNER);
        for (int x = box_start_x + 1; x < box_start_x + box_width - 1; x++) {
            mvaddch(box_start_y, x, ACS_HLINE);
        }
        mvaddch(box_start_y, box_start_x + box_width - 1, ACS_URCORNER);
        
        // Side borders
        for (int y = box_start_y + 1; y < box_start_y + box_height - 1; y++) {
            mvaddch(y, box_start_x, ACS_VLINE);
            mvaddch(y, box_start_x + box_width - 1, ACS_VLINE);
        }
        
        // Bottom border
        mvaddch(box_start_y + box_height - 1, box_start_x, ACS_LLCORNER);
        for (int x = box_start_x + 1; x < box_start_x + box_width - 1; x++) {
            mvaddch(box_start_y + box_height - 1, x, ACS_HLINE);
        }
        mvaddch(box_start_y + box_height - 1, box_start_x + box_width - 1, ACS_LRCORNER);
        
        // Title inside box
        std::string title = "CREATE PLAYER";
        mvprintw(box_start_y + 1, box_start_x + (box_width - title.length()) / 2, "%s", title.c_str());
        
        // Draw separator line
        for (int x = box_start_x + 2; x < box_start_x + box_width - 2; x++) {
            mvaddch(box_start_y + 2, x, ACS_HLINE);
        }
        
        // Input field inside box
        mvprintw(box_start_y + 4, box_start_x + 3, "Name: ");
        
        // Draw input field border
        int input_start_x = box_start_x + 9;
        int input_width = box_width - 12;
        mvaddch(box_start_y + 4, input_start_x - 1, '[');
        mvaddch(box_start_y + 4, input_start_x + input_width, ']');
        
        // Display current name with cursor
        std::string displayName = name;
        if (displayName.length() > (size_t)(input_width - 1)) {
            displayName = displayName.substr(0, input_width - 1);
        }
        mvprintw(box_start_y + 4, input_start_x, "%-*s", input_width, displayName.c_str());
        
        // Show cursor at input position
        curs_set(1);
        move(box_start_y + 4, input_start_x + displayName.length());
        
        refresh();
        
        // Instructions at bottom of terminal (outside box)
        std::string instructions = name.empty() ? "Type your name..." : "Enter: Confirm | Q: Back";
        mvprintw(max_y - 1, (max_x - instructions.length()) / 2, "%s", instructions.c_str());
        
        ch = getch();
        if ((ch == 10 || ch == 13) && !name.empty()) { // Enter key and name not empty
            break;
        } else if (ch == 'q' || ch == 'Q') { // Q - go back
            return "CANCEL";
        } else if (ch == KEY_BACKSPACE || ch == 127 || ch == 8) {
            if (!name.empty()) {
                name.pop_back();
            }
        } else if (ch >= 32 && ch <= 126 && name.length() < 20) { // Printable chars, max 20
            name += (char)ch;
        }
    }
    
    return toUpperCase(name);
}

// Function to get custom word count from user input
int getCustomWordCount() {
    std::string input;
    int ch;
    int max_x, max_y;
    
    while (true) {
        getmaxyx(stdscr, max_y, max_x);
        clear();
        
        // Calculate box dimensions
        int box_width = 40;
        int box_height = 8;
        int box_start_x = (max_x - box_width) / 2;
        int box_start_y = (max_y - box_height) / 2;
        
        // Draw retro box border
        // Top border
        mvaddch(box_start_y, box_start_x, ACS_ULCORNER);
        for (int x = box_start_x + 1; x < box_start_x + box_width - 1; x++) {
            mvaddch(box_start_y, x, ACS_HLINE);
        }
        mvaddch(box_start_y, box_start_x + box_width - 1, ACS_URCORNER);
        
        // Side borders
        for (int y = box_start_y + 1; y < box_start_y + box_height - 1; y++) {
            mvaddch(y, box_start_x, ACS_VLINE);
            mvaddch(y, box_start_x + box_width - 1, ACS_VLINE);
        }
        
        // Bottom border
        mvaddch(box_start_y + box_height - 1, box_start_x, ACS_LLCORNER);
        for (int x = box_start_x + 1; x < box_start_x + box_width - 1; x++) {
            mvaddch(box_start_y + box_height - 1, x, ACS_HLINE);
        }
        mvaddch(box_start_y + box_height - 1, box_start_x + box_width - 1, ACS_LRCORNER);
        
        // Title inside box
        std::string title = "CUSTOM WORD COUNT";
        mvprintw(box_start_y + 1, box_start_x + (box_width - title.length()) / 2, "%s", title.c_str());
        
        // Draw separator line
        for (int x = box_start_x + 2; x < box_start_x + box_width - 2; x++) {
            mvaddch(box_start_y + 2, x, ACS_HLINE);
        }
        
        // Input field inside box
        mvprintw(box_start_y + 4, box_start_x + 3, "Words: ");
        
        // Draw input field border
        int input_start_x = box_start_x + 10;
        int input_width = box_width - 13;
        mvaddch(box_start_y + 4, input_start_x - 1, '[');
        mvaddch(box_start_y + 4, input_start_x + input_width, ']');
        
        // Display current input with cursor
        std::string displayInput = input;
        if (displayInput.length() > (size_t)(input_width - 1)) {
            displayInput = displayInput.substr(0, input_width - 1);
        }
        mvprintw(box_start_y + 4, input_start_x, "%-*s", input_width, displayInput.c_str());
        
        // Show cursor at input position
        curs_set(1);
        move(box_start_y + 4, input_start_x + displayInput.length());
        
        refresh();
        
        // Instructions at bottom of terminal (outside box)
        std::string instructions = input.empty() ? "Enter number of words (1-1000)..." : "Enter: Confirm | Q: Back";
        mvprintw(max_y - 1, (max_x - instructions.length()) / 2, "%s", instructions.c_str());
        
        ch = getch();
        if ((ch == 10 || ch == 13) && !input.empty()) { // Enter key and input not empty
            try {
                int wordCount = std::stoi(input);
                if (wordCount >= 1 && wordCount <= 1000) {
                    curs_set(0); // Hide cursor
                    return wordCount;
                } else {
                    input = ""; // Clear invalid input
                }
            } catch (const std::exception&) {
                input = ""; // Clear invalid input
            }
        } else if (ch == 'q' || ch == 'Q') { // Q - go back
            curs_set(0); // Hide cursor
            return -1;
        } else if (ch == KEY_BACKSPACE || ch == 127 || ch == 8) {
            if (!input.empty()) {
                input.pop_back();
            }
        } else if (ch >= '0' && ch <= '9' && input.length() < 4) { // Only digits, max 4 chars
            input += (char)ch;
        }
    }
}

// Function to show word count selection menu
int showWordCountMenu() {
    int max_x, max_y;
    int choice = 0;
    int ch;
    std::vector<int> wordCounts;
    wordCounts.push_back(5);
    wordCounts.push_back(10);
    wordCounts.push_back(25);
    wordCounts.push_back(50);
    
    while (true) {
        getmaxyx(stdscr, max_y, max_x);
        clear();
        
        // Calculate box dimensions
        int box_width = 30;
        int box_height = 12;  // Title + 4 options + padding
        int box_start_x = (max_x - box_width) / 2;
        int box_start_y = (max_y - box_height) / 2;
        
        // Draw retro box border
        // Top border
        mvaddch(box_start_y, box_start_x, ACS_ULCORNER);
        for (int x = box_start_x + 1; x < box_start_x + box_width - 1; x++) {
            mvaddch(box_start_y, x, ACS_HLINE);
        }
        mvaddch(box_start_y, box_start_x + box_width - 1, ACS_URCORNER);
        
        // Side borders
        for (int y = box_start_y + 1; y < box_start_y + box_height - 1; y++) {
            mvaddch(y, box_start_x, ACS_VLINE);
            mvaddch(y, box_start_x + box_width - 1, ACS_VLINE);
        }
        
        // Bottom border
        mvaddch(box_start_y + box_height - 1, box_start_x, ACS_LLCORNER);
        for (int x = box_start_x + 1; x < box_start_x + box_width - 1; x++) {
            mvaddch(box_start_y + box_height - 1, x, ACS_HLINE);
        }
        mvaddch(box_start_y + box_height - 1, box_start_x + box_width - 1, ACS_LRCORNER);
        
        // Title inside box
        std::string title = "WORD COUNT";
        mvprintw(box_start_y + 2, box_start_x + (box_width - title.length()) / 2, "%s", title.c_str());
        
        // Draw separator line
        for (int x = box_start_x + 2; x < box_start_x + box_width - 2; x++) {
            mvaddch(box_start_y + 3, x, ACS_HLINE);
        }
        
        // Show word count options inside box
        int startY = box_start_y + 5;
        for (size_t i = 0; i < wordCounts.size(); i++) {
            std::string option = std::to_string(wordCounts[i]) + " words";
            
            // Highlight selected option
            if (choice == (int)i) {
                mvprintw(startY + i, box_start_x + 3, "[%s]", option.c_str());
            } else {
                mvprintw(startY + i, box_start_x + 4, "%s", option.c_str());
            }
        }
        
        // Add "Custom" option
        std::string customOption = "Custom";
        if (choice == (int)wordCounts.size()) {
            mvprintw(startY + wordCounts.size(), box_start_x + 3, "[%s]", customOption.c_str());
        } else {
            mvprintw(startY + wordCounts.size(), box_start_x + 4, "%s", customOption.c_str());
        }
        
        refresh();
        
        // Instructions at bottom of terminal (outside box)
        std::string instructions = "WASD/Arrows + Enter | Q: Back";
        mvprintw(max_y - 1, (max_x - instructions.length()) / 2, "%s", instructions.c_str());
        
        ch = getch();
        if ((ch == KEY_UP || ch == 'w' || ch == 'W') && choice > 0) {
            choice--;
        } else if ((ch == KEY_DOWN || ch == 's' || ch == 'S') && choice < (int)wordCounts.size()) {
            choice++;
        } else if (ch == 10 || ch == 13) { // Enter
            if (choice < (int)wordCounts.size()) {
                return wordCounts[choice];
            } else {
                // Custom option selected - get custom input
                int customWords = getCustomWordCount();
                if (customWords > 0) {
                    return customWords;
                } else if (customWords == -1) {
                    return -1; // User cancelled
                }
                // If customWords == 0, stay in menu
            }
        } else if (ch == 'q' || ch == 'Q') { // Q - go back to main menu
            return -1;
        }
    }
}

// Function to show text options selection menu
void showTextOptionsMenu(bool& includePunctuation, bool& includeNumbers) {
    int max_x, max_y;
    int choice = 0;
    int ch;
    includePunctuation = false;
    includeNumbers = false;
    
    while (true) {
        getmaxyx(stdscr, max_y, max_x);
        clear();
        
        // Calculate box dimensions
        int box_width = 38;
        int box_height = 10;  // Title + 2 options + padding
        int box_start_x = (max_x - box_width) / 2;
        int box_start_y = (max_y - box_height) / 2;
        
        // Draw retro box border
        // Top border
        mvaddch(box_start_y, box_start_x, ACS_ULCORNER);
        for (int x = box_start_x + 1; x < box_start_x + box_width - 1; x++) {
            mvaddch(box_start_y, x, ACS_HLINE);
        }
        mvaddch(box_start_y, box_start_x + box_width - 1, ACS_URCORNER);
        
        // Side borders
        for (int y = box_start_y + 1; y < box_start_y + box_height - 1; y++) {
            mvaddch(y, box_start_x, ACS_VLINE);
            mvaddch(y, box_start_x + box_width - 1, ACS_VLINE);
        }
        
        // Bottom border
        mvaddch(box_start_y + box_height - 1, box_start_x, ACS_LLCORNER);
        for (int x = box_start_x + 1; x < box_start_x + box_width - 1; x++) {
            mvaddch(box_start_y + box_height - 1, x, ACS_HLINE);
        }
        mvaddch(box_start_y + box_height - 1, box_start_x + box_width - 1, ACS_LRCORNER);
        
        // Title inside box
        std::string title = "TEXT OPTIONS";
        mvprintw(box_start_y + 2, box_start_x + (box_width - title.length()) / 2, "%s", title.c_str());
        
        // Draw separator line
        for (int x = box_start_x + 2; x < box_start_x + box_width - 2; x++) {
            mvaddch(box_start_y + 3, x, ACS_HLINE);
        }
        
        // Show options with checkboxes inside box
        int startY = box_start_y + 5;
        std::string punctOption = std::string(includePunctuation ? "[X]" : "[ ]") + " Punctuation";
        std::string numbersOption = std::string(includeNumbers ? "[X]" : "[ ]") + " Numbers";
        
        // Highlight selected option
        if (choice == 0) {
            mvprintw(startY, box_start_x + 3, ">");
            mvprintw(startY, box_start_x + 5, "%s", punctOption.c_str());
        } else {
            mvprintw(startY, box_start_x + 5, "%s", punctOption.c_str());
        }
        
        if (choice == 1) {
            mvprintw(startY + 1, box_start_x + 3, ">");
            mvprintw(startY + 1, box_start_x + 5, "%s", numbersOption.c_str());
        } else {
            mvprintw(startY + 1, box_start_x + 5, "%s", numbersOption.c_str());
        }
        
        refresh();
        
        // Instructions at bottom of terminal (outside box)
        std::string instructions = "WASD/Arrows: Navigate | Space: Toggle | Enter: Continue | Q: Back";
        mvprintw(max_y - 2, (max_x - instructions.length()) / 2, "%s", instructions.c_str());
        
        // Add worm closet instruction
        std::string wormInstruction = "Press W for Worm Closet";
        mvprintw(max_y - 1, (max_x - wormInstruction.length()) / 2, "%s", wormInstruction.c_str());
        
        ch = getch();
        if ((ch == KEY_UP || ch == 'w' || ch == 'W') && choice > 0) {
            choice--;
        } else if ((ch == KEY_DOWN || ch == 's' || ch == 'S') && choice < 1) {
            choice++;
        } else if (ch == ' ' || ch == 32) { // Space key to toggle
            if (choice == 0) {
                includePunctuation = !includePunctuation;
            } else if (choice == 1) {
                includeNumbers = !includeNumbers;
            }
        } else if (ch == 10 || ch == 13) { // Enter
            break;
        } else if (ch == 'q' || ch == 'Q') { // Q - use defaults (no punctuation, no numbers)
            includePunctuation = false;
            includeNumbers = false;
            break;
        }
    }
}

// Function to show worm closet (achievements/customization)
bool showWormCloset() {
    int max_x, max_y;
    int choice = 0;
    int ch;
    int worm_frame = 0;  // Animation frame counter for decorative worms
    
    while (true) {
        getmaxyx(stdscr, max_y, max_x);
        clear();
        
        // Calculate box dimensions
        int box_width = 50;
        int box_height = 16;
        int box_start_x = (max_x - box_width) / 2;
        int box_start_y = (max_y - box_height) / 2;
        
        // Draw retro box border
        // Top border
        mvaddch(box_start_y, box_start_x, ACS_ULCORNER);
        for (int x = box_start_x + 1; x < box_start_x + box_width - 1; x++) {
            mvaddch(box_start_y, x, ACS_HLINE);
        }
        mvaddch(box_start_y, box_start_x + box_width - 1, ACS_URCORNER);
        
        // Side borders
        for (int y = box_start_y + 1; y < box_start_y + box_height - 1; y++) {
            mvaddch(y, box_start_x, ACS_VLINE);
            mvaddch(y, box_start_x + box_width - 1, ACS_VLINE);
        }
        
        // Bottom border
        mvaddch(box_start_y + box_height - 1, box_start_x, ACS_LLCORNER);
        for (int x = box_start_x + 1; x < box_start_x + box_width - 1; x++) {
            mvaddch(box_start_y + box_height - 1, x, ACS_HLINE);
        }
        mvaddch(box_start_y + box_height - 1, box_start_x + box_width - 1, ACS_LRCORNER);
        
        // Title inside box
        std::string title = "WORM CLOSET";
        mvprintw(box_start_y + 2, box_start_x + (box_width - title.length()) / 2, "%s", title.c_str());
        
        // Draw separator line
        for (int x = box_start_x + 2; x < box_start_x + box_width - 2; x++) {
            mvaddch(box_start_y + 3, x, ACS_HLINE);
        }
        
        // Show achievement grid (3x3 for future expansion)
        int startY = box_start_y + 5;
        int grid_width = 3 * 10 - 4;  // 3 slots * 10 spacing - 4 for last slot width
        int startX = box_start_x + (box_width - grid_width) / 2;
        
        // Draw achievement slots as ASCII squares
        for (int row = 0; row < 3; row++) {
            for (int col = 0; col < 3; col++) {
                int slot_y = startY + row * 3;
                int slot_x = startX + col * 10;
                
                bool isSelected = (choice == row * 3 + col);
                bool isUnlocked = false;
                std::string slotContent = "[ ]";
                
                // Check if this is the pink worm achievement (slot 0)
                if (row == 0 && col == 0) {
                    for (size_t i = 0; i < achievements.size(); i++) {
                        if (achievements[i].id == "pink_worm" && achievements[i].unlocked) {
                            isUnlocked = true;
                            if (equippedWormColor == "pink") {
                                slotContent = "[*]";  // Equipped
                            } else {
                                slotContent = "[P]";  // Pink available
                            }
                            break;
                        }
                    }
                    if (!isUnlocked) {
                        slotContent = "[?]";  // Locked
                    }
                } else if (row == 0 && col == 1) {
                    // Default worm (always available)
                    isUnlocked = true;
                    if (equippedWormColor == "default") {
                        slotContent = "[*]";  // Equipped
                    } else {
                        slotContent = "[D]";  // Default available
                    }
                } else {
                    slotContent = "[ ]";  // Empty slot for future achievements
                }
                
                // Draw selection indicator
                if (isSelected) {
                    mvprintw(slot_y, slot_x, "> ");
                    // Apply pink color if this is the pink worm slot and it's unlocked
                    if (row == 0 && col == 0 && isUnlocked && has_colors()) {
                        attron(COLOR_PAIR(4));  // Pink color
                        mvprintw(slot_y, slot_x + 2, "%s", slotContent.c_str());
                        attroff(COLOR_PAIR(4));
                    } else {
                        mvprintw(slot_y, slot_x + 2, "%s", slotContent.c_str());
                    }
                    mvprintw(slot_y, slot_x + 5, " <");
                } else {
                    // Apply pink color if this is the pink worm slot and it's unlocked
                    if (row == 0 && col == 0 && isUnlocked && has_colors()) {
                        attron(COLOR_PAIR(4));  // Pink color
                        mvprintw(slot_y, slot_x + 2, "%s", slotContent.c_str());
                        attroff(COLOR_PAIR(4));
                    } else {
                        mvprintw(slot_y, slot_x + 2, "%s", slotContent.c_str());
                    }
                }
            }
        }
        
        // Show current selection info
        std::string info = "";
        if (choice == 0) {
            bool pinkUnlocked = false;
            for (size_t i = 0; i < achievements.size(); i++) {
                if (achievements[i].id == "pink_worm" && achievements[i].unlocked) {
                    pinkUnlocked = true;
                    break;
                }
            }
            info = pinkUnlocked ? "Pink Worm - Unlocked at 60+ WPM" : "??? - Achieve 60+ WPM to unlock";
        } else if (choice == 1) {
            info = "Default Worm - Classic orange-red";
        } else {
            info = "Empty Slot - Future achievement";
        }
        
        mvprintw(box_start_y + box_height - 4, box_start_x + 3, "%-44s", info.c_str());
        
        // Draw decorative worms around the closet box
        int worm_width = box_width - 4;
        
        // Top decorative worm (green, moving left to right)
        drawDecorativeWorm(box_start_y - 1, box_start_x + 2, worm_width, 6, false, worm_frame);
        
        // Bottom decorative worm (blue, moving right to left)  
        drawDecorativeWorm(box_start_y + box_height, box_start_x + 2, worm_width, 7, true, worm_frame + 50);
        
        // Side decorative worms (smaller)
        if (max_x > 80) { // Only show side worms if screen is wide enough
            // Left side worm (purple, moving down)
            int left_worm_x = box_start_x - 10;
            if (left_worm_x > 5) {
                for (int i = 0; i < 3; i++) {
                    int worm_y = box_start_y + 3 + i * 4;
                    if (worm_y < box_start_y + box_height - 2) {
                        drawDecorativeWorm(worm_y, left_worm_x, 8, 8, (worm_frame + i * 30) % 400 > 200, worm_frame + i * 30);
                    }
                }
            }
            
            // Right side worm (pink, moving up)
            int right_worm_x = box_start_x + box_width + 2;
            if (right_worm_x < max_x - 10) {
                for (int i = 0; i < 3; i++) {
                    int worm_y = box_start_y + box_height - 5 - i * 4;
                    if (worm_y > box_start_y + 2) {
                        drawDecorativeWorm(worm_y, right_worm_x, 8, 4, (worm_frame + i * 40) % 300 > 150, worm_frame + i * 40);
                    }
                }
            }
        }
        
        worm_frame++;  // Advance animation
        
        refresh();
        
        // Instructions at bottom of terminal
        std::string instructions = "WASD/Arrows: Navigate | Enter: Equip | Q: Back";
        mvprintw(max_y - 1, (max_x - instructions.length()) / 2, "%s", instructions.c_str());
        
        // Use timeout for smooth animation
        timeout(100);  // 100ms timeout
        ch = getch();
        timeout(-1);   // Reset to blocking mode
        
        if (ch == ERR) {
            // No key pressed, continue animation
            continue;
        }
        if ((ch == KEY_UP || ch == 'w' || ch == 'W') && choice >= 3) {
            choice -= 3;
        } else if ((ch == KEY_DOWN || ch == 's' || ch == 'S') && choice < 6) {
            choice += 3;
        } else if ((ch == KEY_LEFT || ch == 'a' || ch == 'A') && choice % 3 > 0) {
            choice--;
        } else if ((ch == KEY_RIGHT || ch == 'd' || ch == 'D') && choice % 3 < 2) {
            choice++;
        } else if (ch == 10 || ch == 13) { // Enter - equip worm
            if (choice == 0) {
                // Pink worm
                for (size_t i = 0; i < achievements.size(); i++) {
                    if (achievements[i].id == "pink_worm" && achievements[i].unlocked) {
                        equippedWormColor = "pink";
                        saveAchievements();
                        break;
                    }
                }
            } else if (choice == 1) {
                // Default worm
                equippedWormColor = "default";
                saveAchievements();
            }
            // Other slots don't have worms yet
        } else if (ch == 'q' || ch == 'Q') { // Q
            return false;
        }
    }
}

// Achievement system functions
void initializeAchievements() {
    achievements.clear();
    achievements.push_back(Achievement("pink_worm", "red!worm?pink!worm?", "Achieve 60+ WPM to unlock the pink worm variant!"));
}

void saveAchievements() {
    std::ofstream file("achievements.txt");
    if (file.is_open()) {
        file << equippedWormColor << std::endl;
        for (size_t i = 0; i < achievements.size(); i++) {
            file << achievements[i].id << "|" << (achievements[i].unlocked ? 1 : 0) << std::endl;
        }
        file.close();
    }
}

void loadAchievements() {
    std::ifstream file("achievements.txt");
    if (file.is_open()) {
        std::string line;
        if (std::getline(file, line)) {
            equippedWormColor = line;
        }
        
        while (std::getline(file, line)) {
            size_t pos = line.find('|');
            if (pos != std::string::npos) {
                std::string id = line.substr(0, pos);
                bool unlocked = (std::stoi(line.substr(pos + 1)) == 1);
                
                for (size_t i = 0; i < achievements.size(); i++) {
                    if (achievements[i].id == id) {
                        achievements[i].unlocked = unlocked;
                        break;
                    }
                }
            }
        }
        file.close();
    }
}

void checkAchievements(double wpm, double accuracy, double time) {
    bool newAchievement = false;
    
    // Check for pink worm achievement (60+ WPM)
    for (size_t i = 0; i < achievements.size(); i++) {
        if (achievements[i].id == "pink_worm" && !achievements[i].unlocked && wpm >= 60.0) {
            achievements[i].unlocked = true;
            newAchievement = true;
            
            // Show achievement notification
            int max_x, max_y;
            getmaxyx(stdscr, max_y, max_x);
            clear();
            
            std::string congrats = "ACHIEVEMENT UNLOCKED!";
            std::string achieveName = achievements[i].name;
            std::string description = "Pink worm variant unlocked!";
            std::string instruction = "Press any key to continue...";
            
            mvprintw(max_y/2 - 2, (max_x - congrats.length())/2, "%s", congrats.c_str());
            mvprintw(max_y/2, (max_x - achieveName.length())/2, "%s", achieveName.c_str());
            mvprintw(max_y/2 + 1, (max_x - description.length())/2, "%s", description.c_str());
            mvprintw(max_y/2 + 3, (max_x - instruction.length())/2, "%s", instruction.c_str());
            
            refresh();
            getch();
            break;
        }
    }
    
    if (newAchievement) {
        saveAchievements();
    }
}

// Function to get player name (with selection option)
std::string getPlayerName(const std::vector<PlayerScore>& leaderboard) {
    std::vector<std::string> uniqueNames = getUniquePlayerNames(leaderboard);
    
    // If no previous names, go directly to new name input
    if (uniqueNames.empty()) {
        return getNewPlayerName();
    }
    
    // Show selection menu
    std::string selectedName = showNameSelectionMenu(uniqueNames);
    
    if (selectedName == "CANCEL") {
        return "CANCEL";  // User pressed ESC
    } else if (selectedName == "") {
        // User chose "Enter new name"
        return getNewPlayerName();
    } else {
        // User selected an existing name
        return selectedName;
    }
}

// Function to load leaderboard from file
std::vector<PlayerScore> loadLeaderboard() {
    std::vector<PlayerScore> leaderboard;
    std::ifstream file("leaderboard.txt");
    
    if (file.is_open()) {
        std::string line;
        while (std::getline(file, line)) {
            size_t pos1 = line.find('|');
            size_t pos2 = line.find('|', pos1 + 1);
            size_t pos3 = line.find('|', pos2 + 1);
            size_t pos4 = line.find('|', pos3 + 1);
            size_t pos5 = line.find('|', pos4 + 1);
            size_t pos6 = line.find('|', pos5 + 1);
            size_t pos7 = line.find('|', pos6 + 1);
            
            if (pos1 != std::string::npos && pos2 != std::string::npos && pos3 != std::string::npos) {
                std::string name = toUpperCase(line.substr(0, pos1));
                double wpm = std::stod(line.substr(pos1 + 1, pos2 - pos1 - 1));
                double accuracy = std::stod(line.substr(pos2 + 1, pos3 - pos2 - 1));
                
                std::string date = "";
                int wordCount = 15;  // Default for backward compatibility
                bool hasPunctuation = false;  // Default for backward compatibility
                bool hasNumbers = false;  // Default for backward compatibility
                double time = 0.0;
                
                if (pos4 != std::string::npos) {
                    time = std::stod(line.substr(pos3 + 1, pos4 - pos3 - 1));
                } else {
                    // Very old format with only 3 fields (name|wpm|accuracy)
                    time = std::stod(line.substr(pos3 + 1));
                    date = "Unknown";
                    leaderboard.push_back(PlayerScore(name, wpm, accuracy, time, date, wordCount, hasPunctuation, hasNumbers));
                    continue;
                }
                
                if (pos7 != std::string::npos) {
                    // Newest format with all fields
                    try {
                        date = line.substr(pos4 + 1, pos5 - pos4 - 1);
                        wordCount = std::stoi(line.substr(pos5 + 1, pos6 - pos5 - 1));
                        hasPunctuation = (std::stoi(line.substr(pos6 + 1, pos7 - pos6 - 1)) == 1);
                        hasNumbers = (std::stoi(line.substr(pos7 + 1)) == 1);
                    } catch (const std::exception&) {
                        // Fall back to defaults if parsing fails
                        date = "Unknown";
                        wordCount = 15;
                        hasPunctuation = false;
                        hasNumbers = false;
                    }
                } else if (pos5 != std::string::npos) {
                    // Format with word count but no punctuation/numbers
                    try {
                        date = line.substr(pos4 + 1, pos5 - pos4 - 1);
                        wordCount = std::stoi(line.substr(pos5 + 1));
                    } catch (const std::exception&) {
                        // Fall back to defaults if parsing fails
                        date = "Unknown";
                        wordCount = 15;
                    }
                } else {
                    // Old format without word count
                    date = line.substr(pos4 + 1);
                    if (date.empty()) {
                        date = "Unknown";
                    }
                }
                
                leaderboard.push_back(PlayerScore(name, wpm, accuracy, time, date, wordCount, hasPunctuation, hasNumbers));
            }
        }
        file.close();
    }
    
    return leaderboard;
}

// Function to save leaderboard to file
void saveLeaderboard(const std::vector<PlayerScore>& leaderboard) {
    std::ofstream file("leaderboard.txt");
    
    if (file.is_open()) {
        for (size_t i = 0; i < leaderboard.size(); i++) {
            file << leaderboard[i].name << "|" << leaderboard[i].wpm << "|" << leaderboard[i].accuracy << "|" << leaderboard[i].time << "|" << leaderboard[i].date << "|" << leaderboard[i].wordCount << "|" << (leaderboard[i].hasPunctuation ? 1 : 0) << "|" << (leaderboard[i].hasNumbers ? 1 : 0) << std::endl;
        }
        file.close();
    }
}

// Comparison function for sorting scores
bool compareScores(const PlayerScore& a, const PlayerScore& b) {
    if (a.wpm != b.wpm) return a.wpm > b.wpm;
    return a.accuracy > b.accuracy;
}

// Function to add score to leaderboard and maintain top 10
void addToLeaderboard(std::vector<PlayerScore>& leaderboard, const PlayerScore& newScore) {
    leaderboard.push_back(newScore);
    
    // Sort by WPM (descending), then by accuracy (descending) if WPM is same
    std::sort(leaderboard.begin(), leaderboard.end(), compareScores);
    
    // Keep only top 10
    if (leaderboard.size() > 10) {
        leaderboard.resize(10);
    }
}

// Function to display leaderboard with clear, change name, and worm closet options
// Returns: 0 = continue, 1 = cleared leaderboard, 2 = change name requested, 3 = worm closet requested
int showLeaderboard(std::vector<PlayerScore>& leaderboard) {
    int max_x, max_y;
    int ch;
    int worm_frame = 0;         // Animation frame for the worm
    double worm_position = 0.0; // Worm position (0.0 to 1.0)
    
    while (true) {
        getmaxyx(stdscr, max_y, max_x);
        clear();
        
        std::string title = "=== TOP 10 LEADERBOARD ===";
        mvprintw(2, (max_x - title.length())/2, "%s", title.c_str());
        
        // Draw animated worm under the title
        int worm_y = 3;
        int worm_start_x = 2;
        int worm_width = max_x - 4;
        drawBouncyWorm(worm_y, worm_start_x, worm_width, worm_position, worm_frame);
        
        // Update worm animation
        worm_position += 0.02;  // Move worm forward
        if (worm_position >= 1.0) worm_position = 0.0;  // Loop back
        worm_frame++;
        
        // Make leaderboard responsive to terminal width
        int display_width = max_x - 4;
        if (display_width < 95) {
            // Compact format for narrow terminals - center the content
            int compact_width = 50;  // Total width of compact leaderboard
            int compact_start_x = (max_x - compact_width) / 2;
            
            mvprintw(4, compact_start_x, "# Name         WPM   Acc%%  Time  Words  Mode");
            mvprintw(5, compact_start_x, "- ----------- ----  ----  ----  -----  ----");
            
            for (size_t i = 0; i < leaderboard.size() && i < 10; i++) {
                std::string nameDisplay = toUpperCase(leaderboard[i].name);
                if (nameDisplay.length() > 11) {
                    nameDisplay = nameDisplay.substr(0, 8) + "...";
                }
                
                // Create mode indicator (P for punctuation, N for numbers)
                std::string mode = "";
                if (leaderboard[i].hasPunctuation) mode += "P";
                if (leaderboard[i].hasNumbers) mode += "N";
                if (mode.empty()) mode = "-";
                
                mvprintw(6 + i, compact_start_x, "%2zu %-11s %4.0f  %3.0f%%  %3.0fs  %3dw   %-2s", 
                         i + 1, nameDisplay.c_str(), leaderboard[i].wpm, 
                         leaderboard[i].accuracy, leaderboard[i].time, leaderboard[i].wordCount, mode.c_str());
            }
        } else {
            // Full format for wider terminals
            int start_x = (max_x - 105)/2;
            mvprintw(4, start_x, "Rank  Name            WPM    Accuracy  Time   Words  Mode  Date & Time");
            mvprintw(5, start_x, "----  --------------  -----  --------  ----   -----  ----  ----------------");
            
            for (size_t i = 0; i < leaderboard.size() && i < 10; i++) {
                std::string nameDisplay = toUpperCase(leaderboard[i].name);
                if (nameDisplay.length() > 14) {
                    nameDisplay = nameDisplay.substr(0, 11) + "...";
                }
                
                // Create mode indicator (P for punctuation, N for numbers)
                std::string mode = "";
                if (leaderboard[i].hasPunctuation) mode += "P";
                if (leaderboard[i].hasNumbers) mode += "N";
                if (mode.empty()) mode = "-";
                
                mvprintw(6 + i, start_x, "%4zu  %-14s  %5.1f  %7.1f%%  %4.0fs  %3dw   %-4s  %s", 
                         i + 1, nameDisplay.c_str(), leaderboard[i].wpm, 
                         leaderboard[i].accuracy, leaderboard[i].time, leaderboard[i].wordCount, mode.c_str(), leaderboard[i].date.c_str());
            }
        }
        
        if (leaderboard.empty()) {
            mvprintw(8, (max_x - 25)/2, "No scores recorded yet!");
        }
        
        // Center all instruction strings properly
        std::string clearStr = "Press 'C' to clear leaderboard";
        std::string nameStr = "Press 'N' to change player name";
        std::string wormStr = "Press 'W' to open worm closet";
        std::string continueStr = "Press any other key to continue";
        
        mvprintw(max_y - 8, (max_x - clearStr.length())/2, "%s", clearStr.c_str());
        mvprintw(max_y - 7, (max_x - nameStr.length())/2, "%s", nameStr.c_str());
        mvprintw(max_y - 6, (max_x - wormStr.length())/2, "%s", wormStr.c_str());
        mvprintw(max_y - 5, (max_x - continueStr.length())/2, "%s", continueStr.c_str());
        
        // Add worm closet instruction
        std::string wormInstruction = "Press W for Worm Closet";
        mvprintw(max_y - 3, (max_x - wormInstruction.length()) / 2, "%s", wormInstruction.c_str());
        refresh();
        
        // Use timeout to allow animation to continue
        timeout(100);  // 100ms timeout for getch
        ch = getch();
        timeout(-1);   // Reset to blocking mode
        
        if (ch == ERR) {
            // No key pressed, continue animation loop
            continue;
        } else if (ch == 'c' || ch == 'C') {
            // Confirm clear action
            clear();
            mvprintw(max_y/2 - 1, (max_x - 35)/2, "Clear all leaderboard data?");
            mvprintw(max_y/2, (max_x - 25)/2, "Press 'Y' to confirm");
            mvprintw(max_y/2 + 1, (max_x - 30)/2, "Press any other key to cancel");
            refresh();
            
            int confirm = getch();
            if (confirm == 'y' || confirm == 'Y') {
                leaderboard.clear();
                saveLeaderboard(leaderboard);
                return 1; // Leaderboard was cleared
            }
            // If not confirmed, continue the loop to show leaderboard again
        } else if (ch == 'n' || ch == 'N') {
            return 2; // Change name requested
        } else if (ch == 'w' || ch == 'W') {
            return 3; // Worm closet requested
        } else {
            return 0; // No changes made
        }
    }
}

// Function to show animated intro title
void showAnimatedIntro() {
    int max_x, max_y;
    getmaxyx(stdscr, max_y, max_x);
    
    // Split title into words
    std::vector<std::string> title_words;
    title_words.push_back("W4RMUP");
    title_words.push_back("W0RM'S");
    title_words.push_back("T3RMINAL");
    title_words.push_back("TYP3R");
    
    // Calculate full title length for centering
    std::string full_title = "W4RMUP W0RM'S T3RMINAL TYP3R";
    int title_start_x = (max_x - full_title.length()) / 2;
    int title_y = max_y / 2;
    
    clear();
    
    // Show skip instruction
    std::string skip_msg = "Press 's' to skip intro";
    mvprintw(1, (max_x - skip_msg.length()) / 2, "%s", skip_msg.c_str());
    refresh();
    
    // Set non-blocking input to check for skip key
    nodelay(stdscr, TRUE);
    
    // Worm animation variables
    double worm_position = 0.0;
    int worm_frame = 0;
    int worm_y = title_y + 2;  // Position worm below the title
    int worm_start_x = 2;
    int worm_width = max_x - 4;
    
    // Display words one by one with animation and smooth worm movement
    int total_frames = 20;  // 20 frames for slower, smoother worm movement
    bool skip_intro = false;
    
    for (int frame = 0; frame < total_frames && !skip_intro; frame++) {
        // Calculate smooth worm movement
        worm_position = (double)frame / (total_frames - 1);  // Move from 0.0 to 1.0
        
        // Determine which words should be visible based on frame progress
        double word_progress = (double)frame / (total_frames - 1) * title_words.size();
        int words_to_show = (int)word_progress + 1;  // At least 1 word, up to all 4
        if (words_to_show > (int)title_words.size()) words_to_show = title_words.size();
        
        // Clear and redraw everything for each frame
        clear();
        
        // Redraw skip instruction
        mvprintw(1, (max_x - skip_msg.length()) / 2, "%s", skip_msg.c_str());
        
        // Draw words up to current progress
        int temp_x = title_start_x;
        int cursor_x = temp_x; // Track cursor position
        for (int j = 0; j < words_to_show; j++) {
            if (has_colors()) {
                attron(COLOR_PAIR(5));  // Use orange-red color for title
            }
            
            mvprintw(title_y, temp_x, "%s", title_words[j].c_str());
            
            if (has_colors()) {
                attroff(COLOR_PAIR(5));
            }
            
            cursor_x = temp_x + title_words[j].length(); // Update cursor position
            temp_x += title_words[j].length() + 1;  // Move position for next word (+ space)
        }
        
        // Draw the animated worm first
        drawBouncyWorm(worm_y, worm_start_x, worm_width, worm_position, worm_frame);
        
        // Position cursor at end of last word drawn during animation (after worm drawing)
        curs_set(1);
        move(title_y, cursor_x);
        
        refresh();
        
        // Check for skip key press
        int ch = getch();
        if (ch == 's' || ch == 'S') {
            skip_intro = true;
            break;
        }
        
        usleep(150000);  // 150ms delay between frames (total: 3 seconds for 20 frames)
        worm_frame++;  // Advance worm animation frame by frame
    }
    
    // Restore blocking input
    nodelay(stdscr, FALSE);
    
    // Clear the screen and redraw only the title without the worm
    clear();
    int temp_x = title_start_x;
    for (size_t j = 0; j < title_words.size(); j++) {
        if (has_colors()) {
            attron(COLOR_PAIR(5));  // Use orange-red color for title
        }
        
        mvprintw(title_y, temp_x, "%s", title_words[j].c_str());
        
        if (has_colors()) {
            attroff(COLOR_PAIR(5));
        }
        
        temp_x += title_words[j].length() + 1;  // Move position for next word (+ space)
    }
    
    // Hide cursor after title animation is complete
    curs_set(0);
    refresh();
    
    // Hold the complete title for a moment (skip if intro was skipped)
    if (!skip_intro) {
        usleep(500000);  // 500ms final pause (reduced since we're adding more animation)
    }
    
    // Show prompt to continue (animate if not skipped)
    std::string prompt = "Press any key to continue...";
    int prompt_y = title_y + 3;
    int prompt_start_x = (max_x - prompt.length()) / 2;
    
    if (skip_intro) {
        // Show prompt immediately if intro was skipped
        mvprintw(prompt_y, prompt_start_x, "%s", prompt.c_str());
        refresh();
    } else {
        // Animate prompt letter by letter
        for (size_t i = 0; i < prompt.length(); i++) {
            mvaddch(prompt_y, prompt_start_x + i, prompt[i]);
            refresh();
            usleep(50000);  // 50ms delay between letters (fast typing effect)
        }
    }
    
    // Wait for any key press
    getch();
}

// Function to draw a decorative worm for closet/inventory screens
void drawDecorativeWorm(int y, int start_x, int width, int color_pair, bool reverse_direction, int frame) {
    if (width <= 0) return;
    
    // Calculate animation position (moves across the width)
    double progress = (frame % 200) / 200.0; // Complete cycle in 200 frames
    int head_x;
    
    if (reverse_direction) {
        head_x = start_x + width - 1 - (int)(progress * (width - 1));
    } else {
        head_x = start_x + (int)(progress * (width - 1));
    }
    
    // Worm head animation frames (bouncy effect)
    char head_chars[] = {'O', 'o', 'O', '0'};
    char head_char = head_chars[frame % 4];
    
    // Apply color
    if (has_colors()) {
        attron(COLOR_PAIR(color_pair));
    }
    
    // Draw the worm head
    mvaddch(y, head_x, head_char);
    
    // Draw trailing body segments
    int worm_length = 6;
    for (int i = 1; i <= worm_length; i++) {
        int body_x = reverse_direction ? head_x + i : head_x - i;
        if (body_x >= start_x && body_x < start_x + width) {
            char body_char;
            if (i == 1) {
                body_char = 'o';
            } else if (i <= 3) {
                body_char = '.';
            } else {
                body_char = (frame % 2 == 0) ? ':' : '.';
            }
            mvaddch(y, body_x, body_char);
        }
    }
    
    if (has_colors()) {
        attroff(COLOR_PAIR(color_pair));
    }
}

// Function to draw bouncy worm animation with color support
void drawBouncyWorm(int y, int start_x, int width, double position, int frame) {
    if (width <= 0) return;
    
    // Calculate head position across the available width
    int head_x = start_x + (int)(position * (width - 1));
    
    // Worm head animation frames (bouncy effect)
    char head_chars[] = {'O', 'o', 'O', '0'};
    char head_char = head_chars[frame % 4];
    
    // Determine worm color based on equipped variant
    int worm_color = 0;  // Default white
    if (has_colors()) {
        if (equippedWormColor == "pink") {
            worm_color = COLOR_PAIR(4);  // Pink color pair
        } else {
            worm_color = COLOR_PAIR(5);  // Default orange-red worm
        }
        attron(worm_color);
    }
    
    // Draw the worm head
    mvaddch(y, head_x, head_char);
    
    // Draw trailing body segments (bouncy worm effect)
    int worm_length = 8;  // Length of the worm trail
    for (int i = 1; i <= worm_length && head_x - i >= start_x; i++) {
        char body_char;
        
        // Create bouncy effect with different characters
        if (i == 1) {
            body_char = 'o';  // Close to head
        } else if (i <= 3) {
            body_char = '.';  // Medium distance
        } else if (i <= 5) {
            body_char = (frame % 2 == 0) ? '.' : ':';  // Animated middle
        } else {
            body_char = (frame % 3 == 0) ? ':' : '.';  // Tail with slight animation
        }
        
        mvaddch(y, head_x - i, body_char);
    }
    
    if (has_colors()) {
        attroff(worm_color);
    }
}




// Cleanup function
void cleanup() {
    endwin();
}

// Signal handler for clean exit
void signalHandler(int signum) {
    cleanup();
    exit(signum);
}

int main(int argc, char* argv[]) {
    // Set up signal handling and exit cleanup
    signal(SIGINT, signalHandler);   // Handle Ctrl+C
    signal(SIGTERM, signalHandler);  // Handle termination
    atexit(cleanup);                 // Handle normal exit
    
    
    // Initialize ncurses
    initscr();                    // Start ncurses mode
    cbreak();                     // Get chars immediately without Enter
    noecho();                     // Don't auto-display typed chars
    keypad(stdscr, TRUE);         // Enable special keys (arrows, F-keys)
    curs_set(0);                  // Hide cursor initially
    
    // Initialize colors
    if (has_colors()) {           // Check if terminal supports colors
        start_color();            // Enable color functionality
	assume_default_colors(-1,-1);
        
        // Try to use custom RGB color if supported
        if (can_change_color()) {
            // Define custom yellow color for correct chars (RGB: 255, 255, 0)
            // RGB values need to be scaled to 0-1000 for ncurses
            init_color(9, 1000, 1000, 0);  // 255*1000/255, 255*1000/255, 0*1000/255
            init_pair(9, COLOR_CYAN, -1);  // Pair 1: correct chars (yellow) with transparent background
            
            // Define custom pink color for pink worm achievement (RGB: 255, 20, 147)
            init_color(10, 1000, 78, 576);  // 255*1000/255, 20*1000/255, 147*1000/255
            init_pair(4, 10, -1);  // Pair 4: pink worm with transparent background
            
            // Define custom orange-red color for default worm (RGB: 245, 73, 39)
            init_color(11, 958, 286, 153);  // 245*1000/255, 73*1000/255, 39*1000/255
            init_pair(5, 11, -1);  // Pair 5: default worm with transparent background
            
            // Define decorative worm colors for closet
            init_color(12, 0, 1000, 0);      // Bright green (RGB: 0, 255, 0)
            init_pair(6, 12, -1);  // Pair 6: green worm
            
            init_color(13, 0, 500, 1000);    // Blue (RGB: 0, 128, 255)
            init_pair(7, 13, -1);  // Pair 7: blue worm
            
            init_color(14, 800, 0, 800);     // Purple (RGB: 204, 0, 204)
            init_pair(8, 14, -1);  // Pair 8: purple worm
        } else {
            // Fallback to closest standard color (yellow)
            init_pair(1, COLOR_BLUE, -1);   // Pair 1: correct chars (yellow fallback) with transparent background
            init_pair(4, COLOR_MAGENTA, -1);  // Pair 4: pink worm fallback with transparent background
            init_pair(5, COLOR_RED, -1);      // Pair 5: default worm fallback with transparent background
            init_pair(6, COLOR_GREEN, -1);    // Pair 6: green worm fallback
            init_pair(7, COLOR_BLUE, -1);     // Pair 7: blue worm fallback
            init_pair(8, COLOR_MAGENTA, -1);  // Pair 8: purple worm fallback
        }
        
        init_pair(2, COLOR_RED, -1);     // Pair 2: wrong chars (red) with transparent background
        init_pair(3, COLOR_MAGENTA, -1);   // Pair 3: untyped chars (white) with transparent background
    }
    
    // Load leaderboard
    std::vector<PlayerScore> leaderboard = loadLeaderboard();
    
    // Initialize and load achievements
    initializeAchievements();
    loadAchievements();
    
    // Show animated intro
    showAnimatedIntro();
    
    // Get player name once at startup
    std::string playerName = getPlayerName(leaderboard);
    
    // If user cancelled name selection, exit program
    if (playerName == "CANCEL") {
        cleanup();
        return 0;
    }
    
    // If user selected worm closet first, show it then get name again
    while (playerName == "WORM_CLOSET") {
        showWormCloset();
        playerName = getPlayerName(leaderboard);
        if (playerName == "CANCEL") {
            cleanup();
            return 0;
        }
    }
    
    // Main game loop
    while (true) {
        
        // Get word count selection
        int wordCount = showWordCountMenu();
        
        // If user cancelled word count selection, exit program
        if (wordCount == -1) {
            break;
        }
        
        // Get text options selection
        bool includePunctuation, includeNumbers;
        showTextOptionsMenu(includePunctuation, includeNumbers);
        
        // Word list for random text generation
        std::vector<std::string> words;
        words.push_back("the"); words.push_back("quick"); words.push_back("brown"); words.push_back("fox");
        words.push_back("jumps"); words.push_back("over"); words.push_back("lazy"); words.push_back("dog");
        words.push_back("hello"); words.push_back("world"); words.push_back("typing"); words.push_back("test");
        words.push_back("program"); words.push_back("simple"); words.push_back("fast"); words.push_back("computer");
        words.push_back("keyboard"); words.push_back("screen"); words.push_back("mouse"); words.push_back("software");
        words.push_back("hardware"); words.push_back("internet"); words.push_back("website"); words.push_back("email");
        words.push_back("password"); words.push_back("username"); words.push_back("login"); words.push_back("download");
        words.push_back("upload"); words.push_back("file"); words.push_back("folder"); words.push_back("document");
        words.push_back("window"); words.push_back("button"); words.push_back("click"); words.push_back("double");
        words.push_back("right"); words.push_back("left"); words.push_back("center"); words.push_back("top");
        words.push_back("bottom"); words.push_back("middle"); words.push_back("side"); words.push_back("front");
        words.push_back("back"); words.push_back("forward"); words.push_back("backward"); words.push_back("up");
        words.push_back("down"); words.push_back("north"); words.push_back("south"); words.push_back("east");
        words.push_back("west"); words.push_back("morning"); words.push_back("afternoon"); words.push_back("evening");
        words.push_back("night"); words.push_back("today"); words.push_back("tomorrow"); words.push_back("yesterday");
        words.push_back("week"); words.push_back("month"); words.push_back("year"); words.push_back("time");
        words.push_back("clock"); words.push_back("watch"); words.push_back("minute"); words.push_back("second");
        words.push_back("hour"); words.push_back("schedule"); words.push_back("appointment"); words.push_back("meeting");
        words.push_back("conference"); words.push_back("presentation"); words.push_back("project"); words.push_back("task");
        words.push_back("work"); words.push_back("job"); words.push_back("career"); words.push_back("business");
        words.push_back("company"); words.push_back("office"); words.push_back("desk"); words.push_back("chair");
        words.push_back("table"); words.push_back("phone"); words.push_back("mobile"); words.push_back("tablet");
        words.push_back("laptop"); words.push_back("desktop"); words.push_back("server"); words.push_back("network");
        words.push_back("wireless"); words.push_back("bluetooth"); words.push_back("cable"); words.push_back("connection");
        words.push_back("signal"); words.push_back("data"); words.push_back("information"); words.push_back("knowledge");
        words.push_back("learning"); words.push_back("education"); words.push_back("school"); words.push_back("university");
        words.push_back("student"); words.push_back("teacher"); words.push_back("book"); words.push_back("page");
        words.push_back("chapter"); words.push_back("paragraph"); words.push_back("sentence"); words.push_back("word");
        words.push_back("letter"); words.push_back("number"); words.push_back("count"); words.push_back("calculate");
        words.push_back("mathematics"); words.push_back("science"); words.push_back("technology"); words.push_back("innovation");
        words.push_back("development"); words.push_back("progress"); words.push_back("improvement"); words.push_back("solution");
        words.push_back("problem"); words.push_back("challenge"); words.push_back("opportunity");
        
        // Words with punctuation for punctuation mode
        std::vector<std::string> punctuationWords;
        punctuationWords.push_back("hello,"); punctuationWords.push_back("world!"); punctuationWords.push_back("it's"); punctuationWords.push_back("don't");
        punctuationWords.push_back("can't"); punctuationWords.push_back("won't"); punctuationWords.push_back("we're"); punctuationWords.push_back("they're");
        punctuationWords.push_back("you'll"); punctuationWords.push_back("I'll"); punctuationWords.push_back("she'll"); punctuationWords.push_back("he'll");
        punctuationWords.push_back("we'll"); punctuationWords.push_back("they'll"); punctuationWords.push_back("isn't"); punctuationWords.push_back("aren't");
        punctuationWords.push_back("wasn't"); punctuationWords.push_back("weren't"); punctuationWords.push_back("hasn't"); punctuationWords.push_back("haven't");
        punctuationWords.push_back("doesn't"); punctuationWords.push_back("didn't"); punctuationWords.push_back("shouldn't"); punctuationWords.push_back("wouldn't");
        punctuationWords.push_back("couldn't"); punctuationWords.push_back("mustn't"); punctuationWords.push_back("needn't"); punctuationWords.push_back("shan't");
        punctuationWords.push_back("hello."); punctuationWords.push_back("goodbye!"); punctuationWords.push_back("really?"); punctuationWords.push_back("amazing!");
        punctuationWords.push_back("yes,"); punctuationWords.push_back("no,"); punctuationWords.push_back("wait..."); punctuationWords.push_back("stop!");
        punctuationWords.push_back("go!"); punctuationWords.push_back("help!"); punctuationWords.push_back("wow!"); punctuationWords.push_back("oh!");
        
        // Words with numbers for numbers mode
        std::vector<std::string> numberWords;
        numberWords.push_back("123"); numberWords.push_back("456"); numberWords.push_back("789"); numberWords.push_back("101");
        numberWords.push_back("202"); numberWords.push_back("303"); numberWords.push_back("404"); numberWords.push_back("505");
        numberWords.push_back("2024"); numberWords.push_back("2025"); numberWords.push_back("1995"); numberWords.push_back("2000");
        numberWords.push_back("42"); numberWords.push_back("99"); numberWords.push_back("100"); numberWords.push_back("1000");
        numberWords.push_back("test1"); numberWords.push_back("test2"); numberWords.push_back("file1"); numberWords.push_back("file2");
        numberWords.push_back("user1"); numberWords.push_back("user2"); numberWords.push_back("admin123"); numberWords.push_back("pass123");
        numberWords.push_back("v1.0"); numberWords.push_back("v2.0"); numberWords.push_back("v3.1"); numberWords.push_back("v4.2");
        numberWords.push_back("room101"); numberWords.push_back("room202"); numberWords.push_back("apt3b"); numberWords.push_back("unit4a");
        numberWords.push_back("level1"); numberWords.push_back("level2"); numberWords.push_back("step1"); numberWords.push_back("step2");
        numberWords.push_back("page1"); numberWords.push_back("page2"); numberWords.push_back("item1"); numberWords.push_back("item2");
    
    srand(time(nullptr));          // Seed random number generator with current time
    
    // Generate target text based on selected options
    std::string target = "";       // Text user needs to type
    
    if (includeNumbers && !includePunctuation) {
        // Numbers only mode: only pure numbers and spaces
        std::vector<std::string> pureNumbers;
        pureNumbers.push_back("0"); pureNumbers.push_back("1"); pureNumbers.push_back("2"); pureNumbers.push_back("3");
        pureNumbers.push_back("4"); pureNumbers.push_back("5"); pureNumbers.push_back("6"); pureNumbers.push_back("7");
        pureNumbers.push_back("8"); pureNumbers.push_back("9"); pureNumbers.push_back("10"); pureNumbers.push_back("11");
        pureNumbers.push_back("12"); pureNumbers.push_back("13"); pureNumbers.push_back("14"); pureNumbers.push_back("15");
        pureNumbers.push_back("16"); pureNumbers.push_back("17"); pureNumbers.push_back("18"); pureNumbers.push_back("19");
        pureNumbers.push_back("20"); pureNumbers.push_back("25"); pureNumbers.push_back("30"); pureNumbers.push_back("42");
        pureNumbers.push_back("50"); pureNumbers.push_back("75"); pureNumbers.push_back("99"); pureNumbers.push_back("100");
        pureNumbers.push_back("123"); pureNumbers.push_back("456"); pureNumbers.push_back("789"); pureNumbers.push_back("1000");
        pureNumbers.push_back("2024"); pureNumbers.push_back("2025"); pureNumbers.push_back("3000"); pureNumbers.push_back("5000");
        
        for (int i = 0; i < wordCount; i++) {
            if (i > 0) target += " ";  // Add space between numbers
            target += pureNumbers[rand() % pureNumbers.size()];
        }
    } else {
        // Create combined word pool based on selected options
        std::vector<std::string> combinedWords = words;  // Start with base words
        
        if (includePunctuation) {
            // Add punctuation words to the pool
            for (size_t i = 0; i < punctuationWords.size(); i++) {
                combinedWords.push_back(punctuationWords[i]);
            }
        }
        
        if (includeNumbers) {
            // Add number words to the pool (mixed with words)
            for (size_t i = 0; i < numberWords.size(); i++) {
                combinedWords.push_back(numberWords[i]);
            }
        }
        
        // Generate target text using selected word count and combined word pool
        for (int i = 0; i < wordCount; i++) {
            if (i > 0) target += " ";  // Add space between words
            target += combinedWords[rand() % combinedWords.size()];  // Pick random word from combined pool
        }
    }
    
    std::string typed = "";        // What user has typed so far
    time_t start_time = 0;         // When user started typing
    bool started = false;          // Track if timing has begun
    
    // Helper function to find next word start position (converted to avoid lambda)
    // We'll implement this inline in the space handler below
    
    // Word jump tracking
    size_t jumped_from_pos = std::string::npos;  // Position where space jump occurred
    bool has_jumped = false;       // Whether a word jump is active
    
    // Ball animation variables
    int ball_frame = 0;            // Animation frame for rolling ball
    double ball_position = 0.0;    // Ball position (0.0 to 1.0 across screen)
    
    int ch;                        // Variable to store key pressed
    while ((ch = getch()) != 27) { // Main loop - ESC (27) to quit
        
        // Handle different types of input
        if (ch == KEY_BACKSPACE || ch == 127 || ch == 8) {  // Backspace handling
            if (has_jumped && typed.length() > jumped_from_pos) {
                // If we jumped and are past the jump point, return to jump position
                typed = typed.substr(0, jumped_from_pos);
                has_jumped = false;
                jumped_from_pos = std::string::npos;
            } else if (!typed.empty()) {  // Normal backspace
                typed.pop_back();
                has_jumped = false;  // Clear any jump state
                jumped_from_pos = std::string::npos;
            }
        } else if (ch == ' ' || ch == 32) { // Space key - special handling
            if (typed.length() < target.length()) {
                if (!started) {
                    start_time = time(nullptr);
                    started = true;
                }
                
                // Check if we're in the middle of a word (next char isn't space)
                if (typed.length() < target.length() && target[typed.length()] != ' ') {
                    // We're in the middle of a word - jump to next word start
                    jumped_from_pos = typed.length();
                    
                    // Find next word start position inline
                    size_t next_word_pos = target.length(); // Default to end
                    size_t space_pos = target.find(' ', typed.length());
                    if (space_pos != std::string::npos) {
                        // Skip any consecutive spaces and find next non-space
                        while (space_pos < target.length() && target[space_pos] == ' ') {
                            space_pos++;
                        }
                        next_word_pos = space_pos;
                    }
                    
                    // Fill with incorrect markers for skipped letters
                    while (typed.length() < next_word_pos && typed.length() < target.length()) {
                        if (target[typed.length()] == ' ') {
                            typed += ' ';  // Keep spaces as spaces
                        } else {
                            typed += '_';  // Mark skipped letters as incorrect
                        }
                    }
                    has_jumped = true;
                } else {
                    // Normal space - we're at a space position
                    typed += ' ';
                    has_jumped = false;
                    jumped_from_pos = std::string::npos;
                }
            }
        } else if (ch >= 33 && ch <= 126) { // Other printable ASCII characters (not space)
            if (typed.length() < target.length()) {  // Don't go past target
                if (!started) {    // Start timer on first keypress
                    start_time = time(nullptr);
                    started = true;
                }
                typed += (char)ch; // Add character to typed string
                has_jumped = false; // Clear jump state on normal typing
                jumped_from_pos = std::string::npos;
            }
        } else if (ch == 10 || ch == 13) { // Enter key (newline/carriage return)
            // Restart with new text
            typed = "";        // Clear typed text
            started = false;   // Reset timer
            target = "";       // Clear target
            ball_position = 0.0;  // Reset ball position
            ball_frame = 0;       // Reset ball animation
            has_jumped = false;   // Reset jump state
            jumped_from_pos = std::string::npos;
            // Generate new text with selected word count using same logic as initial generation
            if (includeNumbers && !includePunctuation) {
                // Numbers only mode: only pure numbers and spaces
                std::vector<std::string> pureNumbers;
                pureNumbers.push_back("0"); pureNumbers.push_back("1"); pureNumbers.push_back("2"); pureNumbers.push_back("3");
                pureNumbers.push_back("4"); pureNumbers.push_back("5"); pureNumbers.push_back("6"); pureNumbers.push_back("7");
                pureNumbers.push_back("8"); pureNumbers.push_back("9"); pureNumbers.push_back("10"); pureNumbers.push_back("11");
                pureNumbers.push_back("12"); pureNumbers.push_back("13"); pureNumbers.push_back("14"); pureNumbers.push_back("15");
                pureNumbers.push_back("16"); pureNumbers.push_back("17"); pureNumbers.push_back("18"); pureNumbers.push_back("19");
                pureNumbers.push_back("20"); pureNumbers.push_back("25"); pureNumbers.push_back("30"); pureNumbers.push_back("42");
                pureNumbers.push_back("50"); pureNumbers.push_back("75"); pureNumbers.push_back("99"); pureNumbers.push_back("100");
                pureNumbers.push_back("123"); pureNumbers.push_back("456"); pureNumbers.push_back("789"); pureNumbers.push_back("1000");
                pureNumbers.push_back("2024"); pureNumbers.push_back("2025"); pureNumbers.push_back("3000"); pureNumbers.push_back("5000");
                
                for (int i = 0; i < wordCount; i++) {
                    if (i > 0) target += " ";
                    target += pureNumbers[rand() % pureNumbers.size()];
                }
            } else {
                // Create combined word pool based on selected options
                std::vector<std::string> combinedWords = words;  // Start with base words
                
                if (includePunctuation) {
                    // Add punctuation words to the pool
                    for (size_t i = 0; i < punctuationWords.size(); i++) {
                        combinedWords.push_back(punctuationWords[i]);
                    }
                }
                
                if (includeNumbers) {
                    // Add number words to the pool (mixed with words)
                    for (size_t i = 0; i < numberWords.size(); i++) {
                        combinedWords.push_back(numberWords[i]);
                    }
                }
                
                for (int i = 0; i < wordCount; i++) {
                    if (i > 0) target += " ";
                    target += combinedWords[rand() % combinedWords.size()];
                }
            }
        } else if (ch == 'l' || ch == 'L') { // Show leaderboard
            int leaderboardResult = showLeaderboard(leaderboard);
            if (leaderboardResult == 2) {
                // Change name requested
                std::string newPlayerName = getPlayerName(leaderboard);
                if (newPlayerName != "CANCEL") {
                    while (newPlayerName == "WORM_CLOSET") {
                        showWormCloset();
                        newPlayerName = getPlayerName(leaderboard);
                        if (newPlayerName == "CANCEL") {
                            break;
                        }
                    }
                    if (newPlayerName != "CANCEL") {
                        playerName = newPlayerName; // Update player name
                    }
                }
            } else if (leaderboardResult == 3) {
                // Worm closet requested
                showWormCloset();
            }
        } else if (ch == 'W') { // Show worm closet (only capital W to avoid collision with typing)
            showWormCloset();
        }
        
        // Update ball position and animation based on typing progress
        if (target.length() > 0) {
            ball_position = (double)typed.length() / target.length();
            ball_frame++;  // Advance animation frame
        }
        
        // Clear and redraw screen
        clear();                   // Clear entire screen
        
        // Get terminal dimensions
        int max_x, max_y;
        getmaxyx(stdscr, max_y, max_x);
        
        // Calculate window dimensions and position
        int window_width = max_x - 4;  // Leave 2 chars padding on each side
        int window_height = max_y - 4; // Leave 2 lines padding top/bottom
        int win_start_x = 2;
        int win_start_y = 1;
        
        // Draw window outline using box drawing characters
        // Top border
        mvaddch(win_start_y, win_start_x, ACS_ULCORNER);
        for (int x = win_start_x + 1; x < win_start_x + window_width - 1; x++) {
            mvaddch(win_start_y, x, ACS_HLINE);
        }
        mvaddch(win_start_y, win_start_x + window_width - 1, ACS_URCORNER);
        
        // Side borders
        for (int y = win_start_y + 1; y < win_start_y + window_height - 1; y++) {
            mvaddch(y, win_start_x, ACS_VLINE);
            mvaddch(y, win_start_x + window_width - 1, ACS_VLINE);
        }
        
        // Bottom border
        mvaddch(win_start_y + window_height - 1, win_start_x, ACS_LLCORNER);
        for (int x = win_start_x + 1; x < win_start_x + window_width - 1; x++) {
            mvaddch(win_start_y + window_height - 1, x, ACS_HLINE);
        }
        mvaddch(win_start_y + window_height - 1, win_start_x + window_width - 1, ACS_LRCORNER);
        
        // Display title inside the window
        std::string title = "W4RMUP W0RM'S T3RMINAL TYP3R";
        int title_x = win_start_x + (window_width - title.length()) / 2;
        mvprintw(win_start_y + 1, title_x, "%s", title.c_str());
        
        // Display instructions at bottom of window
        std::string instruct = "ENTER: restart | ESC: quit | L: leaderboard | W: worm closet";
        int instruct_x = win_start_x + (window_width - instruct.length()) / 2;
        mvprintw(win_start_y + window_height - 3, instruct_x, "%s", instruct.c_str());
        
        // Display separator line
        for (int x = win_start_x + 2; x < win_start_x + window_width - 2; x++) {
            mvaddch(win_start_y + 2, x, ACS_HLINE);
        }
        
        // Display prompt inside window (centered)
        std::string prompt = "Type this:";
        int prompt_x = win_start_x + (window_width - prompt.length()) / 2;
        mvprintw(win_start_y + 4, prompt_x, "%s", prompt.c_str());
        
        // Draw bouncy worm animation above the text
        int worm_y = win_start_y + 5;
        int worm_start_x = win_start_x + 2;
        int worm_width = window_width - 4;
        drawBouncyWorm(worm_y, worm_start_x, worm_width, ball_position, ball_frame);
        
        // Display target text with word-wrapping and color coding inside window
        int current_row = win_start_y + 7;
        int start_col = win_start_x + 2;  // Inside window margin
        int current_col = start_col;
        int max_text_width = window_width - 4;  // Text area width inside window
        
        // Split text into words for proper wrapping
        std::vector<std::string> words_in_target;
        std::string current_word = "";
        for (size_t i = 0; i < target.length(); i++) {
            if (target[i] == ' ') {
                if (!current_word.empty()) {
                    words_in_target.push_back(current_word);
                    current_word = "";
                }
                words_in_target.push_back(" ");  // Add space as separate element
            } else {
                current_word += target[i];
            }
        }
        if (!current_word.empty()) {
            words_in_target.push_back(current_word);
        }
        
        // Render words with proper wrapping and track cursor position
        size_t char_pos = 0;  // Track position in original target string
        int cursor_row = current_row;   // Track where cursor should be
        int cursor_col = current_col;   // Track where cursor should be
        int last_text_row = current_row;  // Track the last row where text was rendered
        
        for (size_t word_idx = 0; word_idx < words_in_target.size(); word_idx++) {
            const std::string& word = words_in_target[word_idx];
            
            // Check if word fits on current line
            if (word != " " && current_col + word.length() > win_start_x + max_text_width) {
                // Move to next line if word doesn't fit
                current_row++;
                current_col = start_col;
            }
            
            // Render each character in the word
            for (size_t char_idx = 0; char_idx < word.length(); char_idx++) {
                int color = 3;  // Default to white (untyped)
                
                if (char_pos < typed.length()) {  // If user has typed this far
                    if (typed[char_pos] == target[char_pos]) {
                        color = 1;  // Custom orange-red color for correct characters
                    } else {
                        color = 2;  // Red for incorrect characters
                    }
                }
                
                // Update cursor position - should be at the next character to type
                if (char_pos == typed.length()) {
                    cursor_row = current_row;
                    cursor_col = current_col;
                }
                
                if (has_colors()) {
                    attron(COLOR_PAIR(color));
                }
                mvaddch(current_row, current_col, word[char_idx]);
                if (has_colors()) {
                    attroff(COLOR_PAIR(color));
                }
                
                current_col++;
                char_pos++;
                last_text_row = current_row;  // Keep track of the last row with text
            }
        }
        
        // Calculate dynamic position for stats - always 2 rows below the last text line
        int stats_start_y = last_text_row + 2;
        
        // Display progress counter inside window (dynamically positioned and centered)
        std::string progressText = "Progress: " + std::to_string(typed.length()) + "/" + std::to_string(target.length());
        int progress_x = win_start_x + (window_width - progressText.length()) / 2;
        mvprintw(stats_start_y, progress_x, "%s", progressText.c_str());
        
        // Calculate and display live statistics inside window (dynamically positioned)
        if (started && typed.length() > 0) {    // Only if typing has begun
            double elapsed = difftime(time(nullptr), start_time);  // Time elapsed
            if (elapsed > 0) {     // Avoid division by zero
                int correct = 0;   // Count correct characters
                for (size_t i = 0; i < typed.length() && i < target.length(); i++) {
                    if (typed[i] == target[i]) correct++;
                }
                
                // WPM = (correct chars / 5) / (time in minutes)
                double raw_wpm = (correct / 5.0) / (elapsed / 60.0);
                // Accuracy = (correct chars / total typed) * 100
                double accuracy = (correct * 100.0) / typed.length();
                // Apply accuracy penalty to prevent space-mashing exploit
                // Below 50% accuracy, severely penalize WPM
                double accuracy_multiplier = 1.0;
                if (accuracy < 50.0) {
                    accuracy_multiplier = accuracy / 50.0;  // Linear penalty below 50%
                }
                double wpm = raw_wpm * accuracy_multiplier;
                
                // Center the WPM stats line
                char statsBuffer[100];
                snprintf(statsBuffer, sizeof(statsBuffer), "WPM: %.1f | Accuracy: %.1f%% | Time: %.0fs", 
                         wpm, accuracy, elapsed);
                std::string statsText = std::string(statsBuffer);
                int stats_x = win_start_x + (window_width - statsText.length()) / 2;
                mvprintw(stats_start_y + 1, stats_x, "%s", statsText.c_str());
            }
        }
        
        // Show completion message inside window
        if (typed.length() == target.length()) {
            // Calculate final stats
            double elapsed = difftime(time(nullptr), start_time);
            int correct = 0;
            for (size_t i = 0; i < typed.length() && i < target.length(); i++) {
                if (typed[i] == target[i]) correct++;
            }
            double raw_final_wpm = (correct / 5.0) / (elapsed / 60.0);
            double final_accuracy = (correct * 100.0) / typed.length();
            // Apply same accuracy penalty to final WPM calculation
            double final_accuracy_multiplier = 1.0;
            if (final_accuracy < 50.0) {
                final_accuracy_multiplier = final_accuracy / 50.0;  // Linear penalty below 50%
            }
            double final_wpm = raw_final_wpm * final_accuracy_multiplier;
            
            // Add to leaderboard and save
            PlayerScore newScore(playerName, final_wpm, final_accuracy, elapsed, wordCount, includePunctuation, includeNumbers);
            addToLeaderboard(leaderboard, newScore);
            saveLeaderboard(leaderboard);
            
            // Check for achievements
            checkAchievements(final_wpm, final_accuracy, elapsed);
            
            // Center the completion messages
            std::string completeMsg = "COMPLETE! leaderboard!";
            std::string continueMsg = "Press Enter to view leaderboard | Q to quit";
            int complete_x = win_start_x + (window_width - completeMsg.length()) / 2;
            int continue_x = win_start_x + (window_width - continueMsg.length()) / 2;
            mvprintw(win_start_y + 32, complete_x, "%s", completeMsg.c_str());
            mvprintw(win_start_y + 13, continue_x, "%s", continueMsg.c_str());
            refresh();
            
            // Wait for Enter to continue or Q to quit
            while (true) {
                int complete_ch = getch();
                if (complete_ch == 10 || complete_ch == 13) {
                    break;
                } else if (complete_ch == 'q' || complete_ch == 'Q') {
                    // Quit the entire application
                    cleanup();
                    return 0;
                }
            }
            
            // Show leaderboard after completion
            int leaderboardResult = showLeaderboard(leaderboard);
            if (leaderboardResult == 2) {
                // Change name requested
                std::string newPlayerName = getPlayerName(leaderboard);
                if (newPlayerName == "CANCEL") {
                    break; // Exit if cancelled
                }
                while (newPlayerName == "WORM_CLOSET") {
                    showWormCloset();
                    newPlayerName = getPlayerName(leaderboard);
                    if (newPlayerName == "CANCEL") {
                        break;
                    }
                }
                if (newPlayerName != "CANCEL") {
                    playerName = newPlayerName; // Update player name
                }
            } else if (leaderboardResult == 3) {
                // Worm closet requested
                showWormCloset();
            }
            break; // Exit typing test loop to return to menu
        }
        
        // Position cursor at the current typing position (AFTER all display calls)
        if (typed.length() < target.length()) {
            curs_set(1);  // Show cursor when typing
            move(cursor_row, cursor_col);
        } else {
            curs_set(0);  // Hide cursor when not typing
        }
        
        refresh();             // Update screen with all changes
        }  // End of typing test game loop
    }  // End of main menu loop
    
    cleanup();                 // Clean up shader and ncurses
    return 0;
}
