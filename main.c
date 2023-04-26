
#include <ncurses.h>
#include <panel.h>
#include <stdlib.h>
#include <string.h>
#include "word_art.h"
#include "client.h"
#include <pthread.h>
#include <sys/socket.h>
#include <errno.h>
#include <unistd.h>

// 为了使用int32_t
#include <stdint.h>

/*
 * 变量声明
 */

// 宏定义
#define INPUT_MAX 20  // 定义输入框最大长度
#define MAX_USERNAME_LENGTH 20

// 面板结构体
typedef struct PanelData PanelData; // 前向声明

struct PanelData {
    WINDOW *win;
    PANEL *panel;
    int height;
    int width;
};


// 光标开始绘制坐标
int start_y;
int start_x;

// 屏幕尺寸
static int cols, rows;

// 最顶端的面板
static PanelData *current_panel_data;

// art_chat的行数
int n_lines = sizeof(art_chat) / sizeof(char *);

// 程序退出标志
bool is_running = true;

// 面板定义
PanelData *panel_data_chat;
PanelData *panel_data_menu;
PanelData *panel_data_sign_in;
PanelData *panel_data_sign_up;

WINDOW *msg_win_others;
WINDOW *msg_win_me;
// 聊天窗口尺寸
#define MAX_MESSAGES 30
#define MSG_WIDTH 50
#define INPUT_HEIGHT 3

#ifndef BUTTON5_PRESSED
#define BUTTON5_PRESSED (BUTTON4_PRESSED << 1)
#endif


// 定义消息数组(me)
char messages_me[MAX_MESSAGES][MSG_WIDTH];
int num_msgs_me = 0;

// 定义消息数组(others)
char messages_others[MAX_MESSAGES][MSG_WIDTH];
int num_msgs_others = 0;

// 当前用户名
char current_username[MAX_USERNAME_LENGTH];

/*
 * 函数声明
 */

/*
 * 初始化 ncurses
 * @return void
 */
void init_ncurses();

PanelData *init_windows(int height, int width);

void print_art(WINDOW *win, const char *art_word[], int lines, int start_y, int start_x);

void add_to_menu(PanelData *panel_data);

/*
 * 用户注册
 * @param username 用户名
 * @return 0 注册成功
 * @return 1 用户名已存在
 * @return 2 文件打开失败
 */
int register_user(const char *username);

/*
 * 用户登录
 * @param username 用户名
 * @return 0 登录成功
 * @return 1 用户名不存在
 * @return 2 文件打开失败
 */
int login_user(const char *username);

void add_to_sign_in(PanelData *panel_data);

void add_to_sign_up(PanelData *panel_data);

void add_to_chat(PanelData *panel_data);

/*
 * ncurses 有窗口和面板两个屏幕概念, 可以在窗口上绘制文字, 但是不能处理层叠问题,
 * 面板可以处理层叠问题, 但是不能绘制文字, 每一个panel只能关联一个窗口,
 * fresh()负责更新面板的层叠顺序, 例如: top_panel(menu) 将menu置于最顶端,
 * 然后调用fresh()更新层叠顺序, 使得menu在最顶端.
 */
void fresh(void);

void print_messages(WINDOW *msg_win, char messages[MAX_MESSAGES][MSG_WIDTH], int num_msgs);

void add_message(char messages[MAX_MESSAGES][MSG_WIDTH], int *num_msgs, const char *msg);

void *receive_messages(void *win_output);


int main() {
    // 初始化 ncurses
    init_ncurses();

    // 初始化menu, sign_in, sign_up, chat panel
    panel_data_menu = init_windows(rows, cols);
    panel_data_sign_in = init_windows(rows, cols);
    panel_data_sign_up = init_windows(rows, cols);
    panel_data_chat = init_windows(rows, cols);

    // 初始化与选项(登录/注册)相关的变量
    char *options[] = {"Sign in", "Sign up"};
    int option_count = sizeof(options) / sizeof(char *);
    int current_option = 0;

    // 设置当前面板为menu
    current_panel_data = panel_data_menu;

    while (is_running) {
        while (current_panel_data == panel_data_menu) {
            // 显示menu面板
            top_panel(panel_data_menu->panel);

            // 添加静态内容到menu窗口
            add_to_menu(panel_data_menu);

            // 添加动态内容(选项)到menu窗口
            for (int i = 0; i < option_count; i++) {
                if (current_option == i) {
                    wattron(panel_data_menu->win, A_REVERSE);  // 被选中的选项加粗
                }
                if (i == 0) {
                    mvwprintw(panel_data_menu->win, start_y + n_lines + 2, start_x, "Sign in");
                } else {
                    // 这里的95是暂定的 代表sign up 选项的横坐标位置
                    mvwprintw(panel_data_menu->win, start_y + n_lines + 2, 95, "Sign up");
                }
                wattroff(panel_data_menu->win, A_REVERSE);
            }
            wrefresh(panel_data_menu->win);

            int input = wgetch(panel_data_menu->win);
            // 处理用户输入
            switch (input) {
                case KEY_UP:
                    if (current_option > 0) {
                        current_option--;
                    }
                    break;

                case KEY_DOWN:
                    if (current_option < option_count - 1) {
                        current_option++;
                    }
                    break;
                case 'q':
                    endwin();
                    exit(0);
                default :
                    if (current_option == 0) {
                        current_panel_data = panel_data_sign_in;

                    } else if (current_option == 1) {
                        current_panel_data = panel_data_sign_up;
                    }
                    break;
            }
            werase(panel_data_menu->win);
            wrefresh(panel_data_menu->win);
        }

        while (current_panel_data == panel_data_sign_up) {
            top_panel(current_panel_data->panel);
            fresh();
            add_to_sign_up(current_panel_data);

            werase(panel_data_sign_up->win);
            wrefresh(panel_data_sign_up->win);
        }
        while (current_panel_data == panel_data_sign_in) {
            top_panel(current_panel_data->panel);
            fresh();
            add_to_sign_in(current_panel_data);

            werase(panel_data_sign_in->win);
            wrefresh(panel_data_sign_in->win);

        }

        while (current_panel_data == panel_data_chat) {
            curs_set(0);
            top_panel(current_panel_data->panel);
            fresh();
            add_to_chat(panel_data_chat);

            if (current_panel_data == panel_data_chat) {
                werase(panel_data_chat->win);
                wrefresh(panel_data_chat->win);

                // 定义聊天记录窗口大小
                int board_height = rows - INPUT_HEIGHT - 2;

                // 定义输入窗口
                WINDOW *input_win = newwin(INPUT_HEIGHT, cols, board_height, 0);
                box(input_win, 0, 0);

                // 定义一个线程ID变量
                pthread_t tid;

                // 客户端初始化
                client_socket = setup_client();

                while (is_running) {
                    // 打印其他人的信息
                    msg_win_others = newwin(board_height-2, (cols / 2)-1, 1, 1);


                    // 打印输入框
                    mvwprintw(input_win, 1, 1, "Input your msg: ");
                    box(input_win, 0, 0);

                    curs_set(0);
                    wrefresh(input_win);
                    wrefresh(panel_data_chat->win);

                    //定义输入消息
                    char input[MSG_WIDTH - 1];
                    echo();
                    keypad(input_win, TRUE);
                    // 获取输入的消息
                    wgetnstr(input_win, input, MSG_WIDTH - 2);
                    noecho();

                    // 当输入/q时，退出聊天室
                    if (strcmp(input, "/q") == 0) {
                        current_panel_data = panel_data_menu;
                        is_running = false;
                        pthread_join(tid, NULL);

                        break;
                    } else {
                        // 拼接用户名
                        char msg_cat[MSG_WIDTH+MAX_USERNAME_LENGTH];
                        sprintf(msg_cat, "[%s]: %s", current_username, input);
                        // 发送信息到服务器
                        send_msg(client_socket, msg_cat);
                        // 添加消息到聊天记录
                        add_message(messages_me, &num_msgs_me, msg_cat);

                        print_messages(panel_data_chat->win, messages_me, num_msgs_me);

                        curs_set(0);
                        mvwprintw(panel_data_chat->win, rows-2, 1, "Send '/q' to quit.");

                        pthread_create(&tid, NULL, receive_messages, msg_win_others);
                        print_messages(msg_win_others, messages_others, num_msgs_others);
                        // 刷新聊天记录

                        wrefresh(panel_data_chat->win);
                        wrefresh(msg_win_others);
                    }
                    werase(input_win);

                }

                delwin(msg_win_others);
                delwin(input_win);

                close(client_socket);
            } else {
                werase(panel_data_chat->win);
            }
        }
    }
    // 释放内存
    del_panel(panel_data_menu->panel);
    del_panel(panel_data_sign_in->panel);
    del_panel(panel_data_sign_up->panel);
    del_panel(panel_data_chat->panel);
    free(panel_data_menu);
    free(panel_data_sign_in);
    free(panel_data_sign_up);
    free(panel_data_chat);

    // 退出 ncurses
    endwin();

    return 0;
}


void init_ncurses() {
    initscr();
    cbreak();
    noecho();
    curs_set(0);

    // 获取屏幕尺寸
    getmaxyx(stdscr, rows, cols);

    // 设置光标开始绘制坐标
    start_x = (cols - CHAT_ART_WIDTH) / 2;
    start_y = (rows - CHAT_ART_HIGH) / 2;
}

PanelData *init_windows(int height, int width) {

    WINDOW *win = newwin(height, width, 0, 0);
    PANEL *panel = new_panel(win);
    PanelData *panel_data = (PanelData *) malloc(sizeof(PanelData));

    panel_data->win = win;
    panel_data->panel = panel;
    panel_data->height = height;
    panel_data->width = width;

    // 设置键盘响应模式
    keypad(panel_data->win, TRUE);

    return panel_data;
}


void print_art(WINDOW *win, const char *art_word[], int lines, int y, int x) {
    for (int i = 0; i < lines; i++) {
        mvwprintw(win, y + i, x, art_word[i]);
    }
}

void add_to_menu(PanelData *panel_data) {
    // 打印art_chat
    print_art(panel_data->win, art_chat, n_lines, start_y, start_x);

    //  显示退出提示
    mvwprintw(panel_data->win, rows - 1, 0, "Press 'q' to quit");
    mvwprintw(panel_data->win, 0, 0, "menu");

}

/* 注册用户 */
int register_user(const char *username) {
    FILE *fp = fopen("usernames.txt", "a+");
    rewind(fp);
    if (fp == NULL) {
//        printf("Error: Failed to open file.\n");
        return 2;
    }

    /* 检查用户名是否已经存在 */
    char line[MAX_USERNAME_LENGTH];
    while (fgets(line, MAX_USERNAME_LENGTH, fp)) {
        // 去掉换行符
        strtok(line, "\n");
        if (strcmp(line, username) == 0) {
            // 调试使用
//            mvwprintw(panel_data_sign_up->win, 3, 3, "Error: Username already exists.");
//            printf("Error: Username already exists.\n");
            fclose(fp);
            return 1;
        }
    }

    /* 用户名不存在，将其存储到文件中 */
    fprintf(fp, "%s\n", username);
    fflush(fp);
    fclose(fp);
//    printf("User %s registered successfully.\n", username);

    return 0;
}

/* 用户登录 */
int login_user(const char *username) {
    FILE *fp = fopen("usernames.txt", "r");
    if (fp == NULL) {
//        printf("Error: Failed to open file.\n");
        return 2;
    }

    /* 检查用户名是否存在 */
    char line[MAX_USERNAME_LENGTH];
    while (fgets(line, MAX_USERNAME_LENGTH, fp)) {
        strtok(line, "\n");
        if (strcmp(line, username) == 0) {
//            printf("User %s logged in successfully.\n", username);
            fclose(fp);
            return 0;
        }
    }

    /* 用户名不存在 */
//    printf("Error: Username not found.\n");
    fclose(fp);
    return 1;
}


void add_to_sign_in(PanelData *panel_data) {
    curs_set(1);
    echo();

    attron(A_BOLD);
    mvwprintw(panel_data->win, 0, 0, "sign in");
    attron(A_BOLD);

    char input_str[INPUT_MAX + 1];  // 定义输入框字符串

    mvwprintw(panel_data->win, rows / 2, (cols - 20) / 2, "Please input your name:");

    wrefresh(panel_data->win);

    // 创建输入框
    mvgetnstr(rows / 2 + 1, (cols - INPUT_MAX) / 2, input_str, INPUT_MAX);

    // 用户登陆
    if (login_user(input_str) == 0) {
        mvwprintw(panel_data->win, rows - 3, 0, "Your user name is: %s", input_str);
        mvwprintw(panel_data->win, rows - 2, 0, "Press 'c' to Chat!");
        mvwprintw(panel_data->win, rows - 1, 0, "Press 'q' to quit!");
//        mvwprintw(panel_data->win, rows - 4, 0, "rows: %d", rows);
        strcpy(current_username, input_str);
    } else {
        mvwprintw(panel_data->win, rows - 3, 0, "The user name does not exist", input_str);
        mvwprintw(panel_data->win, rows - 2, 0, "Press any key to sign up.");
        mvwprintw(panel_data->win, rows - 1, 0, "Press 'q' to quit!");
    }

    wrefresh(panel_data->win);

    int input;
    switch (input = wgetch(panel_data->win)) {
        case 'c':
            // 进入聊天室
            current_panel_data = panel_data_chat;
            break;
        case 'q':
            // 退出
            current_panel_data = panel_data_menu;
            break;
        default:
            // 进入注册
            current_panel_data = panel_data_sign_up;
            break;
    }
}

void add_to_sign_up(PanelData *panel_data) {
    curs_set(1);
    echo();

    attron(A_BOLD);
    mvwprintw(panel_data->win, 0, 0, "sign up");
    attron(A_BOLD);
    char input_str[INPUT_MAX + 1];  // 定义输入框字符串

    mvwprintw(panel_data->win, rows / 2, (cols - 20) / 2, "Please input your name:");

    wrefresh(panel_data->win);

    // 创建输入框
    mvgetnstr(rows / 2 + 1, (cols - INPUT_MAX) / 2, input_str, INPUT_MAX);

    // 用户注册
    if (register_user(input_str) == 0) {
        mvwprintw(panel_data->win, rows - 3, 0, "Your user name is: %s", input_str);
        mvwprintw(panel_data->win, rows - 2, 0, "Press any key to sign in!");
        mvwprintw(panel_data->win, rows - 1, 0, "Press 'q' to quit!");
    } else {
        mvwprintw(panel_data->win, rows - 3, 0, "Your user name is: %s, but it have been used!", input_str);
        mvwprintw(panel_data->win, rows - 2, 0, "Press 'n' to type again.");
        mvwprintw(panel_data->win, rows - 1, 0, "Press 'q' to quit!");
    }

    wrefresh(panel_data->win);

    int input;
    switch (input = wgetch(panel_data->win)) {
        case 'n':
            // 继续注册
            current_panel_data = panel_data_sign_up;
            break;
        case 'q':
            // 退出
            current_panel_data = panel_data_menu;
            break;
        default:
            // 进入登陆
            current_panel_data = panel_data_sign_in;
    }

}

void add_to_chat(PanelData *panel_data) {
    mvwprintw(panel_data->win, rows/2, (cols-25)/2, "Your user name is : %s", current_username);
    mvwprintw(panel_data->win, rows - 2, 0, "Press any key to chat.");
    mvwprintw(panel_data->win, rows - 1, 0, "Press 'q' to quit.");

    wrefresh(panel_data->win);

    int input;
    switch (input = wgetch(panel_data->win)) {
        case 'q':
            // 退出
            current_panel_data = panel_data_menu;
            break;
        default: {
            // 进入聊天室
            current_panel_data = panel_data_chat;
            break;
        }
    }
}

void fresh(void) {
    update_panels();
    doupdate();
}

void print_messages(WINDOW *msg_win, char messages[MAX_MESSAGES][MSG_WIDTH], int num_msgs) {
    werase(msg_win);
    for (int i = 0; i < num_msgs; i++) {
        if(messages == messages_me) {
            mvwprintw(msg_win, i + 2, cols / 2, messages[i]);
        } else{
            mvwprintw(msg_win, i + 1, 1, messages[i]);
        }
    }

    box(msg_win, 0, 0);
    wrefresh(msg_win);
}

// 添加信息到聊天记录
void add_message(char messages[MAX_MESSAGES][MSG_WIDTH], int *num_msgs, const char *msg) {

    if (*num_msgs < MAX_MESSAGES) {
        strncpy(messages[*num_msgs], msg, MSG_WIDTH - 1);
        (*num_msgs)++;
    } else {
        for (int i = 0; i < MAX_MESSAGES - 1; i++) {
            strncpy(messages[i], messages[i + 1], MSG_WIDTH - 1);
        }
        strncpy(messages[MAX_MESSAGES - 1], msg, MSG_WIDTH - 1);
    }
}


void *receive_messages(void *win_output) {
    char buf[BUF_SIZE];
    int64_t num_bytes;

    while (is_running) {
        if ((num_bytes = recv(client_socket, buf, BUF_SIZE, 0)) > 0) {
            buf[num_bytes] = '\0';

            add_message(messages_others, &num_msgs_others, buf);
            print_messages(msg_win_others, messages_others, num_msgs_others);

            wrefresh(win_output);
        }

        if (num_bytes == -1 && errno != EAGAIN && errno != EWOULDBLOCK) {
            perror("Error receiving message");
            exit(1);
        }
    }

    pthread_exit(NULL);
}

